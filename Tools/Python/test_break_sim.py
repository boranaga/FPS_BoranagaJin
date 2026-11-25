"""
Headless-ish test: simulate TestWorld in the editor, break a few LED actors and log where the
BrokenMesh fragments are over time. Does not save anything.
Run: UnrealEditor.exe <uproject> -ExecutePythonScript=<this file> -nullrhi -unattended
"""
import unreal

BP_CLASSES = ("BP_DestructibleLongLED_C", "BP_DestructibleLED_C")
SAMPLE_TIMES = [3.0]
END_TIME = 3.5

state = {"phase": "load", "t": 0.0, "ticks": 0, "targets": [], "next": 0, "break_t": None}
unreal.EditorLoadingAndSavingUtils.load_map("/Game/LevelS/Jin/TestWorld")


def pick_targets():
    out = []
    for cn in BP_CLASSES:
        actors = [a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
                  if a.get_class().get_name() == cn]
        step = max(1, len(actors) // 3)
        out += [a.get_path_name() for a in actors[::step][:3]]
    return out


def find_sim_actor(path):
    world = unreal.EditorLevelLibrary.get_game_world()
    name = path.split(".")[-1]
    for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        if a.get_name() == name:
            return a
    return None


def log_positions(tag):
    for p, a in state["targets"]:
        if not unreal.SystemLibrary.is_valid(a):
            unreal.log_warning(f"SIM {tag} {p.split('.')[-1]}: DESTROYED")
            continue
        bm = a.get_editor_property("broken_mesh")
        CC = unreal.CollisionChannel
        gt = sorted(n for n in dir(CC) if n.startswith("ECC_") or n[0].isupper())
        if not state.get("printed_gt"):
            state["printed_gt"] = True
            unreal.log_warning(f"SIM gamechannels={gt}")
        resp = [str(bm.get_collision_response_to_channel(c)).split(".")[-1] for c in (CC.ECC_PLAYER, CC.ECC_PAWN, CC.ECC_CAMERA, CC.ECC_CLIMB_WALL, CC.ECC_WORLD_STATIC)]
        loc = bm.get_world_location()
        unreal.log_warning(f"SIM {tag} {a.get_class().get_name()} {a.get_name()}: z={loc.z:.1f} (start z={state['start'][p]:.1f}) "
                           f"vis={bm.is_visible()} simulating={bm.is_simulating_physics()} vel={bm.get_physics_linear_velocity()} coll={bm.get_collision_enabled()} resp={resp}")


def tick(dt):
    try:
        _tick(dt)
    except Exception as ex:
        unreal.log_warning(f"SIM tick error: {ex}")
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()


def _tick(dt):
    state["ticks"] += 1
    if state["phase"] == "load":
        state["names"] = pick_targets()
        state["phase"] = "start"
        unreal.EditorLevelLibrary.editor_play_simulate()
        state["phase"] = "wait"
        return
    world = unreal.EditorLevelLibrary.get_game_world()
    if state["phase"] == "wait":
        if world is None:
            return
        state["wait_ticks"] = state.get("wait_ticks", 0) + 1
        if state["wait_ticks"] < 30:
            return
        for p in state["names"]:
            a = find_sim_actor(p)
            if a:
                state["targets"].append((p, a))
        state["start"] = {p: a.get_editor_property("broken_mesh").get_world_location().z for p, a in state["targets"]}
        unreal.log_warning(f"SIM targets={len(state['targets'])}")
        for p, a in state["targets"]:
            loc = a.get_actor_location()
            for lbl, off in (("down", unreal.Vector(0, 0, -300)), ("up", unreal.Vector(0, 0, 300)), ("fwd", unreal.Vector(300, 0, 0)), ("back", unreal.Vector(-300, 0, 0))):
                res = unreal.SystemLibrary.line_trace_single(world, loc, loc + off, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [a], unreal.DrawDebugTrace.NONE, True)
                if not res:
                    unreal.log_warning(f"SIM trace {a.get_name()} {lbl}: no hit")
                    continue
                t = res.to_tuple()
                for x in t:
                    if isinstance(x, unreal.PrimitiveComponent):
                        unreal.log_warning(f"SIM hitcomp {x.get_name()} objType={x.get_collision_object_type()} profile={x.get_collision_profile_name()} mobility={x.get_editor_property('mobility')} collEnabled={x.get_collision_enabled()} owner={x.get_owner().get_name()}")
                names = [x.get_path_name().split(".")[-1] + ("/" + x.get_name()) for x in t if isinstance(x, (unreal.Actor, unreal.PrimitiveComponent))]
                unreal.log_warning(f"SIM trace {a.get_name()} {lbl}: dist={t[3]:.1f} hit={names}")
            bmc = a.get_editor_property("broken_mesh")
            unreal.log_warning(f"SIM mob {a.get_name()}: root={a.get_editor_property('scene_root').get_editor_property('mobility')} bm={bmc.get_editor_property('mobility')} bmParent={bmc.get_attach_parent()} respStatic={bmc.get_collision_response_to_channel(unreal.CollisionChannel.ECC_WORLD_STATIC)} comps={[c.get_name() for c in a.get_components_by_class(unreal.PrimitiveComponent)]}")
            unreal.log_warning(f"SIM type {a.get_name()}: destruction_mesh_type={a.get_editor_property('destruction_mesh_type')} fractured_mesh={a.get_editor_property('fractured_mesh')}")
            a.call_method("BreakObject", args=(loc, unreal.Vector(0, 0, 1)))
        state["phase"] = "run"
        state["t"] = 0.0
        return
    if state["phase"] == "run":
        state["t"] += dt
        while state["next"] < len(SAMPLE_TIMES) and state["t"] >= SAMPLE_TIMES[state["next"]]:
            log_positions(f"t={SAMPLE_TIMES[state['next']]:.1f}")
            state["next"] += 1
        if state["t"] >= END_TIME:
            unreal.log_warning("SIM done")
            state["phase"] = "done"
            unreal.SystemLibrary.quit_editor()


handle = unreal.register_slate_post_tick_callback(tick)
