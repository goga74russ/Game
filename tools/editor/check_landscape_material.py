import unreal
unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Vysi")
for a in unreal.EditorLevelLibrary.get_all_level_actors():
    if a.get_class().get_name() == "Landscape":
        unreal.log_warning(f"FNCHECK landscape_material={a.get_editor_property('landscape_material')}")
m = unreal.load_asset("/Game/Materials/Landscape/M_VysiLandscape")
unreal.log_warning(f"FNCHECK asset={m} usage_landscape={m.get_editor_property('used_with_landscape') if m else None}")
