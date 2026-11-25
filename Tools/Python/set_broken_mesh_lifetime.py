"""
Set how long the broken LED fragments stay before they disappear.
BrokenMeshLifeTime is counted from the moment of breaking (not from landing), so it is
"fall time (~1s from ceiling height) + 5s on the floor".
"""
import unreal

LIFETIME = 6.0
BLUEPRINTS = [
    "/Game/BORANAGA/Objects/BP_DestructibleLED",
    "/Game/BORANAGA/Objects/BP_DestructibleLongLED",
]

for path in BLUEPRINTS:
    bp = unreal.EditorAssetLibrary.load_asset(path)
    cdo = unreal.get_default_object(bp.generated_class())
    cdo.set_editor_property("broken_mesh_life_time", LIFETIME)
    cdo.set_editor_property("hide_broken_mesh_after_delay", True)
    cdo.set_editor_property("destroy_actor_after_break", False)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_loaded_asset(bp)
    unreal.log_warning(f"[BrokenLife] {path}: lifetime={cdo.get_editor_property('broken_mesh_life_time')}")
