#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A player driver: no position writes, damage calls, equipment grants or enemy
   removal. Plans only read the generator; all progress uses ordinary controls. */
static Cell next_cell(Game *g, Cell from, Cell goal) {
    enum { WIDTH=32,DEPTH=32,N=WIDTH*DEPTH };
    static int ox,oz,deck=-9999;
    static uint8_t doors[N];
    int parent[N],queue[N],head=0,tail=0;
    if(deck!=from.y || from.x<ox+8 || from.x>=ox+24 || from.z<oz+8 || from.z>=oz+24) {
        ox=(from.x/4)*4-16;oz=from.z<16?0:(from.z/4)*4-16;deck=from.y;
        for(int rz=0;rz<DEPTH/4;rz++)for(int rx=0;rx<WIDTH/4;rx++) {
            RegionPlan plan;world_plan_version(g->world.seed,g->world.generator,(Cell){ox/4+rx,deck,oz/4+rz},&plan);
            for(int z=0;z<4;z++)for(int x=0;x<4;x++)doors[(rz*4+z)*WIDTH+rx*4+x]=plan.doors[z*4+x];
        }
    }
    goal.x=(int)clampd(goal.x,from.x-7,from.x+7);goal.z=(int)clampd(goal.z,from.z-7,from.z+7);
    for (int i = 0; i < N; i++)
        parent[i] = -1;
    int start = (from.z-oz)*WIDTH+from.x-ox, end = (goal.z-oz)*WIDTH+goal.x-ox;
    if (start < 0 || start >= N || end < 0 || end >= N)
        return from;
    queue[tail++] = start;
    parent[start] = start;
    while (head < tail && parent[end] < 0) {
        int at = queue[head++];
        Cell c = {at % WIDTH + ox, from.y, at / WIDTH + oz};
        for (int d = 0; d < 4; d++) {
            Cell n = world_neighbor(c, (Direction)d);
            if (n.x < ox || n.x >= ox+WIDTH || n.z < oz || n.z >= oz+DEPTH || !(doors[at] & (1u << d)) ||
                (g->world.open_layer && campaign_layer(n) > g->world.open_layer))
                continue;
            int ni = (n.z-oz)*WIDTH+n.x-ox;
            if (parent[ni] >= 0)
                continue;
            parent[ni] = at;
            queue[tail++] = ni;
        }
    }
    if (parent[end] < 0)
        return from;
    int at = end;
    while (at != start && parent[at] != start)
        at = parent[at];
    return (Cell){at % WIDTH + ox, from.y, at / WIDTH + oz};
}
static bool capture(Game *g, const char *directory, const char *name) {
    if (!directory)
        return true;
    char path[512];
    snprintf(path, sizeof(path), "%s/phase-%s.sav", directory, name);
    return game_save(g, path);
}
static int journey(unsigned seed, bool port, const char *fixtures, int route) {
    Game *g = malloc(sizeof(*g));
    if (!g)
        return 1;
    game_init(g, seed);char resume_path[512];snprintf(resume_path,sizeof(resume_path),"%s/resume.sav",fixtures);
    Vec3 waypoint = {0};
    bool have = false, resumed = false, pending = false, attempt = false;
    Cell previous = {999, 999, 999};
    unsigned previous_respawns = 0, previous_defeated = 0;
    unsigned frames,stage=1,maxlayer=1,recoveries=0;
    Vec3 probe=v3(0,0,-30);unsigned still=0;double recover_until=0;
    static const int dirs[10][3]={{0,0,1},{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{1,0,1},{-1,0,1},{0,1,1},{0,-1,1},{1,-1,1}};
    Cell last_cell={99999,99999,99999};

    for (frames = 0;
         frames < 216000 && !g->world.failed;
         frames++) {
        Player *p = &g->players[0];
        Vec3 stream_position = position_absolute(p->pos);
        world_refresh(&g->world, &stream_position, 1);
        world_stream(&g->world, SC_STREAM_BUDGET);
        Cell goal = {0};
        unsigned phase = campaign_phase(&g->campaign);
        bool briefing = (g->story.done & 1) != 0;
        if (briefing && !(g->campaign.defeated & (1u << 8)))
            goal = campaign_site(8);
        else if (briefing && phase < 4 &&
                 (phase == 0 || (g->story.assigned & (1u << game_story_target(g)))))
            goal = campaign_site(phase * 2 + port);
        else {
            while (g->campaign.tracked != SC_GUARDIANS)
                campaign_track(g);
            game_objective(g, &goal);
        }
        if(phase==4) {
            goal=(Cell){dirs[route][0]*(int)stage,dirs[route][1]*(int)(stage/2),2+dirs[route][2]*(int)stage};
            Vec3 at=position_absolute(p->pos);
            if(cell_equal(world_cell(at),goal) && vlen(vsub(at,world_lift(goal)))<3) {stage++;have=false;}
        }
        if (!cell_equal(previous, goal) || previous_respawns != g->respawns)
            have = false;
        previous = goal;
        previous_respawns = g->respawns;
        if (g->campaign.defeated != previous_defeated) {
            printf("Seed %u: phase %u, guardians %x, rank %u, %u kills, %u respawns at %.1fs\n",
                   seed, phase, g->campaign.defeated, p->level, p->kills, g->respawns, g->time);
            fflush(stdout);
            previous_defeated = g->campaign.defeated;
            campaign_journal(g, 0);
            if (!pending) {
                if (!capture(g, fixtures, "reward"))
                    return 1;
                pending = true;
            }
            if (!campaign_choose(g, 0, phase % 3 + 1))
                return 1;
            p->journal = false; /* Closing the player's journal; no simulation mutation. */
            if (phase == 2 && !resumed) {
                if (!game_save(g, resume_path) || !game_load(g, resume_path)) {
                    fprintf(stderr,
                            "Resume refused: phase=%d story=%d matrix=%d scene=%u mission=%u\n",
                            campaign_valid(&g->campaign, g->players, &g->world),
                            story_valid(&g->story, seed, g->world.generator, g->mission),
                            matrix_valid(&g->matrix, seed, 7, g->players, &g->shelter),
                            g->story.tracked, g->mission);
                    return 1;
                }
                remove(resume_path);
                resumed = true;
                have = false;
                if (!capture(g, fixtures, "midship"))
                    return 1;
            }
            if (phase == 4 && !capture(g, fixtures, "unbound"))
                return 1;
        }
        while (p->points || p->credits >= 60 + p->power * 25) {
            int attr = p->power < 10 ? 0 : p->vitality < 8 ? 1 : 3;
            if (*player_attribute(p, attr) >= 12 || !game_upgrade(g, 0, attr))
                break;
        }
        /* The basic gun remains equipped even when the second slot earns loot. */
        for (int i = 0; p->weapon != 0 && i < SC_WEAPONS; i++)
            game_weapon(g, 0, 1);
        Vec3 feet = position_absolute(p->pos);
        Cell cell = world_cell(feet);
        Actions actions[2] = {0};
        int active = -1;
        for (unsigned i = 0; i < SC_GUARDIANS; i++)
            if (g->campaign.attempts[i].active)
                active = (int)i;
        if (active >= 0) {
            if (!attempt) {
                if (!capture(g, fixtures, "attempt"))
                    return 1;
                attempt = true;
            }
            /* Keep moving across the locked firing line in the central clear lane. */
            Vec3 center = world_center(campaign_site((unsigned)active));
            waypoint = vadd(center, v3(sin(g->time * .65) * 10, 0, cos(g->time * .65) * 10));
            have = false;
        } else if (!have) {
            if (cell.y != goal.y)
                waypoint = world_lift(cell);
            else if (cell.x == goal.x && cell.z == goal.z)
                waypoint = phase==4?world_lift(goal):game_terminal(goal);
            else
                waypoint = world_center(next_cell(g, cell, goal));
            have = true;
        }
        Vec3 diff = vsub(waypoint, feet);
        diff.y = 0;
        double distance = vlen(diff);
        if (active < 0 && distance < .65) {
            have = false;
            if (cell.y != goal.y) {
                actions[0].lift_up = cell.y < goal.y;
                actions[0].lift_down = cell.y > goal.y;
            } else if (cell.x == goal.x && cell.z == goal.z) {
                game_interact(g, 0);
                if (p->decision_open) {
                    if (!capture(g, fixtures, "decision"))
                        return 1;
                    if (!game_story_choose(g, 0, 1))
                        return 1;
                }
            }
        }
        Vec3 aim = vnorm(diff);
        double nearest = 70;
        for (int r = 0; r < g->world.count; r++)
            for (int e = 0; e < g->world.rooms[r].enemy_count; e++) {
                Room *room = &g->world.rooms[r];
                Enemy *enemy = &room->enemies[e];
                int guardian = campaign_guardian(room->cell);
                if (!enemy->alive || (guardian >= 0 && !g->campaign.attempts[guardian].active))
                    continue;
                Vec3 delta = vsub(enemy_center(enemy), player_eye(p));
                double range = vlen(delta);
                if (range < nearest &&
                    world_raycast(&g->world, player_eye(p), vnorm(delta), range) >= range - .1) {
                    nearest = range;
                    aim = vnorm(delta);
                    actions[0].fire = true;
                }
            }
        game_look(p, atan2(aim.x, aim.z) - p->yaw, asin(clampd(aim.y, -1, 1)) - p->pitch);
        Vec3 move = vnorm(diff), forward = v3(sin(p->yaw), 0, cos(p->yaw)),
             right = v3(cos(p->yaw), 0, -sin(p->yaw));
        if (distance > .65) {
            actions[0].forward = (float)vdot(move, forward);
            actions[0].strafe = (float)vdot(move, right);
        }
        actions[0].crouch =
            world_describe_version(seed, g->world.generator, cell).height < 2 ||
            (distance > 1 &&
             world_describe_version(seed, g->world.generator, world_cell(waypoint)).height < 2);
        if(frames%60==0) {
            if(vlen(vsub(feet,probe))<.3)still++;else still=0;
            probe=feet;
            if(still>=5) { recover_until=g->time+2;still=0;recoveries++;have=false;
                printf("RECOVERY,%.2f,%u,%.2f,%.2f,%.2f\n",g->time,recoveries,feet.x,feet.y,feet.z); }
        }
        if(g->time<recover_until) {
            actions[0].strafe=(recoveries%2)?1:-1;actions[0].forward=.5f;actions[0].jump=true;
        }
        if(!cell_equal(cell,last_cell)) {
            RoomSpec r=world_describe_version(seed,g->world.generator,cell);ShipTraits t=campaign_traits(seed,cell);
            printf("CELL,%.2f,%d,%d,%d,%u,%u,%u,%u,%d,%d,%u\n",g->time,cell.x,cell.y,cell.z,campaign_layer(cell),r.theme,r.habitat.form,r.habitat.atmosphere,t.heat,t.order,t.alien);
            last_cell=cell;
            if(campaign_layer(cell)>maxlayer) {maxlayer=campaign_layer(cell);if(!capture(g,fixtures,"deepest"))return 2;}
        }
        game_step(g, actions, 1.0 / 60);
        if (g->world.last_work > SC_STREAM_BUDGET || !isfinite(p->health) || !isfinite(p->energy) ||
            g->respawns > 1000)
            break;
        if (frames % 1800 == 0 || frames == 215999) {
            Room *observed=world_find(&g->world,world_cell(position_absolute(p->pos)));
            printf("SAMPLE,%u,%.2f,%.2f,%.2f,%.2f,%u,%u,%u,%u,%u,%u,%.2f,%.2f,%u,%u,%u,%u\n",
                seed,g->time,feet.x,feet.y,feet.z,p->kills,g->respawns,p->level,
                campaign_phase(&g->campaign),g->campaign.defeated,p->weapons,p->health,p->energy,
                g->shots,g->hits,observed?observed->theme:0,observed?observed->habitat.atmosphere:0);
            fflush(stdout);
        }
        if(frames % 54000 == 0 || frames == 215999) {
            char name[80];snprintf(name,sizeof(name),"minute-%02u",frames==215999?60:frames/3600);
            if(!capture(g,fixtures,name))return 2;
        }
        if (frames % 24000 == 0) {
            printf("Tick %u: %.1f %.1f %.1f -> [%d,%d,%d], %s\n", frames, feet.x, feet.y, feet.z,
                   goal.x, goal.y, goal.z, g->message);
            fflush(stdout);
        }
    }
    printf("DEPTH route=%d maxlayer=%u stage=%u recoveries=%u\n",route,maxlayer,stage,recoveries);
    bool ok = frames == 216000 && !g->world.failed;
    printf("END seed=%u sim=%.2f kills=%u deaths=%u rank=%u phase=%u\n", seed, g->time,g->players[0].kills,g->respawns,g->players[0].level,campaign_phase(&g->campaign));
    game_destroy(g);
    free(g);
    return ok ? 0 : 1;
}
int main(int argc,char **argv) {if(argc!=3)return 2;return journey(713,false,argv[1],atoi(argv[2]));}
