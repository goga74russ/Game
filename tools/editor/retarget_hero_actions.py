"""Bake local Epic sample actions onto Wraith. Run with the editor closed.

Requires the installed Paragon Greystone/Wraith packs and a local copy of the
Game Animation Sample's UEFN mannequin mesh, skeleton and landing-roll clip.
All outputs stay in ignored /Game/Characters/HeroActions: licensed assets must
not be redistributed in the public repository.
"""
import os
from pathlib import Path
import shutil
import unreal

# Copy only the mannequin and one motion we actually use, preserving Unreal
# package paths. This is local preparation, not public asset redistribution.
sample = Path(os.environ.get("FN_ANIMATION_SAMPLE", "D:/Ai/UESamples/GameAnimationSample"))
content = Path(unreal.Paths.project_content_dir())
files = ["Meshes/SKM_UEFN_Mannequin.uasset", "Meshes/SK_UEFN_Mannequin.uasset",
         "Meshes/Character_LodSettings.uasset", "Animations/Jump/M_Neutral_Jump_F_Land_Roll_Lfoot.uasset"]
for relative in files:
    source = sample / "Content/Characters/UEFN_Mannequin" / relative
    destination = content / "Characters/UEFN_Mannequin" / relative
    if not destination.exists():
        assert source.is_file(), "Install Game Animation Sample or set FN_ANIMATION_SAMPLE: " + str(source)
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/Game/Characters/UEFN_Mannequin"], True)

OUT = "/Game/Characters/HeroActions"
WRAITH = "/Game/ParagonWraith/Characters/Heroes/Wraith/Meshes/Wraith"
GREY = "/Game/ParagonGreystone/Characters/Heroes/Greystone/Meshes/Greystone"
MANNEQUIN = "/Game/Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin"

def asset(name, cls, factory):
    path = OUT + "/" + name
    return unreal.load_asset(path) or unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, OUT, cls, factory())

def rig(name, mesh):
    result = asset(name, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory)
    ctrl = unreal.IKRigController.get_controller(result)
    assert ctrl.set_skeletal_mesh(mesh)
    # Explicit shared Epic bone names avoid a dependency on auto-characterizer
    # templates and include fingers for both weapon grip and an unarmed strike.
    pose = mesh.skeleton.get_reference_pose()
    bones = {str(n) for n in unreal.AnimPoseExtensions.get_bone_names(pose)}
    chains = {
        "Root": ("root", "root"), "Spine": ("spine_01", "spine_03"),
        "Head": ("neck_01", "head"),
        "ArmLeft": ("clavicle_l", "hand_l"), "ArmRight": ("clavicle_r", "hand_r"),
        "LegLeft": ("thigh_l", "ball_l"), "LegRight": ("thigh_r", "ball_r"),
    }
    for side in ["l", "r"]:
        for finger in ["thumb", "index", "middle", "ring", "pinky"]:
            chains[finger + side] = (finger + "_01_" + side, finger + "_03_" + side)
    for old in ctrl.get_retarget_chains():
        ctrl.remove_retarget_chain(old.chain_name)
    ctrl.set_retarget_root("pelvis")
    ctrl.set_root_motion_bone("root")
    for label, (start, end) in chains.items():
        assert start in bones and end in bones, (name, start, end, bones)
        ctrl.add_retarget_chain(label, start, end, "")
    unreal.EditorAssetLibrary.save_loaded_asset(result)
    return result

target = unreal.load_asset(WRAITH)
assert target
target_rig = rig("IK_WraithActions", target)

def bake(label, source_path, clips):
    source = unreal.load_asset(source_path)
    assert source, source_path
    source_rig = rig("IK_" + label, source)
    rt = asset("RT_" + label + "_Wraith", unreal.IKRetargeter, unreal.IKRetargetFactory)
    ctrl = unreal.IKRetargeterController.get_controller(rt)
    ctrl.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, source_rig)
    ctrl.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, target_rig)
    ctrl.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE, source)
    ctrl.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET, target)
    ctrl.add_default_ops()
    ctrl.auto_map_chains(unreal.AutoMapChainType.EXACT, True)
    ctrl.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)
    unreal.EditorAssetLibrary.save_loaded_asset(rt)
    for path, output in clips:
        seq = unreal.load_asset(path)
        assert isinstance(seq, unreal.AnimSequence), path
        inputs = unreal.IKRetargetBatchOperationInputs()
        inputs.assets_to_retarget = [unreal.EditorAssetLibrary.find_asset_data(path)]
        inputs.source_mesh = source
        inputs.target_mesh = target
        inputs.ik_retarget_asset = rt
        inputs.target_path = OUT
        inputs.search = seq.get_name()
        inputs.replace = output
        inputs.include_referenced_assets = False
        inputs.overwrite_existing_files = True
        inputs.retain_additive_flags = False
        results = unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
        assert results, output
        anim = unreal.load_asset(OUT + "/" + output)
        assert anim and anim.get_editor_property("skeleton") == target.skeleton, output
        # Gameplay moves the capsule; the action pose must not translate it again.
        anim.set_editor_property("enable_root_motion", True)
        anim.set_editor_property("force_root_lock", True)
        anim.set_editor_property("root_motion_root_lock", unreal.RootMotionRootLock.REF_POSE)
        # Source gameplay notifications do not belong to our attacks.
        unreal.AnimationLibrary.remove_all_animation_notify_tracks(anim)
        unreal.EditorAssetLibrary.save_loaded_asset(anim)
        unreal.log_warning("FN_ACTION_BAKED %s duration=%.3f" % (output, anim.get_play_length()))

base = "/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/"
bake("Greystone", GREY, [(base + "Attack_PrimaryA", "A_HeroMelee"), (base + "Attack_RMB", "A_HeroUnarmed")])
bake("Mannequin", MANNEQUIN, [("/Game/Characters/UEFN_Mannequin/Animations/Jump/M_Neutral_Jump_F_Land_Roll_Lfoot", "A_HeroRoll")])
bake("Wraith", WRAITH, [("/Game/ParagonWraith/Characters/Heroes/Wraith/Animations/Fire_A_Slow", "A_HeroShot")])
unreal.log_warning("FN_ACTIONS_SUCCESS count=4")
