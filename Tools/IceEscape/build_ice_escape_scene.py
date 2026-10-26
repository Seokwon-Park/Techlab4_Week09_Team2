"""original_map.json의 19개 스테이지를 Scenes/IceEscape.scene으로 변환한다.

사용법: python -I build_ice_escape_scene.py [--preview]

- 스테이지 N은 월드 Y = (N-1) * 1000에 두고, 시작 블록 중심이 스테이지 원점이다. (stage_goal.lua 규칙)
- 원본 타일 1칸 = TILE 유닛. 화면 위쪽이 +X, 오른쪽이 +Y이므로 원본 (tx, ty)는 X = -ty, Y = +tx 방향이다.
- 배경 타일은 벽, 그 외 지형은 TERRAIN_SURFACE에 따라 일반 바닥 또는 빙판(UIceVolumeComponent)이 된다.
- 점프는 구현하지 않는다. 원본에서 점프로 건너는 간극은 STAGES의 bridges로 일반 바닥을 깐다.
- 맵 편집이 쉽도록 바닥/빙판 조각마다 액터 하나를 만들고 루트를 월드 좌표에 둔다.
  조각 크기는 루트 Scale이라 Scale만 바꾸면 바닥과 빙판 판정이 함께 늘어난다. 벽만 스테이지당 액터 하나로 묶는다.
- 원본 장애물(Obstacles)과 이동 경로(MovePaths)로 장애물을 둔다. 경로가 있으면 UPatrolMovementComponent로
  스크립트 없이 왕복/순환하고, 경로가 없으면 정지 장애물이다. 시작 블록을 덮거나 길을 끊는 것은 빼고 출력한다.
- 스크립트는 플레이어, 카메라, 골에만 붙인다.
"""
import json
import os
import sys
from collections import deque

HERE = os.path.dirname(os.path.abspath(__file__))
MAP_PATH = os.path.join(HERE, "original_map.json")
SCENE_PATH = os.path.join(HERE, "..", "..", "Scenes", "IceEscape.scene")

TILE = 1.5
STAGE_SPACING = 1000.0
BACKGROUND = " "
START_GOAL = "7"
ICE_TYPES = ("default", "slippery", "reverse")

# 원본 지형 문자 -> 바닥 종류. "floor"는 빙판이 아닌 일반 바닥이고, 나머지는 UIceVolumeComponent.IceType 값이다.
# 문자 의미는 original_map.json의 Legend 참고. 시작/골 블록은 이 표와 관계없이 항상 일반 바닥이다.
# 원본 트리거가 난독화되어 있어 아래 값은 트랙 모양으로 추정했다. slippery는 클릭 입력을 막으므로
# 곡선(C)이나 직각 미로(3)에는 쓰지 않고, 직선 구간과 간극 앞 블록뿐인 D에만 쓴다.
TERRAIN_SURFACE = {
    "-": "default",   # 그룹 2/3: 기본 빙판. 스테이지 1~2는 이것만 쓴다.
    "C": "default",   # 그룹 24/25: 곡선 트랙. 계속 조향해야 한다.
    "3": "reverse",   # 그룹 6/7: 직각으로 꺾이는 빗살 미로.
    "D": "slippery",  # 그룹 26/27: 직선 구간, 간극 앞 2×2 블록.
    "4": "floor",     # 그룹 8/9: 모서리/막다른 곳의 휴식 블록.
    "7": "floor",     # 그룹 14/15: 시작/골 블록과 같은 지형(줄무늬, 체크무늬).
}

# (시작 깃발 타일, 골 타일, 다른 덩어리를 잇는 추가 seed 타일, 간극을 메울 사각형 (x0, y0, x1, y1) 포함 범위)
# 순서가 곧 스테이지 번호다. 1~8은 상단 체인으로, 다음 깃발이 골이다.
STAGES = [
    ((18, 10), (46, 14), [], []),
    ((46, 14), (78, 18), [], []),
    ((78, 18), (94, 22), [], []),
    ((94, 22), (130, 14), [], []),
    ((130, 14), (146, 22), [], []),
    ((146, 22), (186, 10), [], []),
    ((186, 10), (194, 18), [], []),
    ((194, 18), (164, 18), [(182, 18)], [(186, 17, 189, 18), (166, 17, 177, 18)]),
    ((18, 30), (34, 61), [(17, 58)], [(24, 49, 27, 50)]),
    ((62, 30), (90, 30), [], []),
    ((112, 38), (154, 38), [], []),
    ((174, 36), (202, 44), [], []),
    ((222, 36), (222, 49), [], []),
    ((18, 78), (44, 88), [(20, 82)], [(33, 75, 34, 75), (33, 80, 34, 80)]),
    ((62, 73), (86, 70), [], []),
    ((106, 70), (131, 75), [], [(129, 68, 130, 68), (128, 69, 130, 72)]),
    ((158, 58), (196, 65), [], []),
    ((218, 60), (220, 68), [], []),
    ((150, 90), (169, 105), [], []),
]

BRIDGE = "="  # 간극을 메운 타일. 일반 바닥으로 만든다.

# 바닥 색은 IceType별로 하나씩: default 파랑, slippery 흰색, reverse 보라, 일반 바닥 회색, 시작/골 진회색.
COLORS = {
    "start": (0.17, 0.18, 0.20),
    "floor": (0.46, 0.47, 0.49),
    "default": (0.15, 0.45, 0.95),
    "slippery": (0.95, 0.97, 1.00),
    "reverse": (0.55, 0.30, 0.85),
    "wall": (0.16, 0.30, 0.40),
    "cap": (0.69, 0.91, 0.96),
}
ICE_SHAPE_COLOR = 4294950500
PLACEMENT_SHAPE_COLOR = 4286105710

PLAYER_SCRIPT = "Scripts/ice_player.lua"
CAMERA_SCRIPT = "Scripts/IceSlide_PlayerCamera.lua"
GOAL_SCRIPT = "Scripts/ice_escape_goal.lua"

PLAYER_RADIUS = 0.5
OBSTACLE_MESH = "Models/baneling/baneling.obj"
OBSTACLE_MESH_LENGTH = 0.895  # 엔진 축으로 변환한 baneling.obj의 가장 긴 수평 길이(X)
OBSTACLE_MESH_HEIGHT = 0.57
OBSTACLE_COLOR = (0.65, 0.16, 0.20)
MARKER_COLOR = (0.96, 0.74, 0.20)
# 원본 유닛 -> (X 길이, Y 길이) 타일 단위. 원본 units.dat 크기를 32픽셀 타일로 어림잡았다.
OBSTACLE_SIZES = {
    "Zealot": (0.7, 0.7),
    "HighTemplar": (0.7, 0.7),
    "SCV": (0.7, 0.7),
    "Dragoon": (1.0, 1.0),
    "Vulture": (1.0, 1.0),
    "TassadarZeratulArchon": (1.0, 1.0),
    "Mojo": (1.1, 1.1),
    "InfestedCommandCenter": (2.6, 3.6),
}
# 스타에서 비콘은 유닛이 밟고 지나가는 표식이라 충돌 없는 마커로 둔다.
MARKER_UNITS = ("ZergFlagBeacon", "TerranFlagBeacon")
BUILDING_UNITS = ("InfestedCommandCenter",)
# 원본 유닛 이동 속도(월드 유닛/초). 원본 최고 속도(픽셀/프레임) × 24프레임 ÷ 32 × TILE로 어림잡았다.
# 같은 방식으로 저글링은 약 6.2라 플레이어 MoveSpeed 6과 비율이 맞는다.
UNIT_SPEEDS = {
    "Zealot": 4.5,
    "Dragoon": 5.6,
    "HighTemplar": 3.6,
    "SCV": 5.5,
    "Vulture": 7.2,
    "TassadarZeratulArchon": 5.5,
    "Mojo": 5.6,
}
PATH_MATCH_PIXELS = 48  # 정지 장애물 생성 위치가 이 거리 안의 경로 지점이면 그 장애물이 경로를 따라 움직인다.
PATH_STAGE_REACH = 4  # 경로 지점이 스테이지 영역 밖이면 이 타일 수 안의 스테이지에 배정한다(공중 유닛은 벽 위로 다닌다).


# ------------------------------------------------------------------ 타일 영역
class TileMap:
    def __init__(self, rows):
        self.rows = rows
        self.height = len(rows)
        self.width = len(rows[0])

    def at(self, x, y):
        if 0 <= x < self.width and 0 <= y < self.height:
            return self.rows[y][x]
        return BACKGROUND

    def find_block(self, seed, size=4):
        """seed를 포함하는 size×size 크기의 '7' 창. 붙어 있는 '7' 줄무늬가 블록에 섞이지 않게 한다."""
        sx, sy = seed
        windows = [
            (x0, y0)
            for y0 in range(sy - size + 1, sy + 1)
            for x0 in range(sx - size + 1, sx + 1)
            if all(self.at(x, y) == START_GOAL for y in range(y0, y0 + size) for x in range(x0, x0 + size))
        ]
        if not windows:
            raise ValueError("no %dx%d start/goal block at %s" % (size, size, seed))
        # seed가 창 중심에 가장 가까운 것을 고른다(깃발은 블록 중앙 칸에 놓여 있다).
        x0, y0 = min(windows, key=lambda w: abs(w[0] + size / 2.0 - sx - 0.5) + abs(w[1] + size / 2.0 - sy - 0.5))
        return {(x, y) for y in range(y0, y0 + size) for x in range(x0, x0 + size)}

    def flood(self, seed, accept, blocked=frozenset()):
        seen = {seed}
        stack = [seed]
        while stack:
            x, y = stack.pop()
            for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                tile = (nx, ny)
                if tile not in seen and tile not in blocked and accept(self.at(nx, ny)):
                    seen.add(tile)
                    stack.append(tile)
        return seen


def connected(tiles, start, goal):
    queue = deque(start)
    seen = set(start)
    while queue:
        x, y = queue.popleft()
        if (x, y) in goal:
            return True
        for tile in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if tile in tiles and tile not in seen:
                seen.add(tile)
                queue.append(tile)
    return False


def build_stage_regions(tile_map):
    """스테이지마다 {타일: 지형 문자}와 시작/골 블록을 만든다. 앞 스테이지가 차지한 타일은 다시 쓰지 않는다."""
    is_track = lambda c: c != BACKGROUND
    claimed = set()
    stages = []
    for number, (start, goal, seeds, bridges) in enumerate(STAGES, 1):
        start_block = tile_map.find_block(start)
        goal_block = tile_map.find_block(goal)
        tiles = set()
        for seed in [start] + seeds:
            tiles |= tile_map.flood(seed, is_track, blocked=goal_block)
        tiles = (tiles - claimed) | start_block | goal_block
        region = {tile: tile_map.at(*tile) for tile in tiles}
        for x0, y0, x1, y1 in bridges:
            for y in range(y0, y1 + 1):
                for x in range(x0, x1 + 1):
                    if tile_map.at(x, y) != BACKGROUND:
                        raise ValueError("stage %d bridge covers non-background tile %s" % (number, (x, y)))
                    region[(x, y)] = BRIDGE
        if not connected(region.keys(), start_block, goal_block):
            raise ValueError("stage %d: start and goal are not connected" % number)
        claimed |= tiles
        stages.append({"Number": number, "Region": region, "Start": start_block, "Goal": goal_block})
    return stages


def merge_rects(tiles, label_of):
    """같은 라벨의 타일을 행 단위 run -> 아래로 확장하는 그리디로 사각형 (x0, y0, x1, y1, label)에 묶는다."""
    done = set()
    rects = []
    for x, y in sorted(tiles, key=lambda t: (t[1], t[0])):
        if (x, y) in done:
            continue
        label = label_of((x, y))
        x1 = x
        while (x1 + 1, y) in tiles and (x1 + 1, y) not in done and label_of((x1 + 1, y)) == label:
            x1 += 1
        y1 = y
        while all(
            (cx, y1 + 1) in tiles and (cx, y1 + 1) not in done and label_of((cx, y1 + 1)) == label
            for cx in range(x, x1 + 1)
        ):
            y1 += 1
        for cy in range(y, y1 + 1):
            for cx in range(x, x1 + 1):
                done.add((cx, cy))
        rects.append((x, y, x1, y1, label))
    return rects


def block_center(block):
    xs = [x for x, _ in block]
    ys = [y for _, y in block]
    return (min(xs) + max(xs) + 1) / 2.0, (min(ys) + max(ys) + 1) / 2.0


def block_size(block):
    xs = [x for x, _ in block]
    ys = [y for _, y in block]
    return max(xs) - min(xs) + 1, max(ys) - min(ys) + 1


# ------------------------------------------------------------------ 씬 JSON
def r4(value):
    return round(value, 4) + 0.0


def transform(location=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0), pitch=0.0, yaw=0.0):
    return {
        "Location": {"X": r4(location[0]), "Y": r4(location[1]), "Z": r4(location[2])},
        "Rotation": {"Pitch": pitch, "Roll": 0.0, "Yaw": yaw},
        "Scale": {"X": r4(scale[0]), "Y": r4(scale[1]), "Z": r4(scale[2])},
    }


def material(color, shininess=24.0, specular=(0.18, 0.22, 0.25)):
    return [{
        "Base": "DefaultMaterial",
        "Instance": {
            "BaseColor": {"W": 1.0, "X": color[0], "Y": color[1], "Z": color[2]},
            "BlendState": "Opaque",
            "FlipNormalY": False,
            "PSOType": 0,
            "Properties": {},
            "SamplerState": "LinearClamp",
            "Shininess": shininess,
            "SpecularColor": {"X": specular[0], "Y": specular[1], "Z": specular[2]},
            "Textures": ["WhiteTexture", "FlatNormalTexture"],
            "UVScrollSpeed": [0.0, 0.0],
        },
    }]


def scene_root(location=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0)):
    return {
        "Class": "USceneComponent",
        "Name": "DefaultSceneRoot",
        "Parent": "",
        "Properties": {
            "Transform": transform(location, scale),
            "bAutoActivate": True,
            "bHiddenInGame": False,
            "bVisible": True,
        },
    }


def mesh(name, mesh_path, location, scale, color, block, shininess=24.0, specular=(0.18, 0.22, 0.25), shadow=True):
    return {
        "Class": "UStaticMeshComponent",
        "Name": name,
        "OverrideMaterials": material(color, shininess, specular),
        "Parent": "DefaultSceneRoot",
        "Properties": {
            "StaticMesh": mesh_path,
            "Transform": transform(location, scale),
            "bAutoActivate": True,
            "bBlockComponent": block,
            "bCastShadow": shadow,
            "bGenerateOverlapEvents": False,
            "bHiddenInGame": False,
            "bReceiveShadow": True,
            "bVisible": True,
        },
    }


def box(cls, name, location, extent, shape_color, extra=None):
    properties = {
        "BoxExtent": {"X": r4(extent[0]), "Y": r4(extent[1]), "Z": r4(extent[2])},
        "ShapeColor": shape_color,
        "Transform": transform(location),
        "bAutoActivate": True,
        "bBlockComponent": False,
        "bCastShadow": False,
        "bDrawOnlyIfSelected": False,
        "bGenerateOverlapEvents": True,
        "bHiddenInGame": True,
        "bReceiveShadow": False,
        "bVisible": False,
    }
    properties.update(extra or {})
    return {"Class": cls, "Name": name, "Parent": "DefaultSceneRoot", "Properties": properties}


def text(name, value):
    return {
        "Class": "UTextRenderComponent",
        "Name": name,
        "Parent": "DefaultSceneRoot",
        "Properties": {
            "Font": "Assets/Fonts/CookieRun.json",
            "Text": value,
            "TextSize": 0.75,
            "Transform": transform((0.0, -1.56, 2.2), (0.78, 1.0, 1.0), yaw=30.0),
            "bAutoActivate": True,
            "bBlockComponent": False,
            "bCastShadow": True,
            "bGenerateOverlapEvents": False,
            "bHiddenInGame": False,
            "bReceiveShadow": True,
            "bVisible": True,
        },
    }


def actor(name, components, cls="AActor", collision=True, script=""):
    return {
        "Class": cls,
        "Components": components,
        "Name": name,
        "Properties": {"ScriptPath": script, "bActorEnableCollision": collision},
    }


class StageFrame:
    """원본 타일 좌표(연속값) -> 월드 좌표. 시작 블록 중심이 (0, (N-1) * 1000)이다."""

    def __init__(self, stage):
        self.origin = block_center(stage["Start"])
        self.offset_y = (stage["Number"] - 1) * STAGE_SPACING

    def point(self, tx, ty):
        return -(ty - self.origin[1]) * TILE, (tx - self.origin[0]) * TILE + self.offset_y

    def rect(self, rect):
        """타일 사각형 -> (중심 X, 중심 Y, X 길이, Y 길이). 원본 행은 월드 X, 열은 월드 Y."""
        x0, y0, x1, y1 = rect[:4]
        cx, cy = self.point((x0 + x1 + 1) / 2.0, (y0 + y1 + 1) / 2.0)
        return cx, cy, (y1 - y0 + 1) * TILE, (x1 - x0 + 1) * TILE


def surface_of(stage, tile, surface_map):
    terrain = stage["Region"][tile]
    if terrain == BRIDGE:
        return "floor"
    return surface_map[terrain]


def piece_actor(name, cx, cy, sx, sy, kind):
    """바닥/빙판 조각 하나. 루트 Scale이 조각 크기라 자식은 단위 크기로 둔다."""
    ice = kind in ICE_TYPES
    components = [
        scene_root((cx, cy, 0.0), (sx, sy, 1.0)),
        mesh(
            "Floor", "Cube", (0.0, 0.0, -0.35), (1.0, 1.0, 0.7), COLORS[kind], False,
            shininess=96.0 if ice else 24.0,
            specular=(0.65, 0.80, 0.90) if ice else (0.18, 0.22, 0.25),
        ),
    ]
    if ice:
        components.append(box(
            "UIceVolumeComponent", "IceVolume", (0.0, 0.0, 1.0), (0.5, 0.5, 1.5), ICE_SHAPE_COLOR, {"IceType": kind},
        ))
    return actor(name, components)


def block_actor(name, frame, block, prefix, label, number, script=""):
    """시작/골 블록. 바닥, 판정 볼륨, 안내 텍스트를 한 액터에 둔다."""
    bx, by = block_center(block)
    w, h = block_size(block)
    cx, cy = frame.point(bx, by)
    sx, sy = h * TILE, w * TILE
    return actor(name, [
        scene_root((cx, cy, 0.0)),
        mesh("Floor", "Cube", (0.0, 0.0, -0.35), (sx, sy, 0.7), COLORS["start"], False),
        box("UBoxComponent", "%s_PlacementVolume" % prefix, (0.0, 0.0, 0.85), (sx / 2.0, sy / 2.0, 1.1),
            PLACEMENT_SHAPE_COLOR),
        text("Stage%sText" % label, "STAGE %02d / %s" % (number, label.upper())),
    ], script=script)


def obstacle_half_extent(unit):
    """장애물의 월드 AABB 반지름 (X, Y). 충돌은 블로킹 컴포넌트 AABB로 계산된다(UWorld::SweepSingle)."""
    length_x, length_y = (value * TILE for value in OBSTACLE_SIZES.get(unit, (1.0, 1.0)))
    if unit in BUILDING_UNITS or unit in MARKER_UNITS:
        return length_x / 2.0, length_y / 2.0
    scale = max(length_x, length_y) / OBSTACLE_MESH_LENGTH
    return 0.4475 * scale, 0.328 * scale  # baneling.obj 엔진 축 AABB: X -0.353~0.542, Y -0.328~0.328


def patrol_component(offsets, loop, speed):
    """UPatrolMovementComponent. offsets는 시작 위치 기준 월드 오프셋(최대 3개)."""
    properties = {
        "PatrolPointCount": len(offsets),
        "Speed": speed,
        "bLoop": loop,
        "bFaceMoveDirection": True,
        "bAutoActivate": True,
        "bHiddenInGame": False,
        "bVisible": True,
    }
    for index in range(3):
        x, y = offsets[index] if index < len(offsets) else (0.0, 0.0)
        properties["PatrolPoint%d" % (index + 1)] = {"X": r4(x), "Y": r4(y), "Z": 0.0}
    return {"Class": "UPatrolMovementComponent", "Name": "PatrolMovement", "Parent": "", "Properties": properties}


def obstacle_actor(name, unit, cx, cy, patrol=None):
    if unit in MARKER_UNITS:
        size = 0.9 * TILE
        components = [scene_root((cx, cy, 0.0)),
                      mesh("Marker", "Cube", (0.0, 0.0, 0.03), (size, size, 0.06), MARKER_COLOR, False, shadow=False)]
        return actor(name, components, collision=False)
    half_x, half_y = obstacle_half_extent(unit)
    if unit in BUILDING_UNITS:
        body = mesh("Body", "Cube", (0.0, 0.0, 0.75), (half_x * 2.0, half_y * 2.0, 1.5), OBSTACLE_COLOR, True)
    else:
        scale = half_x / 0.4475
        # 메시 원점이 앞쪽으로 치우쳐 있어 AABB 중심이 원본 위치에 오도록 X를 당긴다.
        body = mesh("Body", OBSTACLE_MESH, (-0.0945 * scale, 0.0, 0.0), (scale, scale, scale), OBSTACLE_COLOR, True)
    components = [scene_root((cx, cy, 0.0)), body]
    if patrol:
        offsets, loop = patrol
        components.append(patrol_component(offsets, loop, UNIT_SPEEDS.get(unit, 4.0)))
    return actor(name, components)


def build_patrol_routes(move_paths):
    """원본 Order(Move) 출발→도착 쌍을 유닛별 경로로 묶는다.
    모든 지점이 들어오고 나가는 화살표를 하나씩 가지면 순환(사각형 순찰), 아니면 끝에서 되돌아오는 왕복이다."""
    edges = {}
    for path in move_paths:
        start = (path["Unit"], tuple(path["From"]))
        end = (path["Unit"], tuple(path["To"]))
        edges.setdefault(start, set()).add(end)
    neighbors = {}
    for start, ends in edges.items():
        for end in ends:
            neighbors.setdefault(start, set()).add(end)
            neighbors.setdefault(end, set()).add(start)

    routes, seen = [], set()
    for node in sorted(neighbors):
        if node in seen:
            continue
        group, stack = {node}, [node]
        while stack:
            for other in neighbors[stack.pop()]:
                if other not in group:
                    group.add(other)
                    stack.append(other)
        seen |= group
        indegree = {n: sum(n in edges.get(m, ()) for m in group) for n in group}
        loop = all(len(edges.get(n, ())) == 1 and indegree[n] == 1 for n in group)
        current = min(group) if loop else min(n for n in group if indegree[n] == 0) if any(
            indegree[n] == 0 for n in group) else min(group)
        order = [current]
        while True:
            following = sorted(edges.get(current, set()) - set(order))
            if not following:
                break
            current = following[0]
            order.append(current)
        routes.append({"Unit": node[0], "Points": [list(point) for _, point in order[:4]], "Loop": loop})
    return routes


def route_samples(route, step=8.0):
    """경로를 따라 step 픽셀마다 지점을 뽑는다. 순환이면 마지막에서 처음으로 가는 구간도 포함한다."""
    points = route["Points"] + (route["Points"][:1] if route["Loop"] else [])
    samples = [tuple(points[0])]
    for (x0, y0), (x1, y1) in zip(points, points[1:]):
        count = max(1, int(max(abs(x1 - x0), abs(y1 - y0)) / step))
        samples += [(x0 + (x1 - x0) * i / count, y0 + (y1 - y0) * i / count) for i in range(1, count + 1)]
    return samples


def tiles_blocked_by(unit, px, py):
    """장애물 AABB + 플레이어 반경이 타일 중심을 덮는 타일들. 원본 픽셀 좌표 기준."""
    half_x, half_y = obstacle_half_extent(unit)
    ox, oy = px / 32.0, py / 32.0
    reach_cols = (half_y + PLAYER_RADIUS) / TILE  # 월드 Y = 원본 열
    reach_rows = (half_x + PLAYER_RADIUS) / TILE  # 월드 X = 원본 행
    return {
        (tx, ty)
        for ty in range(int(oy - reach_rows) - 1, int(oy + reach_rows) + 2)
        for tx in range(int(ox - reach_cols) - 1, int(ox + reach_cols) + 2)
        if abs(tx + 0.5 - ox) <= reach_cols and abs(ty + 0.5 - oy) <= reach_rows
    }


def assign_obstacles(stages, obstacles, move_paths):
    """원본 장애물을 스테이지에 배정한다.
    이동 경로가 있는 장애물은 경로를 따라 움직이고, 생성 위치를 못 찾은 경로도 경로 첫 지점에 장애물을 만든다.
    시작 블록을 덮거나(이동 장애물은 경로 전체 기준), 정지 장애물이 길을 끊으면 빼고 사유를 남긴다."""
    def owner(tiles, reach=1):
        for tile in tiles:
            for stage in stages:
                if tile in stage["Region"]:
                    return stage
        best, best_distance = None, reach + 1
        for x, y in tiles:
            for stage in stages:
                for dx in range(-reach, reach + 1):
                    for dy in range(-reach, reach + 1):
                        if (x + dx, y + dy) in stage["Region"] and max(abs(dx), abs(dy)) < best_distance:
                            best, best_distance = stage, max(abs(dx), abs(dy))
        return best

    placed = {stage["Number"]: [] for stage in stages}
    blocked = {stage["Number"]: set() for stage in stages}
    skipped = []

    statics = list(obstacles)
    for route in build_patrol_routes(move_paths):
        unit, points = route["Unit"], route["Points"]
        # 같은 유닛의 정지 장애물이 경로 지점 근처에서 만들어졌다면 그 지점에서 출발시킨다.
        candidates = [
            (abs(o["Pixel"][0] - p[0]) + abs(o["Pixel"][1] - p[1]), i, o)
            for o in statics if o["Unit"] == unit
            for i, p in enumerate(points)
            if abs(o["Pixel"][0] - p[0]) <= PATH_MATCH_PIXELS and abs(o["Pixel"][1] - p[1]) <= PATH_MATCH_PIXELS
        ]
        if candidates:
            _, index, matched = min(candidates, key=lambda c: c[0])
            statics.remove(matched)
            if route["Loop"]:
                points = points[index:] + points[:index]
            elif index == len(points) - 1:
                points = points[::-1]
        route = {"Unit": unit, "Points": points, "Loop": route["Loop"]}
        stage = owner([(int(x // 32), int(y // 32)) for x, y in points], PATH_STAGE_REACH)
        if stage is None:
            skipped.append((unit, points[0], "route outside stages"))
            continue
        swept = set()
        for x, y in route_samples(route):
            swept |= tiles_blocked_by(unit, x, y)
        if swept & stage["Start"]:
            skipped.append((unit, points[0], "route crosses stage %d start" % stage["Number"]))
            continue
        placed[stage["Number"]].append({
            "Unit": unit, "Pixel": points[0], "Tile": [int(points[0][0] // 32), int(points[0][1] // 32)], "Route": route,
        })

    for obstacle in statics:
        unit = obstacle["Unit"]
        px, py = obstacle["Pixel"]
        stage = owner([tuple(obstacle["Tile"])])
        if stage is None:
            skipped.append((unit, obstacle["Tile"], "outside stages"))
            continue
        if unit not in MARKER_UNITS:
            covered = tiles_blocked_by(unit, px, py)
            if covered & (stage["Start"] | stage["Goal"]):
                skipped.append((unit, obstacle["Tile"], "covers stage %d start/goal" % stage["Number"]))
                continue
            walkable = set(stage["Region"]) - blocked[stage["Number"]] - covered
            if not connected(walkable, stage["Start"], stage["Goal"]):
                skipped.append((unit, obstacle["Tile"], "blocks stage %d path" % stage["Number"]))
                continue
            blocked[stage["Number"]] |= covered
        placed[stage["Number"]].append(obstacle)
    return placed, skipped


def build_stage_actors(stage, surface_map, obstacles):
    number = stage["Number"]
    region = stage["Region"]
    frame = StageFrame(stage)
    pieces = set(region) - stage["Start"] - stage["Goal"]
    actors = [
        block_actor("Stage%d_Start" % number, frame, stage["Start"], "start", "Start", number),
        block_actor("Stage%d_Goal" % number, frame, stage["Goal"], "goal", "Goal", number, script=GOAL_SCRIPT),
    ]

    counters = {}
    for rect in merge_rects(pieces, lambda tile: surface_of(stage, tile, surface_map)):
        kind = rect[4]
        label = "Ice_%s" % kind if kind in ICE_TYPES else "Floor"
        counters[label] = counters.get(label, 0) + 1
        cx, cy, sx, sy = frame.rect(rect)
        actors.append(piece_actor("Stage%d_%s_%02d" % (number, label, counters[label]), cx, cy, sx, sy, kind))

    xs = [x for x, _ in region]
    ys = [y for _, y in region]
    wall_tiles = {
        (x, y)
        for y in range(min(ys) - 1, max(ys) + 2)
        for x in range(min(xs) - 1, max(xs) + 2)
        if (x, y) not in region
    }
    # 루트가 월드 원점이라 자식 벽의 상대 좌표가 곧 월드 좌표다.
    walls = [scene_root()]
    for index, rect in enumerate(merge_rects(wall_tiles, lambda _: "wall"), 1):
        cx, cy, sx, sy = frame.rect(rect)
        walls.append(mesh("Wall_%d" % index, "Cube", (cx, cy, 0.5), (sx, sy, 1.0), COLORS["wall"], True))
        walls.append(mesh("FrostCap_%d" % index, "Cube", (cx, cy, 1.03), (sx, sy, 0.06), COLORS["cap"], False))
    actors.append(actor("Stage%d_Walls" % number, walls))

    for index, obstacle in enumerate(obstacles, 1):
        unit = obstacle["Unit"]
        kind = "Marker" if unit in MARKER_UNITS else "Obstacle"
        cx, cy = frame.point(obstacle["Pixel"][0] / 32.0, obstacle["Pixel"][1] / 32.0)
        patrol = None
        route = obstacle.get("Route")
        if route:
            # 경로 지점을 출발 지점 기준 월드 오프셋으로 바꾼다. 원본 행은 -X, 열은 +Y 방향이다.
            x0, y0 = route["Points"][0]
            offsets = [(-(y - y0) / 32.0 * TILE, (x - x0) / 32.0 * TILE) for x, y in route["Points"][1:]]
            patrol = (offsets, route["Loop"])
        actors.append(obstacle_actor("Stage%d_%s_%02d_%s" % (number, kind, index, unit), unit, cx, cy, patrol))
    return actors


def common_actors():
    player = actor("IcePlayer", [
        scene_root((0.0, 0.0, 0.5)),
        {
            "Class": "USphereComponent",
            "Name": "PlayerCollider",
            "Parent": "DefaultSceneRoot",
            "Properties": {
                "ShapeColor": PLACEMENT_SHAPE_COLOR,
                "SphereRadius": 0.5,
                "Transform": transform(),
                "bAutoActivate": True,
                "bBlockComponent": False,
                "bCastShadow": False,
                "bDrawOnlyIfSelected": False,
                "bGenerateOverlapEvents": True,
                "bHiddenInGame": False,
                "bReceiveShadow": False,
                "bVisible": True,
            },
        },
        {
            "Class": "UStaticMeshComponent",
            "Name": "PlayerMesh",
            "OverrideMaterials": [],
            "Parent": "DefaultSceneRoot",
            "Properties": {
                "StaticMesh": "Models/Shiba/Shiba.obj",
                "Transform": transform(scale=(0.5, 0.5, 0.5), yaw=180.0),
                "bAutoActivate": True,
                "bBlockComponent": False,
                "bCastShadow": True,
                "bGenerateOverlapEvents": False,
                "bHiddenInGame": False,
                "bReceiveShadow": True,
                "bVisible": True,
            },
        },
        {
            "Class": "USlidingMovementComponent",
            "Name": "SlidingMovement",
            "Parent": "",
            "Properties": {
                "Acceleration": 100000000.0,
                "CollisionRadius": 0.5,
                "Deceleration": 10000000.0,
                "IceGrip": 5.0,
                "MoveSpeed": 6.0,
                "TurnRate": 360.0,
                "bAutoActivate": True,
                "bHiddenInGame": False,
                "bVisible": True,
            },
        },
    ], script=PLAYER_SCRIPT)
    camera_root = scene_root((-12.663397789001465, 0.0, 15.0))
    camera = actor("PlayerCamera", [
        camera_root,
        {
            "Class": "UCameraComponent",
            "Name": "UCameraComponent_0",
            "Parent": "DefaultSceneRoot",
            "Properties": {
                "Transform": transform(pitch=45.0),
                "bAutoActivate": False,
                "bHiddenInGame": False,
                "bVisible": True,
            },
        },
    ], cls="APawn", script=CAMERA_SCRIPT)

    def light(name, location, cls, component, properties):
        base = {
            "LightColor": 4294967295,
            "ShadowBias": 0.009999999776482582,
            "ShadowResolutionScale": 1.0,
            "ShadowSharpen": 0.0,
            "ShadowSlopeBias": 0.009999999776482582,
            "bAutoActivate": True,
            "bCastShadows": True,
            "bHiddenInGame": False,
            "bVisible": True,
        }
        base.update(properties)
        return actor(name, [
            scene_root(location),
            {"Class": cls, "Name": component, "Parent": "DefaultSceneRoot", "Properties": base},
        ], collision=False)

    sun = light("IceEscape_Sun", (0.0, 0.0, 20.0), "UDirectionalLightComponent", "Sunlight", {
        "AngularRadiusDegrees": 3.0,
        "Intensity": 1.2000000476837158,
        "Transform": transform(pitch=55.0, yaw=325.0),
    })
    sky = light("IceEscape_SkyFill", (0.0, 0.0, 15.0), "USkyLightComponent", "SkyFill", {
        "Intensity": 0.4000000059604645,
        "Transform": transform(),
    })
    return [player, camera, sun, sky]


def validate_surface_map(stages, surface_map):
    used = {terrain for stage in stages for terrain in stage["Region"].values()} - {BRIDGE}
    missing = sorted(used - set(surface_map))
    if missing:
        raise ValueError("TERRAIN_SURFACE has no entry for terrain %s" % missing)
    invalid = {k: v for k, v in surface_map.items() if v != "floor" and v not in ICE_TYPES}
    if invalid:
        raise ValueError("TERRAIN_SURFACE values must be 'floor' or one of %s: %s" % (ICE_TYPES, invalid))


def build(surface_map):
    with open(MAP_PATH, encoding="utf-8") as f:
        original = json.load(f)
    stages = build_stage_regions(TileMap(original["Rows"]))
    validate_surface_map(stages, surface_map)
    placed, skipped = assign_obstacles(stages, original.get("Obstacles", []), original.get("MovePaths", []))
    actors = common_actors()
    for stage in stages:
        actors += build_stage_actors(stage, surface_map, placed[stage["Number"]])
    scene = {"Version": 4, "World": {"PersistentLevel": {"Actors": actors, "Properties": {}}, "Properties": {}}}
    return scene, stages, placed, skipped


def preview(stage, obstacles=()):
    region = stage["Region"]
    marks = {}
    for obstacle in obstacles:
        if obstacle["Unit"] in MARKER_UNITS:
            marks[tuple(obstacle["Tile"])] = "m"
        elif obstacle.get("Route"):
            for x, y in obstacle["Route"]["Points"]:
                marks.setdefault((int(x // 32), int(y // 32)), "o")
            marks[tuple(obstacle["Tile"])] = "M"
        else:
            marks[tuple(obstacle["Tile"])] = "X"
    xs = [x for x, _ in region]
    ys = [y for _, y in region]
    lines = ["Stage %d  tiles x%d-%d y%d-%d" % (stage["Number"], min(xs), max(xs), min(ys), max(ys))]
    for y in range(min(ys) - 1, max(ys) + 2):
        row = ""
        for x in range(min(xs) - 1, max(xs) + 2):
            if (x, y) in marks:
                row += marks[(x, y)]
            elif (x, y) in stage["Start"]:
                row += "S"
            elif (x, y) in stage["Goal"]:
                row += "G"
            else:
                row += region.get((x, y), "#")
        lines.append(row)
    return "\n".join(lines)


def main():
    scene, stages, placed, skipped = build(TERRAIN_SURFACE)
    if "--preview" in sys.argv:
        for stage in stages:
            print(preview(stage, placed[stage["Number"]]) + "\n")
    for number, obstacles in placed.items():
        if obstacles:
            print("Stage %d obstacles: %s" % (number, ", ".join(
                o["Unit"] + ("(loop)" if o.get("Route", {}).get("Loop") else "(patrol)" if o.get("Route") else "")
                for o in obstacles)))
    for unit, tile, reason in skipped:
        print("skipped %s at tile %s: %s" % (unit, tile, reason))
    with open(SCENE_PATH, "w", encoding="utf-8", newline="\r\n") as f:
        json.dump(scene, f, indent=2, sort_keys=True)
        f.write("\n")
    actors = scene["World"]["PersistentLevel"]["Actors"]
    components = sum(len(a["Components"]) for a in actors)
    print("%d stages, %d actors, %d components -> %s" % (len(stages), len(actors), components, os.path.normpath(SCENE_PATH)))


if __name__ == "__main__":
    main()
