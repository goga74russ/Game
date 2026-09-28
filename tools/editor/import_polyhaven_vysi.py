"""Import director-selected CC0 Poly Haven surfaces and apply a three-surface Vysi material.
Run with UnrealEditor-Cmd -run=pythonscript -script=<this file> -EnablePlugins=PythonScriptPlugin.
"""
import os
import unreal

MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
ROOT = os.environ.get('FN_POLYHAVEN_ROOT', r'D:\Ai\Assets\PolyHaven')
DEST = '/Game/Materials/Landscape/PolyHaven'
textures = {}
for surface in ('rocky_trail_02', 'forest_leaves_04', 'marble_cliff_04'):
    for kind in ('diff', 'nor_dx', 'arm', 'disp'):
        filename = os.path.join(ROOT, surface, 'textures', f'{surface}_{kind}_2k.png')
        if not os.path.isfile(filename):
            raise RuntimeError('Missing texture: ' + filename)
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', filename)
        task.set_editor_property('destination_path', DEST + '/' + surface)
        task.set_editor_property('automated', True)
        task.set_editor_property('replace_existing', True)
        task.set_editor_property('save', True)
        AT.import_asset_tasks([task])
        tex = unreal.load_asset(task.get_editor_property('imported_object_paths')[0])
        tex.set_editor_property('srgb', kind == 'diff')
        if kind == 'nor_dx':
            tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP)
        elif kind != 'diff':
            tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS)
        unreal.EditorAssetLibrary.save_loaded_asset(tex)
        textures[surface, kind] = tex

path = '/Game/Materials/Landscape/M_VysiLandscape'
mat = unreal.load_asset(path)
if not mat:
    mat = AT.create_asset('M_VysiLandscape', '/Game/Materials/Landscape', unreal.Material, unreal.MaterialFactoryNew())
MEL.delete_all_material_expressions(mat)
def node(cls, **props):
    e = MEL.create_material_expression(mat, cls, 0, 0)
    for k,v in props.items(): e.set_editor_property(k,v)
    return e
def link(a, out, b, inp):
    if not MEL.connect_material_expressions(a,out,b,inp): raise RuntimeError('Connection failed: '+inp)
def scalar(v): return node(unreal.MaterialExpressionConstant, r=v)
def binary(cls,a,b):
    n=node(cls); link(a,'',n,'A'); link(b,'',n,'B'); return n
wp=node(unreal.MaterialExpressionWorldPosition)
xy=node(unreal.MaterialExpressionComponentMask,r=True,g=True,b=False,a=False); link(wp,'',xy,'')
# Approximate chapter road centreline in world metres; blend edges across 4 metres.
route=[(-30,-10),(0,0),(45,12),(140,38),(180,40),(280,-20),(420,-20),(445,0),(525,10),(560,20),(640,10),(698,-8),(720,0),(742,0),(850,0),(875,12)]
code='float2 p=P.xy/100.0; float d=100000.0;\n'
for a,b in zip(route,route[1:]):
    code+=f'{{float2 a=float2({a[0]}.0,{a[1]}.0), b=float2({b[0]}.0,{b[1]}.0); float2 v=b-a; float t=saturate(dot(p-a,v)/dot(v,v)); d=min(d,length(p-a-t*v));}}\n'
code+='return 1.0-smoothstep(3.0,7.0,d);'
road=node(unreal.MaterialExpressionCustom, code=code, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT1)
ci=unreal.CustomInput(); ci.set_editor_property('input_name','P'); road.set_editor_property('inputs',[ci]); link(wp,'',road,'P')
vn=node(unreal.MaterialExpressionVertexNormalWS)
nz=node(unreal.MaterialExpressionComponentMask,r=False,g=False,b=True,a=False); link(vn,'',nz,'')
slope=binary(unreal.MaterialExpressionMultiply,binary(unreal.MaterialExpressionSubtract,scalar(0.82),nz),scalar(8))
sat=node(unreal.MaterialExpressionSaturate); link(slope,'',sat,'')
def sample(surface,kind,scale):
    uv=binary(unreal.MaterialExpressionDivide,xy,scalar(scale))
    typ=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if kind=='diff' else (unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if kind=='nor_dx' else unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    n=node(unreal.MaterialExpressionTextureSample,texture=textures[surface,kind],sampler_type=typ); link(uv,'',n,'UVs'); return n
samples={s:{k:sample(s,k,200 if s=='rocky_trail_02' else (150 if s=='forest_leaves_04' else 500)) for k in ('diff','nor_dx','arm')} for s in ('rocky_trail_02','forest_leaves_04','marble_cliff_04')}
def blend(a,ao,b,bo,alpha):
    n=node(unreal.MaterialExpressionLinearInterpolate); link(a,ao,n,'A'); link(b,bo,n,'B'); link(alpha,'',n,'Alpha'); return n
for kind,out,prop in [('diff','RGB',unreal.MaterialProperty.MP_BASE_COLOR),('nor_dx','RGB',unreal.MaterialProperty.MP_NORMAL),('arm','G',unreal.MaterialProperty.MP_ROUGHNESS),('arm','R',unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)]:
    ground=blend(samples['forest_leaves_04'][kind],out,samples['rocky_trail_02'][kind],out,road)
    final=blend(ground,'',samples['marble_cliff_04'][kind],out,sat)
    MEL.connect_material_property(final,'',prop)
MEL.layout_material_expressions(mat)
MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.EditorLoadingAndSavingUtils.load_map('/Game/Maps/Vysi')
count=0
for actor in unreal.EditorLevelLibrary.get_all_level_actors():
    if isinstance(actor, unreal.LandscapeProxy):
        actor.set_editor_property('landscape_material',mat)
        count+=1
if not count: raise RuntimeError('No landscape found')
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
unreal.log_warning('FN_POLYHAVEN_SUCCESS textures=12 landscapes='+str(count))

