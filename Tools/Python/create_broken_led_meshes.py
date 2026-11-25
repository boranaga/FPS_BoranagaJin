"""
Generate procedural 'broken' StaticMeshes for the destructible LED blueprints
and assign them to the BrokenMesh component of each blueprint.

Run via Tools/Python/run_create_broken_led_meshes.ps1 (editor closed), or from
the editor with the Python Editor Script plugin enabled.

Env vars:
    BROKEN_DRY_RUN=1   create meshes only in a scratch folder, do not touch blueprints
"""
import math
import os
import random

import unreal

DRY_RUN = os.environ.get("BROKEN_DRY_RUN", "0") == "1"
OUT_DIR = "/Game/_ScratchBroken" if DRY_RUN else "/Game/BORANAGA/Objects/BrokenMeshes"

# Blueprint -> new mesh name
TARGETS = [
    ("/Game/BORANAGA/Objects/BP_DestructibleLED", "SM_BrokenLED"),
    ("/Game/BORANAGA/Objects/BP_DestructibleLongLED", "SM_BrokenLongLED"),
]

EAL = unreal.EditorAssetLibrary
SUBS = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
SDL = unreal.SubobjectDataBlueprintFunctionLibrary


def get_template(bp, comp_name):
    for h in SUBS.k2_gather_subobject_data_for_blueprint(bp):
        data = SDL.get_data(h)
        obj = SDL.get_object_for_blueprint(data, bp)
        if obj and obj.get_name().split("_GEN_VARIABLE")[0] == comp_name:
            return obj
    return None


def add_cleanup_component(bp):
    """Add BrokenMeshCleanupComponent (C++, new class) to the blueprint if it is not there yet."""
    handles = SUBS.k2_gather_subobject_data_for_blueprint(bp)
    for h in handles:
        o = SDL.get_object_for_blueprint(SDL.get_data(h), bp)
        if o and isinstance(o, unreal.BrokenMeshCleanupComponent):
            return
    params = unreal.AddNewSubobjectParams()
    params.set_editor_property("parent_handle", handles[0])
    params.set_editor_property("new_class", unreal.BrokenMeshCleanupComponent)
    params.set_editor_property("blueprint_context", bp)
    handle, fail = SUBS.add_new_subobject(params)
    if str(fail):
        unreal.log_error(f"[BrokenLED] add component failed: {fail}")
    else:
        SUBS.rename_subobject(handle, unreal.Text("BrokenMeshCleanup"))
        unreal.log_warning(f"[BrokenLED] added BrokenMeshCleanupComponent to {bp.get_name()}")


def vsub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def vcross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def vnorm(a):
    l = math.sqrt(a[0] ** 2 + a[1] ** 2 + a[2] ** 2) or 1.0
    return (a[0] / l, a[1] / l, a[2] / l)


def rot(p, c, ang, axis):
    v = vsub(p, c)
    ca, sa = math.cos(ang), math.sin(ang)
    d = v[0] * axis[0] + v[1] * axis[1] + v[2] * axis[2]
    cr = vcross(axis, v)
    r = tuple(v[i] * ca + cr[i] * sa + axis[i] * d * (1 - ca) for i in range(3))
    return (r[0] + c[0], r[1] + c[1], r[2] + c[2])


def build_shards(origin, ext, seed):
    """Triangles of shattered shards filling the bounds box."""
    rng = random.Random(seed)
    e = [ext.x, ext.y, ext.z]
    o = [origin.x, origin.y, origin.z]
    order = sorted(range(3), key=lambda i: e[i])
    t_ax, s_ax, l_ax = order[0], order[1], order[2]  # thin, short, long
    n_l = max(2, min(8, int(round(e[l_ax] / max(e[s_ax], 1.0) * 2))))
    n_s = 2
    thick = max(e[t_ax], 0.5)
    tris = []

    def to3(pl, ps, z):
        v = [0.0, 0.0, 0.0]
        v[l_ax] = o[l_ax] + pl
        v[s_ax] = o[s_ax] + ps
        v[t_ax] = o[t_ax] + z
        return tuple(v)

    for i in range(n_l):
        for j in range(n_s):
            if rng.random() < 0.18:  # hole in the shattered fixture
                continue
            l0 = -e[l_ax] + 2 * e[l_ax] * i / n_l
            l1 = -e[l_ax] + 2 * e[l_ax] * (i + 1) / n_l
            s0 = -e[s_ax] + 2 * e[s_ax] * j / n_s
            s1 = -e[s_ax] + 2 * e[s_ax] * (j + 1) / n_s
            corners = [(l0, s0), (l1, s0), (l1, s1), (l0, s1)]
            if rng.random() < 0.5:
                corners = corners[1:] + corners[:1]
            for tri2d in ([corners[0], corners[1], corners[2]], [corners[0], corners[2], corners[3]]):
                if rng.random() < 0.15:
                    continue
                cl = sum(p[0] for p in tri2d) / 3.0
                cs = sum(p[1] for p in tri2d) / 3.0
                shrink = rng.uniform(0.82, 0.96)
                pts = []
                for (pl, ps) in tri2d:
                    pl = cl + (pl - cl) * shrink + rng.uniform(-0.04, 0.04) * e[s_ax]
                    ps = cs + (ps - cs) * shrink + rng.uniform(-0.04, 0.04) * e[s_ax]
                    pts.append((pl, ps))
                zoff = rng.uniform(-0.35, 0.35) * thick
                tilt_dir = rng.uniform(0, 2 * math.pi)
                tilt = rng.uniform(-0.22, 0.22)
                top = [to3(pl, ps, thick * rng.uniform(0.5, 1.0) + zoff) for pl, ps in pts]
                bot = [to3(pl, ps, -thick * rng.uniform(0.5, 1.0) + zoff) for pl, ps in pts]
                center = to3(cl, cs, zoff)
                ax = [0.0, 0.0, 0.0]
                ax[l_ax] = math.cos(tilt_dir)
                ax[s_ax] = math.sin(tilt_dir)
                ax = tuple(ax)
                top = [rot(p, center, tilt, ax) for p in top]
                bot = [rot(p, center, tilt, ax) for p in bot]
                tris.append((top[0], top[1], top[2]))
                tris.append((bot[0], bot[2], bot[1]))
                for k in range(3):
                    a, b = k, (k + 1) % 3
                    tris.append((top[a], bot[a], bot[b]))
                    tris.append((top[a], bot[b], top[b]))
    return tris, (l_ax, s_ax)


def write_fbx(path, tris, l_ax, s_ax, org, ext):
    """Write triangles as an ASCII FBX with explicit per-corner normals and UVs (Z-up, cm)."""
    verts, idx, normals, uvs = [], [], [], []
    for (p0, p1, p2) in tris:
        n = vnorm(vcross(vsub(p1, p0), vsub(p2, p0)))
        for k, p in enumerate((p0, p1, p2)):
            verts += [p[0], p[1], p[2]]
            normals += [n[0], n[1], n[2]]
            uvs += [(p[l_ax] - org[l_ax]) / (2 * ext[l_ax] + 1e-6) + 0.5,
                    (p[s_ax] - org[s_ax]) / (2 * ext[s_ax] + 1e-6) + 0.5]
            i = len(idx)
            idx.append(i if k < 2 else -(i + 1))  # last index of a polygon is stored as ~i
    f = lambda a: ",".join(f"{x:.6f}" for x in a)
    txt = f"""; FBX 7.4.0 project file
FBXHeaderExtension:  {{
	FBXHeaderVersion: 1003
	FBXVersion: 7400
	Creator: "create_broken_led_meshes.py"
}}
GlobalSettings:  {{
	Version: 1000
	Properties70:  {{
		P: "UpAxis", "int", "Integer", "",2
		P: "UpAxisSign", "int", "Integer", "",1
		P: "FrontAxis", "int", "Integer", "",1
		P: "FrontAxisSign", "int", "Integer", "",-1
		P: "CoordAxis", "int", "Integer", "",0
		P: "CoordAxisSign", "int", "Integer", "",1
		P: "UnitScaleFactor", "double", "Number", "",1
	}}
}}
Definitions:  {{
	Version: 100
	Count: 2
	ObjectType: "Geometry" {{
		Count: 1
	}}
	ObjectType: "Model" {{
		Count: 1
	}}
}}
Objects:  {{
	Geometry: 1000, "Geometry::BrokenMesh", "Mesh" {{
		Vertices: *{len(verts)} {{
			a: {f(verts)}
		}}
		PolygonVertexIndex: *{len(idx)} {{
			a: {",".join(str(i) for i in idx)}
		}}
		GeometryVersion: 124
		LayerElementNormal: 0 {{
			Version: 101
			Name: ""
			MappingInformationType: "ByPolygonVertex"
			ReferenceInformationType: "Direct"
			Normals: *{len(normals)} {{
				a: {f(normals)}
			}}
		}}
		LayerElementUV: 0 {{
			Version: 101
			Name: "UVMap"
			MappingInformationType: "ByPolygonVertex"
			ReferenceInformationType: "Direct"
			UV: *{len(uvs)} {{
				a: {f(uvs)}
			}}
		}}
		LayerElementMaterial: 0 {{
			Version: 101
			Name: ""
			MappingInformationType: "AllSame"
			ReferenceInformationType: "IndexToDirect"
			Materials: *1 {{
				a: 0
			}}
		}}
		Layer: 0 {{
			Version: 100
			LayerElement:  {{
				Type: "LayerElementNormal"
				TypedIndex: 0
			}}
			LayerElement:  {{
				Type: "LayerElementMaterial"
				TypedIndex: 0
			}}
			LayerElement:  {{
				Type: "LayerElementUV"
				TypedIndex: 0
			}}
		}}
	}}
	Model: 2000, "Model::BrokenMesh", "Mesh" {{
		Version: 232
		Shading: T
		Culling: "CullingOff"
	}}
}}
Connections:  {{
	C: "OO",2000,0
	C: "OO",1000,2000
}}
"""
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(txt)


def make_mesh(name, src_mesh, material, seed):
    asset_path = f"{OUT_DIR}/{name}"
    if EAL.does_asset_exist(asset_path):
        EAL.delete_asset(asset_path)
    b = src_mesh.get_bounds()
    tris, (l_ax, s_ax) = build_shards(b.origin, b.box_extent, seed)
    ext = [b.box_extent.x, b.box_extent.y, b.box_extent.z]
    org = [b.origin.x, b.origin.y, b.origin.z]

    import tempfile
    fbx_path = os.path.join(tempfile.gettempdir(), name + ".fbx")
    write_fbx(fbx_path, tris, l_ax, s_ax, org, ext)

    # Interchange scripted import (AssetTools' import task path syncs the content browser and crashes headless)
    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    source = unreal.InterchangeManager.create_source_data(fbx_path)
    params = unreal.ImportAssetParameters()
    params.set_editor_property("is_automated", True)
    params.set_editor_property("replace_existing", True)
    params.set_editor_property("destination_name", name)
    mgr.import_asset(OUT_DIR, source, params)
    mesh = EAL.load_asset(asset_path)
    if not mesh:
        raise RuntimeError("FBX import failed for " + asset_path)

    nb = mesh.get_bounds()
    unreal.log_warning(f"[BrokenLED] {name} bounds origin={nb.origin} extent={nb.box_extent} (source extent={b.box_extent})")

    mesh.set_material(0, material)
    unreal.EditorStaticMeshLibrary.add_simple_collisions(mesh, unreal.ScriptingCollisionShapeType.BOX)
    EAL.save_loaded_asset(mesh)
    unreal.log(f"[BrokenLED] created {asset_path} with {len(tris)} triangles")
    return mesh


def main():
    if not EAL.does_directory_exist(OUT_DIR):
        EAL.make_directory(OUT_DIR)
    for bp_path, mesh_name in TARGETS:
        bp = EAL.load_asset(bp_path)
        if not bp:
            unreal.log_error(f"[BrokenLED] cannot load {bp_path}")
            continue
        intact = get_template(bp, "IntactMesh")
        broken = get_template(bp, "BrokenMesh")
        if not intact or not broken:
            unreal.log_error(f"[BrokenLED] {bp_path}: IntactMesh/BrokenMesh template not found")
            continue
        src_mesh = intact.get_editor_property("static_mesh")
        cdo = unreal.get_default_object(bp.generated_class())
        emissive_idx = set()
        try:
            emissive_idx = {m.get_editor_property("material_index")
                            for m in cdo.get_editor_property("emissive_materials")}
        except Exception as ex:
            unreal.log_warning(f"[BrokenLED] could not read EmissiveMaterials: {ex}")
        n_mats = len(src_mesh.static_materials)
        pick = next((i for i in range(n_mats) if i not in emissive_idx), 0)
        material = src_mesh.get_material(pick)
        if pick in emissive_idx and material:
            # every slot glows: use a dark (intensity 0) child instance so the wreck does not emit light
            param = str(cdo.get_editor_property("emissive_intensity_parameter_name"))
            mic_name = f"MI_{mesh_name}_Dark"
            mic_path = f"{OUT_DIR}/{mic_name}"
            if EAL.does_asset_exist(mic_path):
                EAL.delete_asset(mic_path)
            mic = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                mic_name, OUT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
            mic.set_editor_property("parent", material)
            unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mic, param, 0.0)
            EAL.save_loaded_asset(mic)
            material = mic
        unreal.log(f"[BrokenLED] {bp_path}: src={src_mesh.get_path_name()} mats={n_mats} "
                   f"emissive={sorted(emissive_idx)} use slot {pick}")
        seed = sum(ord(c) for c in mesh_name)
        new_mesh = make_mesh(mesh_name, src_mesh, material, seed)
        if DRY_RUN:
            continue
        broken.set_editor_property("static_mesh", new_mesh)
        add_cleanup_component(bp)
        # the cleanup component removes the actor 6s after landing; disable the timed hide (10s after break)
        cdo.set_editor_property("hide_broken_mesh_after_delay", False)
        cdo.set_editor_property("destroy_actor_after_break", False)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        EAL.save_loaded_asset(bp)
        unreal.log(f"[BrokenLED] assigned {mesh_name} to {bp_path}")


main()
