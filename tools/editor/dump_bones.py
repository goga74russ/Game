# Prints bone names of the hero meshes (retarget mapping check).
import unreal
for p in ["/Game/Characters/Skeleton/SK_HeroSkeleton", "/Game/ParagonWraith/Characters/Heroes/Wraith/Meshes/Wraith"]:
    sk = unreal.load_asset(p).skeleton
    pose = sk.get_reference_pose()
    names = [str(n) for n in unreal.AnimPoseExtensions.get_bone_names(pose)]
    unreal.log_warning("BONES %s: %s" % (p, " ".join(names)))
