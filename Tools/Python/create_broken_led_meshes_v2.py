"""
v2: BrokenMesh that keeps the shape of the original LED fixture but looks broken.

Takes the real IntactMesh geometry (keeps its UVs and material slots), then
  * subdivides it so the damage can be cut finely,
  * knocks a jagged chunk out of one corner and cuts a crack across the panel,
  * bends the fixture down toward the room around the broken end (it "droops"),
  * crumples the vertices around the broken edge,
and imports the result as a new StaticMesh. Glowing material slots get a dark (intensity 0)
material instance, the other slots keep the original materials.

Run via Tools/Python/run_create_broken_led_meshes.ps1 -Script create_broken_led_meshes_v2.py (editor closed).
Env: BROKEN_DRY_RUN=1  -> meshes go to /Game/_ScratchBroken, blueprints untouched.
"""
import math
import os
import random
import tempfile

import unreal

DRY_RUN = os.environ.get("BROKEN_DRY_RUN", "0") == "1"
OUT_DIR = "/Game/_ScratchBroken" if DRY_RUN else "/Game/BORANAGA/Objects/BrokenMeshes"

TARGETS = [
    ("/Game/BORANAGA/Objects/BP_DestructibleLED", "SM_BrokenLED"),
    ("/Game/BORANAGA/Objects/BP_DestructibleLongLED", "SM_BrokenLongLED"),
]

MAX_TRIS = 12000        # upper bound for triangles after subdivision
MAX_EDGE = 1.5          # cm, triangles are subdivided until their longest edge is below this
CRACK_WIDTH = 0.45      # cm
BEND_MAX_DEG = 38.0     # how far the broken end droops
CHUNK_A = 0.30          # chunk size along the long axis (fraction of the long extent)
CHUNK_B = 0.65          # chunk size along the short axis (fraction of the short extent)

EAL = unreal.EditorAssetLibrary
SUBS = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
SDL = unreal.SubobjectDataBlueprintFunctionLibrary


# ---------------------------------------------------------------- blueprint helpers
def get_template(bp, comp_name):
    for h in SUBS.k2_gather_subobject_data_for_blueprint(bp):
        o = SDL.get_object_for_blueprint(SDL.get_data(h), bp)
        if o and o.get_name().split("_GEN_VARIABLE")[0] == comp_name:
            return o
    return None


def add_cleanup_component(bp):
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
        unreal.log_error(f"[BrokenV2] add component failed: {fail}")
    else:
        SUBS.rename_subobject(handle, unreal.Text("BrokenMeshCleanup"))


# ---------------------------------------------------------------- small vector helpers
def sub(a, b): return [a[0] - b[0], a[1] - b[1], a[2] - b[2]]
def cross(a, b): return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]
def dot(a, b): return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
def length(a): return math.sqrt(dot(a, a))
def normalize(a):
    l = length(a) or 1.0
    return [a[0] / l, a[1] / l, a[2] / l]
def smoothstep(x):
    x = max(0.0, min(1.0, x))
    return x * x * (3 - 2 * x)


def hash01(p, salt=0):
    """Deterministic pseudo random in [0,1) from a position (so welded vertices move together)."""
    k = (round(p[0] * 50), round(p[1] * 50), round(p[2] * 50), salt)
    return random.Random(hash(k) & 0xFFFFFFFF).random()


# ---------------------------------------------------------------- read source mesh
class Tri:
    __slots__ = ("p", "uv", "mat")

    def __init__(self, p, uv, mat):
        self.p, self.uv, self.mat = p, uv, mat


def read_tris(mesh):
    d = mesh.get_static_mesh_description(0)
    tris = []
    for i in range(d.get_triangle_count()):
        tid = unreal.TriangleID(i)
        mat = d.get_triangle_polygon_group(tid).id_value
        p, uv = [], []
        for vi in d.get_triangle_vertex_instances(tid):
            v = d.get_vertex_position(d.get_vertex_instance_vertex(vi))
            p.append([v.x, v.y, v.z])
            t = d.get_vertex_instance_uv(vi, 0)
            uv.append([t.x, t.y])
        tris.append(Tri(p, uv, mat))
    return tris


def subdivide(tris, max_edge):
    """Split triangles on their longest edge until every edge is shorter than max_edge."""
    out, stack = [], list(tris)
    while stack:
        t = stack.pop()
        e = [length(sub(t.p[(i + 1) % 3], t.p[i])) for i in range(3)]
        i = max(range(3), key=lambda k: e[k])
        if e[i] <= max_edge:
            out.append(t)
            continue
        a, b, c = i, (i + 1) % 3, (i + 2) % 3
        mp = [(t.p[a][k] + t.p[b][k]) / 2 for k in range(3)]
        mu = [(t.uv[a][k] + t.uv[b][k]) / 2 for k in range(2)]
        stack.append(Tri([t.p[a], mp, t.p[c]], [t.uv[a], mu, t.uv[c]], t.mat))
        stack.append(Tri([mp, t.p[b], t.p[c]], [mu, t.uv[b], t.uv[c]], t.mat))
    return out


# ---------------------------------------------------------------- the breaking
def break_tris(tris, seed):
    rng = random.Random(seed)
    pts = [p for t in tris for p in t.p]
    lo = [min(p[i] for p in pts) for i in range(3)]
    hi = [max(p[i] for p in pts) for i in range(3)]
    ext = [hi[i] - lo[i] for i in range(3)]
    order = sorted(range(3), key=lambda i: ext[i])
    T, S, L = order                       # thin, short, long axis
    ctr = [(lo[i] + hi[i]) / 2 for i in range(3)]
    front = 1.0 if ctr[T] >= 0 else -1.0  # pivot sits on the mounting side, so the fixture extends to the front

    el = rng.choice((-1.0, 1.0))          # which end breaks
    es = rng.choice((-1.0, 1.0))          # which side
    Lc = hi[L] if el > 0 else lo[L]
    Sc = hi[S] if es > 0 else lo[S]
    a, b = CHUNK_A * ext[L], CHUNK_B * ext[S]
    ph1, ph2 = rng.uniform(0, 6.28), rng.uniform(0, 6.28)

    def chunk_r(l, s):
        dl, ds = abs(l - Lc), abs(s - Sc)
        th = math.atan2(ds / b, dl / a)
        jag = 1 + 0.28 * math.sin(5 * th + ph1) + 0.14 * math.sin(11 * th + ph2)
        return math.hypot(dl / a, ds / b) / jag

    # crack: zig-zag polyline from the chunk edge across the panel
    def to_ls(l, s): return (l, s)
    start = (Lc - el * a * 0.85, Sc - es * b * 0.15)
    end = (ctr[L] - el * ext[L] * 0.35, ctr[S] - es * ext[S] * 0.45)
    crack = [start]
    n = 6
    for i in range(1, n):
        f = i / n
        crack.append((start[0] + (end[0] - start[0]) * f + rng.uniform(-0.04, 0.04) * ext[L],
                      start[1] + (end[1] - start[1]) * f + rng.uniform(-0.12, 0.12) * ext[S]))
    crack.append(end)

    def crack_dist(l, s):
        best = 1e9
        for i in range(len(crack) - 1):
            (x0, y0), (x1, y1) = crack[i], crack[i + 1]
            dx, dy = x1 - x0, y1 - y0
            u = max(0.0, min(1.0, ((l - x0) * dx + (s - y0) * dy) / (dx * dx + dy * dy + 1e-9)))
            best = min(best, math.hypot(l - (x0 + dx * u), s - (y0 + dy * u)))
        return best

    kept = []
    for t in tris:
        cl = sum(p[L] for p in t.p) / 3
        cs = sum(p[S] for p in t.p) / 3
        if chunk_r(cl, cs) < 0.92:
            continue                       # knocked out
        if crack_dist(cl, cs) < CRACK_WIDTH * 0.5:
            continue                       # crack
        kept.append(t)

    bend_max = math.radians(BEND_MAX_DEG)
    zone = ext[L] * 0.5

    def deform(p):
        q = list(p)
        x = (q[L] - ctr[L]) * el
        y = (q[T] - ctr[T]) * front
        w = smoothstep(x / zone)           # 0 in the middle, 1 at the broken end
        th = bend_max * w
        x2 = x * math.cos(th) - y * math.sin(th)
        y2 = x * math.sin(th) + y * math.cos(th)
        q[L] = ctr[L] + x2 * el
        q[T] = ctr[T] + y2 * front
        # crumple close to the broken edge
        edge = chunk_r(p[L], p[S])
        if edge < 1.45:
            k = (1.45 - edge) / 0.45
            q[T] += front * (hash01(p, 1) - 0.4) * 1.1 * k
            q[L] += (hash01(p, 2) - 0.5) * 0.5 * k
            q[S] += (hash01(p, 3) - 0.5) * 0.5 * k
        return q

    for t in kept:
        t.p = [deform(p) for p in t.p]
    return kept, (L, S, T)


# ---------------------------------------------------------------- normals + fbx
def smooth_normals(tris, max_angle_deg=45.0):
    cos_lim = math.cos(math.radians(max_angle_deg))
    fn = [normalize(cross(sub(t.p[1], t.p[0]), sub(t.p[2], t.p[0]))) for t in tris]
    welded = {}
    for ti, t in enumerate(tris):
        for p in t.p:
            welded.setdefault((round(p[0], 2), round(p[1], 2), round(p[2], 2)), []).append(ti)
    normals = []
    for ti, t in enumerate(tris):
        cn = []
        for p in t.p:
            acc = [0.0, 0.0, 0.0]
            for tj in welded[(round(p[0], 2), round(p[1], 2), round(p[2], 2))]:
                if dot(fn[ti], fn[tj]) >= cos_lim:
                    acc = [acc[k] + fn[tj][k] for k in range(3)]
            cn.append(normalize(acc) if length(acc) > 1e-6 else fn[ti])
        normals.append(cn)
    return normals


def write_fbx(path, tris, n_mats):
    normals = smooth_normals(tris)
    verts, idx, nrm, uvs, mats = [], [], [], [], []
    for ti, t in enumerate(tris):
        for k in range(3):
            verts += t.p[k]
            nrm += normals[ti][k]
            uvs += [t.uv[k][0], 1.0 - t.uv[k][1]]   # FBX V is flipped vs UE
            i = len(idx)
            idx.append(i if k < 2 else -(i + 1))
        mats.append(t.mat)
    f = lambda a: ",".join(f"{x:.5f}" for x in a)
    mat_nodes = "".join(
        f'\tMaterial: {3000 + i}, "Material::M{i}", "" {{\n\t\tVersion: 102\n\t\tShadingModel: "phong"\n\t\tMultiLayer: 0\n\t}}\n'
        for i in range(n_mats))
    mat_conns = "".join(f'\tC: "OO",{3000 + i},2000\n' for i in range(n_mats))
    txt = f"""; FBX 7.4.0 project file
FBXHeaderExtension:  {{
	FBXHeaderVersion: 1003
	FBXVersion: 7400
	Creator: "create_broken_led_meshes_v2.py"
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
	Count: {2 + n_mats}
	ObjectType: "Geometry" {{
		Count: 1
	}}
	ObjectType: "Model" {{
		Count: 1
	}}
	ObjectType: "Material" {{
		Count: {n_mats}
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
			Normals: *{len(nrm)} {{
				a: {f(nrm)}
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
			MappingInformationType: "ByPolygon"
			ReferenceInformationType: "IndexToDirect"
			Materials: *{len(mats)} {{
				a: {",".join(str(m) for m in mats)}
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
{mat_nodes}}}
Connections:  {{
	C: "OO",2000,0
	C: "OO",1000,2000
{mat_conns}}}
"""
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(txt)


# ---------------------------------------------------------------- asset creation
def base_blend_mode(mat):
    base = mat
    while isinstance(base, unreal.MaterialInstanceConstant):
        base = base.get_editor_property("parent")
    return base.get_editor_property("blend_mode") if base else None


def make_wreck_material(base, mesh_name, slot, param, emissive):
    """Child of the original material for the broken mesh:
       * two sided, so the back of the fixture is not see-through,
       * glowing slots are switched off (emissive intensity 0); if they were additive/translucent
         light strips they become opaque dark plastic instead of vanishing,
       * real glass slots stay translucent."""
    name = f"MI_{mesh_name}_Mat{slot}"
    path = f"{OUT_DIR}/{name}"
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    mic = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, OUT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mic.set_editor_property("parent", base)
    ov = mic.get_editor_property("base_property_overrides")
    ov.set_editor_property("override_two_sided", True)
    ov.set_editor_property("two_sided", True)
    bm = base_blend_mode(base)
    if emissive and bm in (unreal.BlendMode.BLEND_ADDITIVE, unreal.BlendMode.BLEND_TRANSLUCENT):
        ov.set_editor_property("override_blend_mode", True)
        ov.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    mic.set_editor_property("base_property_overrides", ov)
    if emissive:
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mic, param, 0.0)
    unreal.MaterialEditingLibrary.update_material_instance(mic)
    EAL.save_loaded_asset(mic)
    unreal.log_warning(f"[BrokenV2] {name}: base blend={bm} emissive={emissive}")
    return mic


def make_mesh(name, src_mesh, materials, seed):
    asset_path = f"{OUT_DIR}/{name}"
    if EAL.does_asset_exist(asset_path):
        EAL.delete_asset(asset_path)
    base = read_tris(src_mesh)
    edge = MAX_EDGE
    tris = subdivide(base, edge)
    while len(tris) > MAX_TRIS:            # keep the wreck light: coarser cuts for big meshes
        edge *= 1.25
        tris = subdivide(base, edge)
    n_before = len(tris)
    tris, axes = break_tris(tris, seed)
    unreal.log_warning(f"[BrokenV2] {name}: {n_before} tris after subdivision -> {len(tris)} after breaking, axes(L,S,T)={axes}")

    fbx_path = os.path.join(tempfile.gettempdir(), name + ".fbx")
    write_fbx(fbx_path, tris, len(materials))

    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    params = unreal.ImportAssetParameters()
    params.set_editor_property("is_automated", True)
    params.set_editor_property("replace_existing", True)
    params.set_editor_property("destination_name", name)
    mgr.import_asset(OUT_DIR, unreal.InterchangeManager.create_source_data(fbx_path), params)
    mesh = EAL.load_asset(asset_path)
    if not mesh:
        raise RuntimeError("import failed: " + asset_path)

    n_slots = len(mesh.static_materials)
    unreal.log_warning(f"[BrokenV2] {name}: imported with {n_slots} material slots (wanted {len(materials)})")
    for i, m in enumerate(materials[:n_slots]):
        mesh.set_material(i, m)
    unreal.EditorStaticMeshLibrary.add_simple_collisions(mesh, unreal.ScriptingCollisionShapeType.BOX)
    EAL.save_loaded_asset(mesh)
    return mesh


def main():
    if not EAL.does_directory_exist(OUT_DIR):
        EAL.make_directory(OUT_DIR)
    for bp_path, mesh_name in TARGETS:
        bp = EAL.load_asset(bp_path)
        intact = get_template(bp, "IntactMesh")
        broken = get_template(bp, "BrokenMesh")
        if not (bp and intact and broken):
            unreal.log_error(f"[BrokenV2] cannot process {bp_path}")
            continue
        src_mesh = intact.get_editor_property("static_mesh")
        cdo = unreal.get_default_object(bp.generated_class())
        emissive = {m.get_editor_property("material_index") for m in cdo.get_editor_property("emissive_materials")}
        param = str(cdo.get_editor_property("emissive_intensity_parameter_name"))
        materials = []
        for i in range(len(src_mesh.static_materials)):
            base = src_mesh.get_material(i)
            materials.append(make_wreck_material(base, mesh_name, i, param, i in emissive) if base else base)
        new_mesh = make_mesh(mesh_name, src_mesh, materials, sum(ord(c) for c in mesh_name))
        nb = new_mesh.get_bounds()
        unreal.log_warning(f"[BrokenV2] {mesh_name} bounds extent={nb.box_extent} source extent={src_mesh.get_bounds().box_extent}")
        if DRY_RUN:
            continue
        broken.set_editor_property("static_mesh", new_mesh)
        add_cleanup_component(bp)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        EAL.save_loaded_asset(bp)
        unreal.log_warning(f"[BrokenV2] assigned {mesh_name} to {bp_path}")


main()
