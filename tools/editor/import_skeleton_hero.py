# Imports the rigged Mixamo skeleton (Import/Skeleton_Mixamo.fbx) as a skeletal mesh into /Game/Characters/Skeleton.
import unreal

SRC = unreal.Paths.project_dir() + "Import/Skeleton_Mixamo.fbx"
DST = "/Game/Characters/Skeleton"

opts = unreal.FbxImportUI()
opts.import_mesh = True
opts.import_as_skeletal = True
opts.import_animations = False
opts.import_materials = True
opts.import_textures = True
opts.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH

task = unreal.AssetImportTask()
task.filename = SRC
task.destination_path = DST
task.destination_name = "SK_HeroSkeleton"
task.automated = True
task.replace_existing = True
task.save = True
task.options = opts
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

for p in task.imported_object_paths:
    a = unreal.load_asset(p)
    unreal.log_warning("IMPORTED %s %s" % (p, type(a).__name__))
    if isinstance(a, unreal.SkeletalMesh):
        b = a.get_bounds()
        unreal.log_warning("BOUNDS origin=%s extent=%s" % (b.origin, b.box_extent))
        unreal.log_warning("MATERIALS %d" % len(a.materials))
