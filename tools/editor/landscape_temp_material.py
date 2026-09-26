"""Give the Vysi landscape a temporary engine material so it renders in game; report component count."""
import unreal

unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Vysi")
mat = unreal.load_asset("/Engine/EngineMaterials/WorldGridMaterial")
for a in unreal.EditorLevelLibrary.get_all_level_actors():
    if a.get_class().get_name() == "Landscape":
        comps = a.get_components_by_class(unreal.LandscapeComponent)
        unreal.log_warning(f"FNCHECK components={len(comps)} material_before={a.get_editor_property('landscape_material')}")
        a.set_editor_property("landscape_material", mat)
        unreal.log_warning(f"FNCHECK material_after={a.get_editor_property('landscape_material')}")
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log_warning("FNCHECK saved")
