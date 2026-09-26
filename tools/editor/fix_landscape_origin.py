"""Move the Vysi landscape so heightmap pixel (0,0) sits at world (-100 m, -504 m), matching the grey-box.

The import dialog treats Location as the landscape centre; the heightmap generator assumes a corner origin.
"""
import unreal

unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Vysi")
for a in unreal.EditorLevelLibrary.get_all_level_actors():
    if a.get_class().get_name() == "Landscape":
        a.modify()
        a.set_actor_location(unreal.Vector(-10000.0, -50400.0, 0.0), False, False)
        a.post_edit_change()
        origin, extent = a.get_actor_bounds(False)
        unreal.log_warning(f"FNCHECK moved: loc={a.get_actor_location()} bounds_origin={origin}")
ok = unreal.EditorLevelLibrary.save_current_level()
unreal.log_warning(f"FNCHECK saved={ok}")
