"""Export triangles of static meshes to JSON (for previews). Output path from env MESH_EXPORT_OUT."""
import json, os
import unreal

BROKEN_DIR = os.environ.get("MESH_EXPORT_DIR", "/Game/BORANAGA/Objects/BrokenMeshes")
ASSETS = {
    "BrokenLED": BROKEN_DIR + "/SM_BrokenLED",
    "BrokenLongLED": BROKEN_DIR + "/SM_BrokenLongLED",
    "IntactLED": "/Game/StarterBundle/ModularScifiHallways/Meshes/SM_Hall_Light_Square",
    "IntactLongLED": "/Game/StarterBundle/ModularSci_Comm/Meshes/SM_Light_A",
}
out = {}
for key, path in ASSETS.items():
    m = unreal.EditorAssetLibrary.load_asset(path)
    d = m.get_static_mesh_description(0)
    tris = []
    for t in range(d.get_triangle_count()):
        vis = d.get_triangle_vertex_instances(unreal.TriangleID(t))
        pts = []
        for vi in vis:
            v = d.get_vertex_instance_vertex(vi)
            p = d.get_vertex_position(v)
            pts.append([p.x, p.y, p.z])
        tris.append(pts)
    out[key] = tris
    unreal.log_warning(f"EXPORT {key}: {len(tris)} tris")
with open(os.environ["MESH_EXPORT_OUT"], "w") as f:
    json.dump(out, f)
