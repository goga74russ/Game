"""Re-apply M_VysiLandscape to the Vysi landscape so per-component material instances are regenerated."""
import unreal

unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Vysi")
mat = unreal.load_asset("/Game/Materials/Landscape/M_VysiLandscape")
grid = unreal.load_asset("/Engine/EngineMaterials/WorldGridMaterial")
for a in unreal.EditorLevelLibrary.get_all_level_actors():
    if a.get_class().get_name() == "Landscape":
        a.modify()
        # Toggle through another material to force PostEditChange -> UpdateAllComponentMaterialInstances.
        a.set_editor_property("landscape_material", grid)
        a.post_edit_change()
        a.set_editor_property("landscape_material", mat)
        a.post_edit_change()
        comps = a.get_components_by_class(unreal.LandscapeComponent)
        mi = comps[0].get_material(0) if comps else None
        unreal.log_warning(f"FNCHECK component material now: {mi}")
unreal.log_warning(f"FNCHECK saved={unreal.EditorLevelLibrary.save_current_level()}")
