"""원본맵(.scx)에서 스테이지 지형, 깃발 위치, 장애물 생성 정보를 꺼내 JSON으로 저장한다.

사용법: python -I extract_scx.py <map.scx> <out.json>

보호 MPQ(정크 해시 항목, 조작된 블록 테이블 오프셋)도 SC 로더처럼 필요한 항목만 따라가면 읽힌다.
CHK의 TRIG/MRGN은 보호용 더미라서 쓰지 않는다. 실제 로직은 STRx 문자열 뒤에 붙은 euddraft 페이로드에 있다.
페이로드는 암호화되어 있지 않은 트리거 묶음이라, 로케이션 좌표를 상수로 쓰고 바로 유닛을 만들거나
이동시키는 트리거는 정적으로 읽을 수 있다. 좌표를 변수로 계산하는 트리거와 빙판 판정 로직은 읽을 수 없다.
"""
import json
import struct
import sys

CHK_NAME = "staredit\\scenario.chk"
STAGE_ROWS = 112  # 스테이지는 y 8~106에 있다. 아래쪽은 지오이저/시작위치로 만든 글자 장식이다.

# CV5 그룹 쌍(group >> 1) -> 지형 문자. 평지 타일은 짝/홀 열에 두 그룹이 번갈아 깔린다.
TERRAIN_CHARS = {1: "-", 2: " ", 3: "3", 4: "4", 7: "7", 12: "C", 13: "D"}

UNIT_FLAG = 215
MARKER_UNITS = {
    21: "TomKazansky",
    55: "KukulzaMutalisk",
    56: "KukulzaGuardian",
    80: "Mojo",
    82: "Gantrithor",
    102: "GerardDuGalle",
}

# euddraft 페이로드 트리거 해석용
DEATHS_TABLE = 0x58A364  # EUD: SetDeaths(player, unit)의 대상 주소 = DEATHS_TABLE + 4 * (player + 12 * unit)
MRGN_TABLE = 0x58DC60  # 로케이션 배열. 항목당 20바이트(left, top, right, bottom, string/flags)
LOCATION_COUNT = 255
ACTION_CREATE_UNIT = 44
ACTION_SET_DEATHS = 45
ACTION_ORDER = 46
MODIFIER_SET_TO = 7
OBSTACLE_PLAYER = 7  # Player 8 (0부터 셈)
CREATED_UNITS = {
    2: "Vulture",
    7: "SCV",
    65: "Zealot",
    66: "Dragoon",
    67: "HighTemplar",
    76: "TassadarZeratulArchon",
    80: "Mojo",
    130: "InfestedCommandCenter",
    197: "ZergFlagBeacon",
    198: "TerranFlagBeacon",
}


# ---------------------------------------------------------------- MPQ
def _build_crypt_table():
    table = [0] * 0x500
    seed = 0x00100001
    for index1 in range(256):
        index2 = index1
        for _ in range(5):
            seed = (seed * 125 + 3) % 0x2AAAAB
            temp1 = (seed & 0xFFFF) << 16
            seed = (seed * 125 + 3) % 0x2AAAAB
            temp2 = seed & 0xFFFF
            table[index2] = temp1 | temp2
            index2 += 0x100
    return table


CRYPT = _build_crypt_table()
MASK = 0xFFFFFFFF


def hash_string(text, hash_type):
    seed1, seed2 = 0x7FED7FED, 0xEEEEEEEE
    for ch in text.upper().encode("latin1"):
        seed1 = (CRYPT[hash_type * 256 + ch] ^ ((seed1 + seed2) & MASK)) & MASK
        seed2 = (ch + seed1 + seed2 + (seed2 << 5) + 3) & MASK
    return seed1


def decrypt(buf, key):
    count = len(buf) // 4
    words = list(struct.unpack("<%dI" % count, buf[: count * 4]))
    seed = 0xEEEEEEEE
    for i in range(count):
        seed = (seed + CRYPT[0x400 + (key & 0xFF)]) & MASK
        value = words[i] ^ ((key + seed) & MASK)
        key = ((((~key) << 0x15) + 0x11111111) & MASK) | (key >> 0x0B)
        seed = (value + seed + (seed << 5) + 3) & MASK
        words[i] = value
    return struct.pack("<%dI" % count, *words)


# ------------------------------------------------- PKWARE DCL explode (zlib contrib/blast.c 포팅)
MAXBITS = 13
LITLEN = [
    11, 124, 8, 7, 28, 7, 188, 13, 76, 4, 10, 8, 12, 10, 12, 10, 8, 23, 8,
    9, 7, 6, 7, 8, 7, 6, 55, 8, 23, 24, 12, 11, 7, 9, 11, 12, 6, 7, 22, 5,
    7, 24, 6, 11, 9, 6, 7, 22, 7, 11, 38, 7, 9, 8, 25, 11, 8, 11, 9, 12,
    8, 12, 5, 38, 5, 38, 5, 11, 7, 5, 6, 21, 6, 10, 53, 8, 7, 24, 10, 27,
    44, 253, 253, 253, 252, 252, 252, 13, 12, 45, 12, 45, 12, 61, 12, 45,
    44, 173,
]
LENLEN = [2, 35, 36, 53, 38, 23]
DISTLEN = [2, 20, 53, 230, 247, 151, 248]
LEN_BASE = [3, 2, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 40, 72, 136, 264]
LEN_EXTRA = [0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8]


def _construct(rep):
    """압축된 반복 길이 표 -> {(비트 길이, 코드): 심볼} 정규 허프만 표."""
    lengths = []
    for value in rep:
        lengths += [value & 15] * ((value >> 4) + 1)
    count = [0] * (MAXBITS + 1)
    for length in lengths:
        count[length] += 1
    offs = [0] * (MAXBITS + 1)
    for length in range(1, MAXBITS):
        offs[length + 1] = offs[length] + count[length]
    symbols = [0] * len(lengths)
    for symbol, length in enumerate(lengths):
        if length:
            symbols[offs[length]] = symbol
            offs[length] += 1
    table = {}
    first = index = 0
    for length in range(1, MAXBITS + 1):
        for k in range(count[length]):
            table[(length, first + k)] = symbols[index + k]
        index += count[length]
        first = (first + count[length]) << 1
    return table


LIT_CODE = _construct(LITLEN)
LEN_CODE = _construct(LENLEN)
DIST_CODE = _construct(DISTLEN)


def explode(src):
    pos = bitbuf = bitcnt = 0
    out = bytearray()

    def bits(need):
        nonlocal pos, bitbuf, bitcnt
        while bitcnt < need:
            bitbuf |= src[pos] << bitcnt
            pos += 1
            bitcnt += 8
        value = bitbuf & ((1 << need) - 1)
        bitbuf >>= need
        bitcnt -= need
        return value

    def decode(table):
        code = 0
        for length in range(1, MAXBITS + 1):
            code |= bits(1) ^ 1  # PKWARE 허프만 코드는 비트가 반전되어 있다
            symbol = table.get((length, code))
            if symbol is not None:
                return symbol
            code <<= 1
        raise ValueError("invalid PKWARE code")

    coded_literals = bits(8)
    dict_bits = bits(8)
    if coded_literals > 1 or not 4 <= dict_bits <= 6:
        raise ValueError("invalid PKWARE header")
    while True:
        if bits(1):
            symbol = decode(LEN_CODE)
            length = LEN_BASE[symbol] + bits(LEN_EXTRA[symbol])
            if length == 519:  # 종료 코드
                break
            shift = 2 if length == 2 else dict_bits
            dist = (decode(DIST_CODE) << shift) + bits(shift) + 1
            for _ in range(length):
                out.append(out[-dist])
        else:
            out.append(decode(LIT_CODE) if coded_literals else bits(8))
    return bytes(out)


def read_chk(data):
    magic, _, _, _, sector_shift, hash_offset, block_offset, hash_count, _ = struct.unpack(
        "<4sIIHHIIII", data[:32]
    )
    if magic != b"MPQ\x1a":
        raise ValueError("not an MPQ archive")

    hash_table = decrypt(data[hash_offset : hash_offset + hash_count * 16], hash_string("(hash table)", 3))
    entries = [struct.unpack("<IIHHI", hash_table[i * 16 : i * 16 + 16]) for i in range(hash_count)]
    name_a, name_b = hash_string(CHK_NAME, 1), hash_string(CHK_NAME, 2)
    index = hash_string(CHK_NAME, 0) % hash_count
    block_index = None
    for _ in range(hash_count):
        hash_a, hash_b, _, _, block = entries[index]
        if block == 0xFFFFFFFF:
            break
        if hash_a == name_a and hash_b == name_b:
            block_index = block
            break
        index = (index + 1) % hash_count
    if block_index is None:
        raise ValueError("scenario.chk not found")

    # 블록 테이블은 스트림 암호라 처음부터 필요한 항목까지 복호화한다.
    block_table = decrypt(data[block_offset : block_offset + (block_index + 1) * 16], hash_string("(block table)", 3))
    offset, _, file_size, flags = struct.unpack("<IIII", block_table[block_index * 16 : block_index * 16 + 16])
    if flags & 0x00010000:
        raise NotImplementedError("encrypted scenario.chk")
    if not flags & 0x00000200:
        raise NotImplementedError("expected multi-compressed sectors")

    sector_size = 512 << sector_shift
    sectors = (file_size + sector_size - 1) // sector_size
    sector_offsets = struct.unpack("<%dI" % (sectors + 1), data[offset : offset + (sectors + 1) * 4])
    chk = bytearray()
    for i in range(sectors):
        sector = data[offset + sector_offsets[i] : offset + sector_offsets[i + 1]]
        expected = min(sector_size, file_size - i * sector_size)
        if len(sector) == expected:
            chk += sector
        elif sector[0] == 0x08:
            chk += explode(sector[1:])
        else:
            raise NotImplementedError("compression mask 0x%02x" % sector[0])
    return bytes(chk)


def read_sections(chk):
    sections = {}
    pos = 0
    while pos + 8 <= len(chk):
        name = chk[pos : pos + 4].decode("latin1")
        size = struct.unpack("<i", chk[pos + 4 : pos + 8])[0]
        sections[name] = chk[pos + 8 : pos + 8 + size]  # 중복 섹션은 마지막 것이 이긴다
        pos += 8 + size
    return sections


# ------------------------------------------------- euddraft 페이로드
def string_data_end(strx):
    """STRx = u32 개수 + u32 오프셋 표 + 문자열 데이터. 마지막 문자열의 NUL 다음부터가 페이로드다."""
    count = struct.unpack("<I", strx[:4])[0]
    offsets = struct.unpack("<%dI" % count, strx[4 : 4 + 4 * count])
    end = 4 + 4 * count
    for offset in set(offsets):
        if offset:
            end = max(end, strx.find(b"\0", offset) + 1)
    return end


def parse_trigger_actions(block):
    """2400바이트가 그럴듯한 트리거면 액션 목록, 아니면 None. 데이터 영역을 트리거로 오인하지 않게 엄격히 본다."""
    for i in range(16):
        comparison, condition = block[i * 20 + 14], block[i * 20 + 15]
        if condition and (condition > 23 or comparison > 10):
            return None
    actions = []
    for i in range(64):
        offset = 320 + i * 32
        location, _, _, _, group, number, unit_id, action, modifier = struct.unpack(
            "<IIIIIIHBB", block[offset : offset + 28]
        )
        if action == 0:
            continue
        if action > 59 or modifier > 10:
            return None
        actions.append((action, group, number, unit_id, modifier, location))
    if not actions or any(flag > 1 for flag in block[2372:2400]):
        return None
    return actions


def scan_payload_triggers(payload):
    offset = 0
    while offset + 2400 <= len(payload):
        actions = parse_trigger_actions(payload[offset : offset + 2400])
        if actions:
            yield actions
            offset += 2400
        else:
            offset += 4


def location_center(rect):
    if not rect or None in rect:
        return None
    left, top, right, bottom = rect
    return [(left + right) / 2.0, (top + bottom) / 2.0]


def read_payload_obstacles(strx):
    """로케이션에 상수 좌표를 쓴 뒤 같은 트리거에서 실행되는 CreateUnit/Order를 모은다."""
    obstacles, paths = [], []
    seen_obstacles, seen_paths = set(), set()
    for actions in scan_payload_triggers(strx[string_data_end(strx) :]):
        locations = {}  # 로케이션 번호(1부터) -> [left, top, right, bottom], 이 트리거에서 상수로 정해진 값만
        for action, group, number, unit_id, modifier, location in actions:
            if action == ACTION_SET_DEATHS and group > 26:
                address = (DEATHS_TABLE + 4 * (group + 12 * unit_id)) & MASK
                if MRGN_TABLE <= address < MRGN_TABLE + LOCATION_COUNT * 20:
                    index = (address - MRGN_TABLE) // 20 + 1
                    field = (address - MRGN_TABLE) % 20 // 4
                    if field < 4:
                        rect = locations.setdefault(index, [None] * 4)
                        rect[field] = number if modifier == MODIFIER_SET_TO else None  # Add/Subtract는 알 수 없음
            elif action == ACTION_CREATE_UNIT and group == OBSTACLE_PLAYER:
                center = location_center(locations.get(location))
                key = (unit_id, tuple(center or ()))
                if center and key not in seen_obstacles:
                    seen_obstacles.add(key)
                    obstacles.append({
                        "Unit": CREATED_UNITS.get(unit_id, "Unit%d" % unit_id),
                        "UnitId": unit_id,
                        "Pixel": center,
                        "Tile": [int(center[0] // 32), int(center[1] // 32)],
                    })
            elif action == ACTION_ORDER:
                start = location_center(locations.get(location))
                end = location_center(locations.get(number))
                key = (unit_id, tuple(start or ()), tuple(end or ()))
                if start and end and key not in seen_paths:
                    seen_paths.add(key)
                    paths.append({
                        "Unit": CREATED_UNITS.get(unit_id, "Unit%d" % unit_id),
                        "UnitId": unit_id,
                        "From": start,
                        "To": end,
                    })
    return obstacles, paths


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    with open(sys.argv[1], "rb") as f:
        sections = read_sections(read_chk(f.read()))

    tileset = struct.unpack("<H", sections["ERA "])[0]
    width, height = struct.unpack("<HH", sections["DIM "])
    tiles = struct.unpack("<%dH" % (width * height), sections["MTXM"][: width * height * 2])
    rows = [
        "".join(TERRAIN_CHARS.get(tiles[y * width + x] >> 5, "?") for x in range(width))
        for y in range(STAGE_ROWS)
    ]

    flags, markers = [], []
    unit_data = sections["UNIT"]
    for i in range(len(unit_data) // 36):
        _, px, py, unit_id = struct.unpack("<IHHH", unit_data[i * 36 : i * 36 + 10])
        tile = [px // 32, py // 32]
        if tile[1] >= STAGE_ROWS:
            continue
        if unit_id == UNIT_FLAG:
            flags.append(tile)
        elif unit_id in MARKER_UNITS:
            markers.append({"Unit": MARKER_UNITS[unit_id], "Tile": tile})

    obstacles, move_paths = read_payload_obstacles(sections["STRx"])

    result = {
        "Source": "저글링의빙판탈출 Test16",
        "Tileset": tileset,
        "MapSize": [width, height],
        "Legend": {
            " ": "group 4/5 (background)",
            "-": "group 2/3",
            "3": "group 6/7",
            "4": "group 8/9",
            "7": "group 14/15",
            "C": "group 24/25",
            "D": "group 26/27",
        },
        "Rows": rows,
        "Flags": sorted(flags),
        "Markers": markers,
        "Obstacles": obstacles,  # Player 8 CreateUnit, 픽셀 좌표(1타일 = 32)
        "MovePaths": move_paths,  # Order(Move) 출발/도착. 왕복 이동 구현용으로 기록만 한다.
    }
    with open(sys.argv[2], "w", encoding="utf-8", newline="\n") as f:
        json.dump(result, f, ensure_ascii=False, indent=1)
    print("rows %d, flags %d, markers %d, obstacles %d, move paths %d -> %s" % (
        len(rows), len(flags), len(markers), len(obstacles), len(move_paths), sys.argv[2]))


if __name__ == "__main__":
    main()
