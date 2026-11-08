/* Only the opt-in Quest application includes this renderer. XR owns the
   framebuffer, projections and viewports; ordinary targets never compile it. */
static bool quest_fade_init(Renderer *r) {
    if (r->quest_fade)
        return true;
    GLuint vs =
        shader(GL_VERTEX_SHADER, "out vec2 p;void main(){p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);"
                                 "gl_Position=vec4(p*2.0-1.0,0,1);}");
    GLuint fs =
        shader(GL_FRAGMENT_SHADER, "in vec2 p;uniform vec3 uComfort;out vec4 result;void main(){"
                                   "float edge=smoothstep(.28,.67,length(p-.5));"
                                   "result=vec4(0,0,0,max(uComfort.x,edge*uComfort.y));}");
    if (!vs || !fs) {
        if (vs)
            gl.DeleteShader(vs);
        if (fs)
            gl.DeleteShader(fs);
        return false;
    }
    r->quest_fade = gl.CreateProgram();
    gl.AttachShader(r->quest_fade, vs);
    gl.AttachShader(r->quest_fade, fs);
    gl.LinkProgram(r->quest_fade);
    gl.DeleteShader(vs);
    gl.DeleteShader(fs);
    GLint ok = 0;
    gl.GetProgramiv(r->quest_fade, GL_LINK_STATUS, &ok);
    return ok != 0;
}
void renderer_quest_prepare(Renderer *r, Game *g) {
    r->quest_active = true;
    gl.UseProgram(r->program);
    gl.BindVertexArray(r->vao);
    upload_world(r, g);
    r->sprite_drawn[0] = r->tentacle_drawn[0] = 0;
}
/* Pixel-space glyphs become geometry on a readable, world-anchored panel. */
static void quest_panel(Renderer *r, const float *mvp, Vec3 center, Vec3 right, Vec3 up,
                        float pixels_w, float pixels_h, double size) {
    double scale = size / pixels_w;
    Vec3 origin = vsub(vadd(vsub(center, vmul(right, size * .5)), vmul(up, pixels_h * scale * .5)),
                       r->origin);
    float transform[16] = {(float)(right.x * scale),
                           (float)(right.y * scale),
                           (float)(right.z * scale),
                           0,
                           (float)(-up.x * scale),
                           (float)(-up.y * scale),
                           (float)(-up.z * scale),
                           0,
                           0,
                           0,
                           1,
                           0,
                           (float)origin.x,
                           (float)origin.y,
                           (float)origin.z,
                           1};
    float panel_mvp[16];
    multiply(panel_mvp, mvp, transform);
    gl.UniformMatrix4fv(r->mvp, 1, GL_FALSE, panel_mvp);
    gl.Uniform3f(r->offset, 0, 0, 0);
    gl.Uniform1i(r->ui, 1);
}
static void quest_hands(Renderer *r, const Game *g, const QuestView *q) {
    const Player *p = &g->players[0];
    const WeaponDef *weapon = game_gun(p);
    for (int hand = 0; hand < 2; hand++) {
        int pose = hand ? QUEST_AIM_RIGHT : QUEST_AIM_LEFT;
        Vec3 at = quest_position(&q->tracking, pose, q->base);
        Vec3 forward = quest_forward(&q->tracking, pose);
        Vec3 right = quest_axis(&q->tracking, pose, 0), up = quest_axis(&q->tracking, pose, 1);
        Color color = hand ? weapon->color : rgb(.55f, .65f, .7f);
        /* The barrel ends at the actual target-ray origin, where shots begin. */
        cube(&r->mesh, vsub(vsub(at, vmul(forward, .09)), r->origin), vmul(right, .055),
             vmul(up, .06), vmul(forward, .18), rgb(.12f, .17f, .19f), .1f);
        cube(&r->mesh, vsub(vsub(at, vmul(forward, .015)), r->origin), vmul(right, .06),
             vmul(up, .065), vmul(forward, .018), color, hand && p->muzzle > 0 ? 3 : .8f);
        cube(&r->mesh, vsub(vadd(vsub(at, vmul(forward, .12)), vmul(up, -.055)), r->origin),
             vmul(right, .04), vmul(up, .12), vmul(forward, .055), rgb(.09f, .12f, .14f), .1f);
    }
    flush(r);
}
static void quest_panels(Renderer *r, Game *g, const QuestView *q, const float *mvp) {
    Color ink = rgb(.8f, .9f, .87f), mint = rgb(.35f, 1, .8f), gold = rgb(1, .74f, .36f);
    Player *p = &g->players[0];
    char text[160];
    if (q->recovery || q->unsafe) {
        /* Large, head-following recovery text works before either hand is
           tracked and remains visible when the user turns to find them. */
        Vec3 right = quest_axis(&q->tracking, QUEST_HEAD, 0);
        Vec3 up = quest_axis(&q->tracking, QUEST_HEAD, 1);
        Vec3 center = vadd(q->head, vmul(quest_forward(&q->tracking, QUEST_HEAD), 1.5));
        quest_panel(r, mvp, center, right, up, 640, 360, 1.8);
        rect(r, 0, 0, 640, 360, rgb(.018f, .04f, .055f));
        frame(r, 0, 0, 640, 360, gold);
        if (q->unsafe && !q->recovery) {
            text_at(r, 26, 26, 3, "RETURN TO YOUR STARTING POINT", gold);
            text_at(r, 26, 100, 2.5f, "YOUR VIEW IS OUTSIDE THE PLAY AREA", ink);
            text_at(r, 26, 145, 2.5f, "OR TOO CLOSE TO A WALL.", ink);
            text_at(r, 26, 220, 2.5f, "LEAN BACK, OR CLICK THE RIGHT", mint);
            text_at(r, 26, 265, 2.5f, "STICK TO RECENTER YOUR VIEW.", mint);
            flush(r);
            return;
        }
        text_at(r, 26, 26, 3, q->controllers ? "TRACKING READY" : "WAITING FOR CONTROLLERS", gold);
        unsigned left = (1u << QUEST_LEFT) | (1u << QUEST_AIM_LEFT);
        unsigned right_mask = (1u << QUEST_RIGHT) | (1u << QUEST_AIM_RIGHT);
        snprintf(text, sizeof(text), "LEFT: %s / RIGHT: %s",
                 (q->tracking.valid & left) == left ? "READY" : "WAITING",
                 (q->tracking.valid & right_mask) == right_mask ? "READY" : "WAITING");
        text_at(r, 26, 85, 2.5f, text, mint);
        text_at(r, 26, 145, 2.5f, "HOLD BOTH CONTROLLERS IN VIEW.", ink);
        text_at(r, 26, 190, 2.5f, "RELEASE BUTTONS AND STICKS.", ink);
        text_at(r, 26, 235, 2.5f, "PRESS A WHEN TRACKING IS READY.", gold);
        text_at(r, 26, 300, 2, "RIGHT STICK CLICK: RECENTER", mint);
        flush(r);
        return;
    }
    if (q->menu) {
        quest_panel(r, mvp, q->menu_center, q->menu_right, q->menu_up, 960, 650, 2.0);
        rect(r, 0, 0, 960, 650, rgb(.012f, .025f, .034f));
        frame(r, 0, 0, 960, 650, gold);
        text_at(r, 24, 22, 2.5f, q->title, gold);
        snprintf(text, sizeof(text), "HP %.0f / SHIELD %.0f / ARMOR %.0f / ENERGY %.0f", p->health,
                 p->shield, p->armor, p->energy);
        text_at(r, 24, 58, 1.3f, text, mint);
        for (int i = 0; i < q->line_count; i++)
            text_at(r, 24, 96 + i * 32, 1.8f, q->lines[i], ink);
        if (p->map_open) {
            if (g->matrix.divers[0].active)
                matrix_schematic(r, g, 0, 40, 140, 340);
            else
                deck_map(r, g, 0, 40, 140, 340, true);
        }
        for (int i = 0; i < q->item_count; i++) {
            float y = 96 + i * 55;
            if (i == q->selected)
                rect(r, 486, y - 7, 454, 51, rgb(.06f, .18f, .19f));
            wrapped(r, 498, y, 1.8f, 428, q->items[i], i == q->selected ? gold : ink);
        }
        text_at(r, 24, 580, 1.35f, q->footer, mint);
        text_at(r, 24, 610, 1.6f, "RIGHT STICK SELECT / A CONFIRM / B BACK / Y RESUME", ink);
        flush(r);
    } else if (q->wrist) {
        Vec3 at = vadd(quest_position(&q->tracking, QUEST_LEFT, q->base), v3(0, .12, 0));
        Vec3 forward = vnorm(vsub(at, q->head));
        Vec3 right = vnorm(vcross(v3(0, 1, 0), forward));
        if (vlen(right) < .1)
            right = quest_axis(&q->tracking, QUEST_HEAD, 0);
        Vec3 up = vcross(forward, right);
        quest_panel(r, mvp, at, right, up, 400, 370, .34);
        rect(r, 0, 0, 400, 370, rgb(.012f, .03f, .04f));
        if (g->matrix.divers[0].active)
            snprintf(text, sizeof(text), "INT %.0f / BW %.0f / TRACE %.0f",
                     g->matrix.divers[0].integrity, g->matrix.divers[0].bandwidth,
                     g->matrix.divers[0].trace);
        else if (p->riding)
            snprintf(text, sizeof(text), "%s / HULL %.0f / HP %.0f",
                     p->riding == MECH ? "MECH" : "WAGON", g->rigs[0][p->riding - 1].armor,
                     p->health);
        else
            snprintf(text, sizeof(text), "HP %.0f / SH %.0f / AR %.0f", p->health, p->shield,
                     p->armor);
        text_at(r, 12, 14, 1.5f, text, mint);
        if (g->matrix.divers[0].active)
            matrix_schematic(r, g, 0, 86, 74, 220);
        else
            deck_map(r, g, 0, 86, 74, 220, false);
        text_at(r, 12, 325, 1.15f, game_gun(p)->name, gold);
        flush(r);
    }
    if (!q->menu && g->message_time > 0) {
        Vec3 forward = quest_forward(&q->tracking, QUEST_HEAD);
        Vec3 right = quest_axis(&q->tracking, QUEST_HEAD, 0),
             up = quest_axis(&q->tracking, QUEST_HEAD, 1);
        Vec3 at = vadd(q->head, vadd(vmul(forward, 1.5), vmul(up, -.65)));
        quest_panel(r, mvp, at, right, up, 640, 110, 1.5);
        rect(r, 0, 0, 640, 110, rgb(.015f, .03f, .035f));
        wrapped(r, 18, 18, 2, 600, g->message, gold);
        flush(r);
    }
}
bool renderer_quest_eye(Renderer *r, Game *g, const QuestView *q, GLuint framebuffer, int x, int y,
                        int width, int height, const float *projection,
                        const float *tracking_view) {
    if (width < 1 || height < 1 || !quest_projection_valid(projection))
        return false;
    Player *p = &g->players[0];
    bool matrix = g->matrix.divers[0].active;
    bool outside = !matrix && !p->sheltered && position_absolute(p->pos).z < 0;
    r->origin =
        matrix || p->sheltered ? v3(0, 0, 0) : world_center(world_cell(position_absolute(p->pos)));
    float view[16], mvp[16];
    if (!quest_view(&q->tracking, tracking_view, q->base, r->origin, view))
        return false;
    multiply(mvp, projection, view);
    gl.BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    gl.Viewport(x, y, width, height);
    gl.Enable(GL_SCISSOR_TEST);
    gl.Scissor(x, y, width, height);
    gl.ClearColor(.001f, .002f, .004f, 1);
    gl.DepthMask(GL_TRUE);
    gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl.Disable(GL_BLEND);
    gl.Enable(GL_DEPTH_TEST);
    gl.UseProgram(r->program);
    gl.BindVertexArray(r->vao);
    gl.Uniform1i(r->ui, 0);
    gl.Uniform3f(r->offset, 0, 0, 0);
    gl.UniformMatrix4fv(r->mvp, 1, GL_FALSE, mvp);
    Vec3 eye = vsub(q->head, r->origin);
    gl.Uniform3f(r->eye, (float)eye.x, (float)eye.y, (float)eye.z);
    gl.Uniform3f(r->grid, (float)fmod(r->origin.x, 4096), (float)fmod(r->origin.y, 4096),
                 (float)fmod(r->origin.z, 4096));
    AtmosphereDef atmosphere = sc_atmospheres[STILL_AIR];
    if (g->world.generator >= 10 && !outside && !matrix && !p->sheltered) {
        Cell cell = world_cell(position_absolute(p->pos));
        RoomSpec room = world_describe_version(g->world.seed, g->world.generator, cell);
        atmosphere = sc_atmospheres[room.habitat.atmosphere];
        const FactionRegion *region = g->factions.version ? factions_at(&g->factions, cell) : NULL;
        if (region && (region->flags & FS_GROWTH))
            atmosphere = sc_atmospheres[SPORE_AIR];
        const Room *live = world_find(&g->world, cell);
        if (live && live->faction_edit == EDIT_WEATHER)
            atmosphere = sc_atmospheres[VOID_GLOW];
    }
    gl.Uniform1f(r->fog_density, outside ? 0 : matrix || p->sheltered ? .009f : atmosphere.density);
    gl.Uniform3f(r->fog_color, atmosphere.fog.r, atmosphere.fog.g, atmosphere.fog.b);
    lights(r, g, p);
    Vec3 lamp = vsub(quest_position(&q->tracking, QUEST_AIM_LEFT, q->base), r->origin);
    Vec3 direction = quest_forward(&q->tracking, QUEST_AIM_LEFT);
    r->quest_lamp_origin = lamp;
    r->quest_lamp_direction = direction;
    GLfloat torch[] = {(float)direction.x, (float)direction.y, (float)direction.z,
                       p->flashlight && !matrix ? 1.f : 0.f};
    gl.Uniform4fv(r->torch, 1, torch);
    gl.Uniform3f(r->torch_origin, (float)lamp.x, (float)lamp.y, (float)lamp.z);
    if (!q->unsafe && !q->recovery) {
        if (matrix)
            matrix_scene(r, g, 0);
        else if (p->sheltered)
            fold_interior(r, g, 0);
        else {
            starfield(r, p);
            draw_chunks(r, g);
            hull_view(r, g, q->head);
            boarding_skiff(r, g, q->head);
            if (!outside)
                infestation_tentacles(r, g, 0);
            actors(r, g, 0);
            campaign_scene(r, g, 0);
            if (!outside) {
                navigation_scene(r, g, 0);
                encounter_scene(r, g, 0);
                politics_scene(r, g, 0);
            }
        }
        flush(r);
        if (!outside && !p->sheltered && !matrix && g->world.generator >= 10)
            atmosphere_draw(r, g, 0, mvp, eye, atmosphere);
        if (!q->menu && q->controllers)
            quest_hands(r, g, q);
    }
    gl.Disable(GL_DEPTH_TEST);
    if (!quest_fade_init(r))
        return false;
    gl.UseProgram(r->quest_fade);
    gl.Uniform3f(gl.GetUniformLocation(r->quest_fade, "uComfort"), q->unsafe ? 1 : q->fade,
                 q->vignette ? q->movement * .85f : 0, 0);
    gl.Enable(GL_BLEND);
    gl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl.DrawArrays(GL_TRIANGLES, 0, 3);
    gl.Disable(GL_BLEND);
    gl.UseProgram(r->program);
    quest_panels(r, g, q, mvp);
    gl.Uniform1i(r->ui, 0);
    gl.Disable(GL_SCISSOR_TEST);
    gl.Enable(GL_DEPTH_TEST);
    if (r->mesh.failed)
        r->failed = true;
    GLenum error = gl.GetError();
    if (r->failed || error != GL_NO_ERROR)
        SDL_Log("Quest eye failed: GL 0x%x / renderer %d / mesh %d", error, r->failed,
                r->mesh.failed);
    return !r->failed && error == GL_NO_ERROR;
}
