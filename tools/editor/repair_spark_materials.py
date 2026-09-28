"""Repair Aura Spark material wiring with the editor closed.

Run through Unreal's Python commandlet. Assets use locally installed Paragon
textures and stay in ignored Characters. A fresh backup is made before edits.
"""
from datetime import datetime
from pathlib import Path
import shutil
import unreal

content = Path(unreal.Paths.project_content_dir())
backup = Path(unreal.Paths.project_saved_dir()) / "Recovery" / ("Spark-materials-" + datetime.now().strftime("%Y%m%d-%H%M%S"))
backup.mkdir(parents=True, exist_ok=True)
for file in (content / "Characters/Spark").glob("*.uasset"):
    shutil.copy2(file, backup / file.name)
lib = unreal.MaterialEditingLibrary
mat = unreal.load_asset("/Game/Characters/Spark/M_SparkCore")
assert mat, "Missing local Spark material"
objects = {}
for prop in [unreal.MaterialProperty.MP_EMISSIVE_COLOR, unreal.MaterialProperty.MP_OPACITY]:
    custom = lib.get_material_property_input_node(mat, prop)
    names = lib.get_material_expression_input_names(custom)
    inputs = lib.get_inputs_for_material_expression(mat, custom)
    assert len(names) == len(inputs)
    for name, source in zip(names, inputs):
        if not name.endswith("Tex"):
            continue
        if isinstance(source, unreal.MaterialExpressionTextureObject):
            objects[source.get_editor_property("texture").get_path_name()] = source
            continue
        texture = source.get_editor_property("texture")
        assert texture
        key = texture.get_path_name()
        if key not in objects:
            node = lib.create_material_expression(mat, unreal.MaterialExpressionTextureObject, -800, -200 * len(objects))
            node.set_editor_property("texture", texture)
            objects[key] = node
        assert lib.connect_material_expressions(objects[key], "", custom, name)
lib.recompile_material(mat)
assert unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
mat = unreal.load_asset("/Game/Characters/Spark/M_SparkGlow")
custom = lib.get_material_property_input_node(mat, unreal.MaterialProperty.MP_OPACITY)
names = lib.get_material_expression_input_names(custom)
sources = lib.get_inputs_for_material_expression(mat, custom)
sample = sources[names.index("TexA")]
assert lib.connect_material_expressions(sample, "R", custom, "TexA")
lib.recompile_material(mat)
assert unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
unreal.log("SPARK_MATERIAL_REPAIR_COMPLETE backup=" + str(backup))
