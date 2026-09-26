"""Build M_VysiLandscape: slope-based auto material (dry grass on gentle ground, rock on steep), assign it to the Vysi landscape.

Textures: Fab Megascans surfaces (Medium 2K) — Grass Dried (pjvvL0), Rock Cliff (xccibbi).
Tiling uses world XY so it is independent of landscape component layout.
"""
import unreal

MEL = unreal.MaterialEditingLibrary
ASSETS = unreal.AssetToolsHelpers.get_asset_tools()
PKG, NAME = "/Game/Materials/Landscape", "M_VysiLandscape"
SURF = "/Game/Fab/Megascans/Surfaces"
GRASS = f"{SURF}/Grass_Dried_pjvvL0/Medium/pjvvL0_tier_2/Textures/T_pjvvL0_2K"
ROCK = f"{SURF}/Rock_Cliff_xccibbi/Medium/xccibbi_tier_2/Textures/T_xccibbi_2K"

path = f"{PKG}/{NAME}"
if unreal.EditorAssetLibrary.does_asset_exist(path):
    # Rebuild in place so the landscape's per-component instances keep their parent.
    mat = unreal.load_asset(path)
    MEL.delete_all_material_expressions(mat)
else:
    mat = ASSETS.create_asset(NAME, PKG, unreal.Material, unreal.MaterialFactoryNew())


def node(cls, x, y, **props):
    e = MEL.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def link(a, a_out, b, b_in):
    MEL.connect_material_expressions(a, a_out, b, b_in)


def const(v, x, y):
    return node(unreal.MaterialExpressionConstant, x, y, r=v)


# World-space UVs (cm -> tiles).
wp = node(unreal.MaterialExpressionWorldPosition, -1600, 0)
xy = node(unreal.MaterialExpressionComponentMask, -1400, 0, r=True, g=True, b=False, a=False)
link(wp, "", xy, "")


def uvs(tile_cm, y):
    d = node(unreal.MaterialExpressionDivide, -1200, y)
    link(xy, "", d, "A")
    link(const(tile_cm, -1350, y + 60), "", d, "B")
    return d


def sample(tex_path, uv, x, y, stype):
    s = node(unreal.MaterialExpressionTextureSample, x, y,
             texture=unreal.load_asset(tex_path), sampler_type=stype)
    link(uv, "", s, "UVs")
    return s


COLOR = unreal.MaterialSamplerType.SAMPLERTYPE_COLOR
NORMAL = unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
MASKS = unreal.MaterialSamplerType.SAMPLERTYPE_MASKS

uv_g, uv_r, uv_macro = uvs(400.0, -400), uvs(900.0, 400), uvs(9000.0, 900)
g_b, g_n, g_m = (sample(GRASS + "_B", uv_g, -900, -600, COLOR), sample(GRASS + "_N", uv_g, -900, -350, NORMAL),
                 sample(GRASS + "_ORM", uv_g, -900, -100, MASKS))
r_b, r_n, r_m = (sample(ROCK + "_B", uv_r, -900, 200, COLOR), sample(ROCK + "_N", uv_r, -900, 450, NORMAL),
                 sample(ROCK + "_ORM", uv_r, -900, 700, MASKS))
macro = sample(GRASS + "_B", uv_macro, -900, 950, COLOR)  # very low frequency brightness variation

# Slope: rock where the surface normal leans more than ~40 degrees.  alpha = saturate((0.82 - Nz) * 8)
nrm = node(unreal.MaterialExpressionVertexNormalWS, -900, 1200)
nz = node(unreal.MaterialExpressionComponentMask, -700, 1200, r=False, g=False, b=True, a=False)
link(nrm, "", nz, "")
sub = node(unreal.MaterialExpressionSubtract, -550, 1200)
link(const(0.82, -700, 1300), "", sub, "A")
link(nz, "", sub, "B")
mul = node(unreal.MaterialExpressionMultiply, -400, 1200)
link(sub, "", mul, "A")
link(const(8.0, -550, 1300), "", mul, "B")
slope = node(unreal.MaterialExpressionSaturate, -250, 1200)
link(mul, "", slope, "")


def lerp(a, a_out, b, b_out, x, y):
    l = node(unreal.MaterialExpressionLinearInterpolate, x, y)
    link(a, a_out, l, "A")
    link(b, b_out, l, "B")
    link(slope, "", l, "Alpha")
    return l


base = lerp(g_b, "RGB", r_b, "RGB", -300, -500)
normal = lerp(g_n, "RGB", r_n, "RGB", -300, -250)
rough = lerp(g_m, "G", r_m, "G", -300, 0)
ao = lerp(g_m, "R", r_m, "R", -300, 150)

# Macro variation: base * (0.45 + macro.R * 0.3), then dry the whole palette slightly toward ochre.
mv = node(unreal.MaterialExpressionMultiply, -500, 950)
link(macro, "R", mv, "A")
link(const(0.3, -650, 1050), "", mv, "B")
mv2 = node(unreal.MaterialExpressionAdd, -350, 950)
link(mv, "", mv2, "A")
link(const(0.45, -500, 1050), "", mv2, "B")
tinted = node(unreal.MaterialExpressionMultiply, -100, -500)
link(base, "", tinted, "A")
link(mv2, "", tinted, "B")

MEL.connect_material_property(tinted, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(normal, "", unreal.MaterialProperty.MP_NORMAL)
MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.connect_material_property(ao, "", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(path)

unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Vysi")
for a in unreal.EditorLevelLibrary.get_all_level_actors():
    if a.get_class().get_name() == "Landscape":
        a.set_editor_property("landscape_material", unreal.load_asset(path))
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log_warning(f"FNCHECK material built and assigned: {path}")
