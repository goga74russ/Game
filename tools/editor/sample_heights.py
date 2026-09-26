"""Line-trace the Vysi landscape at layout checkpoints and compare with the planned heights (metres)."""
import unreal

unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Vysi")
world = unreal.EditorLevelLibrary.get_editor_world()
points = {
    "village": (0, 0, 0), "oath": (160, 40, 18), "strel": (360, 10, 44), "exit3": (435, 0, 55),
    "bucket": (540, 20, 65), "treba": (645, 10, 78), "arena": (720, 0, 90), "arena_n": (735, 0, 90),
    "ridge": (800, 0, 104), "oak": (860, 0, 118),
    "probe_y+20@720": (720, 20, None), "probe_y-20@720": (720, -20, None), "probe_x700": (700, 0, None),
}
for name, (x, y, z) in points.items():
    start = unreal.Vector(x * 100.0, y * 100.0, 50000.0)
    end = unreal.Vector(x * 100.0, y * 100.0, -50000.0)
    hit = unreal.SystemLibrary.line_trace_single(world, start, end, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [],
                                                 unreal.DrawDebugTrace.NONE, True)
    hz = hit.to_tuple()[4].z / 100.0 if hit else None  # impact point
    unreal.log_warning(f"FNCHECK {name}: planned={z} terrain={hz}")
