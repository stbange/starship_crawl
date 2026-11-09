#include "app_internal.h"

void audio_frame(void) {
    if (!app.audio)
        return;
    if (app.paused) {
        SDL_ClearAudioStream(app.audio);
        app.audio_shots = app.game.shots;
        app.shot_sample = 1800;
        return;
    }
    if (app.audio_shots != app.game.shots) {
        app.audio_shots = app.game.shots;
        app.shot_sample = 0;
        app.audio_noise = 27;
    }
    float target[DISTRICT_TYPES] = {0};
    for (int i = 0; i < app.game.player_count; i++) {
        Vec3 feet = position_absolute(app.game.players[i].pos);
        Room *room = world_find(&app.game.world, world_cell(feet));
        if (feet.z >= 0 && room && room->ready)
            target[room->district] += (room->type == CONDUIT ? .45f : 1.0f) / app.game.player_count;
    }
    /* District drones share the mix. Creature cues pan relative to a solo view;
       split-screen cues favour the channel belonging to the affected viewport. */
    double warning[ALIEN_BEHAVIORS][2] = {{0}};
    for (int room = 0; room < app.game.world.count; room++)
        for (int n = 0; n < app.game.world.rooms[room].enemy_count; n++) {
            const Enemy *e = &app.game.world.rooms[room].enemies[n];
            unsigned voice = e->behavior;
            double strength = e->warning_sound;
            if (app.game.factions.version && e->country) {
                voice = ALIEN_CONDUCTOR + politics_government(app.game.world.seed, e->country - 1);
                const FactionForce *force = factions_force(&app.game, e->force);
                if (force && force->physical)
                    strength = fmax(strength, force->order == ORDER_RETREAT ? .24 : .10);
                for (int player = 0; player < app.game.player_count; player++) {
                    const FactionIncident *incident =
                        &app.game.factions.incidents[player][e->country - 1];
                    if (incident->strikes == 1 && incident->grace > 0 &&
                        cell_equal(world_region(incident->site),
                                   world_region(world_cell(position_absolute(e->pos)))))
                        strength = fmax(strength, incident->grace);
                }
            }
            if (!e->alive || !voice || strength <= 0)
                continue;
            for (int player = 0; player < app.game.player_count; player++) {
                const Player *p = &app.game.players[player];
                if (p->sheltered || app.game.matrix.divers[player].active)
                    continue;
                Vec3 delta = vsub(enemy_center(e), player_eye(p));
                double distance = vlen(delta);
                if (distance < 35) {
                    double pan = app.game.player_count == 2
                                     ? (player ? .8 : -.8)
                                     : vdot(vnorm(delta), v3(cos(p->yaw), 0, -sin(p->yaw)));
                    for (int channel = 0; channel < 2; channel++) {
                        double gain = .75 * (1 + (channel ? pan : -pan) * .8);
                        warning[voice][channel] =
                            fmax(warning[voice][channel], (1 - distance / 35) * strength * gain);
                    }
                }
            }
        }
    int queued = SDL_GetAudioStreamQueued(app.audio);
    for (int block = 0; block < 4 && queued < 16384; block++) {
        float samples[1024];
        for (int i = 0; i < 512; i++) {
            double t = app.audio_time;
            for (int d = 0; d < DISTRICT_TYPES; d++)
                app.ambience[d] += (target[d] - app.ambience[d]) * .0005f;
            double ambient = app.ambience[HABITAT] *
                             (sin(t * SC_PI * 2 * 54) + .3 * sin(t * SC_PI * 2 * 81)) * .009;
            ambient += app.ambience[MACHINERY] *
                       (sin(t * SC_PI * 2 * 37.5) + .45 * sin(t * SC_PI * 2 * 112.5)) * .014;
            ambient += app.ambience[SANCTUM] *
                       (sin(t * SC_PI * 2 * 65.4) + sin(t * SC_PI * 2 * 98.1)) * .007;
            for (int d = LUXURY; d < DISTRICT_TYPES; d++)
                ambient +=
                    app.ambience[d] *
                    (sin(t * SC_PI * 2 * (42 + d * 5)) + .25 * sin(t * SC_PI * 2 * (84 + d * 7))) *
                    .007;
            double cues[2] = {0};
            for (int k = 1; k < ALIEN_BEHAVIORS; k++)
                if (warning[k][0] > 0 || warning[k][1] > 0) {
                    double pulse = pow(fmax(0, sin(t * (k == ALIEN_MACHINE ? 28 : 12 + k))), 6);
                    double tone = .024 * pulse * sin(t * SC_PI * 2 * (95 + k * 37));
                    for (int channel = 0; channel < 2; channel++)
                        cues[channel] += warning[k][channel] * tone;
                }
            double shot = 0;
            if (app.shot_sample < 1800) {
                double age = (double)app.shot_sample++ / 22050;
                app.audio_noise = app.audio_noise * 1664525u + 1013904223u;
                shot = (sin(age * (1600 - age * 5000)) * .11 +
                        ((double)(app.audio_noise % 1000) / 500 - 1) * .05) *
                       exp(-age * 65);
            }
            for (int channel = 0; channel < 2; channel++)
                samples[i * 2 + channel] = (float)clampd(ambient + shot + cues[channel], -.95, .95);
            app.audio_time += 1.0 / 22050;
        }
        SDL_PutAudioStreamData(app.audio, samples, sizeof(samples));
        queued += (int)sizeof(samples);
    }
}
