"""Print landscape transforms and bounds in /Game/Maps/Vysi (run via UnrealEditor-Cmd -run=pythonscript)."""
import unreal

world = unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Vysi")
actors = unreal.EditorLevelLibrary.get_all_level_actors()
for a in actors:
    cls = a.get_class().get_name()
    if "Landscape" in cls:
        origin, extent = a.get_actor_bounds(False)
        unreal.log_warning(f"FNCHECK {cls} {a.get_name()} loc={a.get_actor_location()} scale={a.get_actor_scale3d()} "
                           f"bounds_origin={origin} extent={extent} hidden={a.is_hidden_ed()}")
unreal.log_warning(f"FNCHECK total actors: {len(actors)}")
