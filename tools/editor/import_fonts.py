# Imports OFL fonts from Import/Fonts as FontFace assets into /Game/UI/Fonts (HUD v1).
import unreal, os
src = unreal.Paths.project_dir() + "Import/Fonts/"
tasks = []
for f in os.listdir(src):
    if not f.endswith(".ttf"):
        continue
    t = unreal.AssetImportTask()
    t.filename = src + f
    t.destination_path = "/Game/UI/Fonts"
    t.destination_name = "FF_" + f[:-4].replace("-", "_")
    t.automated = True; t.replace_existing = True; t.save = True
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for t in tasks:
    for p in t.imported_object_paths:
        a = unreal.load_asset(p)
        if isinstance(a, unreal.FontFace):
            a.set_editor_property("loading_policy", unreal.FontLoadingPolicy.INLINE)
            unreal.EditorAssetLibrary.save_loaded_asset(a)
        unreal.log_warning("IMPORTED %s %s" % (p, type(a).__name__))

unreal.SystemLibrary.quit_editor()
