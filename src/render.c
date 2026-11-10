#include "render_internal.h"

static const char *vertex_source = "layout(location=0) in vec3 aPosition;\n"
                                   "layout(location=1) in vec3 aNormal;\n"
                                   "layout(location=2) in vec3 aColor;\n"
                                   "layout(location=3) in float aGlow;\n"
                                   "uniform mat4 uMVP;uniform vec3 uOffset;out vec3 vPosition;out "
                                   "vec3 vNormal;out vec3 vColor;out float vGlow;\n"
                                   "void "
                                   "main(){vPosition=aPosition+uOffset;vNormal=aNormal;vColor="
                                   "aColor;vGlow=aGlow;gl_Position=uMVP*vec4("
                                   "vPosition,1.0);}\n";
static const char *fragment_source =
    "in vec3 vPosition;in vec3 vNormal;in vec3 vColor;in float vGlow;\n"
    "uniform vec3 uEye;uniform vec3 uGrid;uniform int uUI;uniform float uTime;uniform float "
    "uFogDensity;uniform vec3 uFogColor;\n"
    "uniform vec4 uTorch;uniform int uLampCount;uniform vec4 uLampPos[12];uniform vec4 "
    "uLampColor[12];out vec4 result;\n"
#ifdef SC_WEBXR
    "uniform vec3 uTorchOrigin;\n"
#endif
    "float lamp(float phase){if(mod(phase,4.0)<0.5)return 0.0;float "
    "t=mod(uTime+phase*.37,7.0+phase*.13);if(t<.3)return .06;if(t<.55)return .4;if(t<1.1)return "
    ".02;return .75+.25*sin(t*2.0+phase);}\n"
    "void main(){if(uUI==1){result=vec4(vColor,1.0);return;}\n"
    " vec3 n=normalize(vNormal);vec3 p=vPosition+uGrid;vec3 fromEye=vPosition-uEye;\n"
    " float distance=length(fromEye);vec3 view=-fromEye/max(distance,.001);\n"
    " vec3 illumination=vec3(.045,.055,.062)*(0.45+0.55*max(n.y,0.0));\n"
    " for(int i=0;i<12;i++){if(i>=uLampCount)break;vec3 delta=uLampPos[i].xyz-vPosition;float "
    "d2=dot(delta,delta);float radius=uLampPos[i].w;\n"
    "  if(d2<radius*radius){float d=sqrt(d2);float falloff=pow(1.0-d/radius,2.0);float "
    "diffuse=max(dot(n,delta/max(d,.001)),0.0);\n"
    "   illumination+=uLampColor[i].rgb*uLampColor[i].w*falloff*(.12+diffuse);}}\n"
#ifdef SC_WEBXR
    " vec3 fromTorch=vPosition-uTorchOrigin;float torchDistance=length(fromTorch);\n"
    " float cone=smoothstep(.79,.95,dot(fromTorch/max(torchDistance,.001),uTorch.xyz));\n"
    " float beam=uTorch.w*cone/(1.0+.0025*torchDistance*torchDistance)*"
    "max(0.0,1.0-torchDistance/70.0);\n"
    " illumination+=vec3(1.5,1.35,1.05)*beam*(.15+.95*"
    "max(dot(n,-fromTorch/max(torchDistance,.001)),0.0));\n"
#else
    " float cone=smoothstep(.79,.95,dot(fromEye/max(distance,.001),uTorch.xyz));\n"
    " float beam=uTorch.w*cone/(1.0+.0025*distance*distance)*max(0.0,1.0-distance/70.0);\n"
    " illumination+=vec3(1.5,1.35,1.05)*beam*(.15+.95*max(dot(n,view),0.0));\n"
#endif
    " vec2 tile=abs(n.y)>.5?p.xz:(abs(n.x)>.5?p.zy:p.xy);vec2 panel=fract(tile/3.0);\n"
    " float seam=step(panel.x,.018)+step(panel.y,.018);\n"
    " float grain=fract(sin(dot(floor(p*9.0),vec3(12.9898,78.233,39.425)))*43758.5453);\n"
    " float stain=smoothstep(.1,.8,sin(p.x*.21+sin(p.z*.17)*2.0)*sin(p.z*.29+p.y*.41));\n"
    " vec3 surface=vColor*(.65+grain*.26)*(1.0-stain*.38);\n"
    " if(vGlow>=0.0&&vGlow<.1){surface*=1.0-min(seam,1.0)*.48;surface=mix(surface,vec3(.10,.046,."
    "023),stain*.28);}\n"
    " if(vGlow<0.0)surface*=.75+.25*sin(p.x*14.0+p.z*9.0+sin(p.y*7.0));\n"
    " vec3 color=surface*illumination;\n"
    " float emissive=vGlow>=4.0?lamp(floor(vGlow-4.0))*.8:max(vGlow,0.0)*.55;\n"
    " color+=vColor*emissive;\n"
    " if(vGlow<0.0)color+=vec3(.13,.16,.12)*pow(max(dot(n,view),0.0),18.0)*beam*.16;\n"
    " float fog=1.0-exp(-distance*uFogDensity);color=mix(color,uFogColor,fog);\n"
    " color=pow(max(color,vec3(0.0)),vec3(.75));\n"
    " float d=mod(gl_FragCoord.x,2.0)*.25+mod(gl_FragCoord.y,2.0)*.5;\n"
    " color=floor(color*42.0+d)/42.0;result=vec4(color,1.0);}\n";

GLuint sc_render_shader(GLenum type, const char *source) {
#ifdef __EMSCRIPTEN__
    const char *header = "#version 300 es\nprecision highp float;\n";
#else
    const char *header = "#version 330 core\n";
#endif
    GLuint id = gl.CreateShader(type);
    const char *sources[] = {header, source};
    gl.ShaderSource(id, 2, sources, NULL);
    gl.CompileShader(id);
    GLint ok = 0;
    gl.GetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        gl.GetShaderInfoLog(id, sizeof(log), NULL, log);
        SDL_Log("Shader: %s", log);
        gl.DeleteShader(id);
        return 0;
    }
    return id;
}

bool renderer_init(Renderer *r) {
    memset(r, 0, sizeof(*r));
    preferences_default(&r->preferences);
    if (!gl_load())
        return false;
    GLuint vs = sc_render_shader(GL_VERTEX_SHADER, vertex_source),
           fs = sc_render_shader(GL_FRAGMENT_SHADER, fragment_source);
    if (!vs || !fs) {
        if (vs)
            gl.DeleteShader(vs);
        if (fs)
            gl.DeleteShader(fs);
        return false;
    }
    r->program = gl.CreateProgram();
    gl.AttachShader(r->program, vs);
    gl.AttachShader(r->program, fs);
    gl.LinkProgram(r->program);
    gl.DeleteShader(vs);
    gl.DeleteShader(fs);
    GLint ok;
    gl.GetProgramiv(r->program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        gl.GetProgramInfoLog(r->program, sizeof(log), NULL, log);
        SDL_Log("Program: %s", log);
        renderer_destroy(r);
        return false;
    }
    r->mvp = gl.GetUniformLocation(r->program, "uMVP");
    r->eye = gl.GetUniformLocation(r->program, "uEye");
    r->grid = gl.GetUniformLocation(r->program, "uGrid");
    r->ui = gl.GetUniformLocation(r->program, "uUI");
    r->offset = gl.GetUniformLocation(r->program, "uOffset");
    r->torch = gl.GetUniformLocation(r->program, "uTorch");
#ifdef SC_WEBXR
    r->torch_origin = gl.GetUniformLocation(r->program, "uTorchOrigin");
#endif
    r->time = gl.GetUniformLocation(r->program, "uTime");
    r->fog_density = gl.GetUniformLocation(r->program, "uFogDensity");
    r->fog_color = gl.GetUniformLocation(r->program, "uFogColor");
    r->lamp_count = gl.GetUniformLocation(r->program, "uLampCount");
    r->lamp_pos = gl.GetUniformLocation(r->program, "uLampPos[0]");
    r->lamp_color = gl.GetUniformLocation(r->program, "uLampColor[0]");
    gl.GenVertexArrays(1, &r->vao);
    gl.GenBuffers(1, &r->vbo);
    gl.GenFramebuffers(SC_LOCAL_VIEWS, r->framebuffer);
    gl.GenTextures(SC_LOCAL_VIEWS, r->texture);
    gl.GenRenderbuffers(SC_LOCAL_VIEWS, r->depth);
    gl.BindVertexArray(r->vao);
    sc_render_layout(r->vbo);
    gl.DepthFunc(GL_LEQUAL);
    /* A fixed celestial sphere has no near-field parallax and is shared by both
       cameras. Cache directions once instead of rebuilding trigonometry per frame. */
    for (int i = 0; i < SC_SPACE_STARS; i++) {
        uint32_t h = world_hash(0x51a7, (Cell){i, 0, 0}, 17);
        double y = 1 - 2 * (i + .5) / SC_SPACE_STARS;
        double radial = sqrt(1 - y * y), angle = i * 2.399963229728653;
        float brightness = .20f + (h % 100) * .0055f;
        r->stars[i] = (SpaceStar){v3(radial * cos(angle), y, radial * sin(angle)),
                                  h % 7 ? rgb(brightness * .83f, brightness * .91f, brightness)
                                        : rgb(brightness, brightness * .87f, brightness * .70f),
                                  .28 + ((h >> 8) % 100) * .0045};
    }
    if (!sc_render_atmosphere_init(r)) {
        renderer_destroy(r);
        return false;
    }
    SDL_Log("Renderer: %s / %s", gl.GetString(GL_RENDERER), gl.GetString(GL_VERSION));
    return true;
}
void renderer_destroy(Renderer *r) {
#ifdef SC_WEBXR
    if (r->quest_fade)
        gl.DeleteProgram(r->quest_fade);
#endif
    if (r->sprite_program)
        gl.DeleteProgram(r->sprite_program);
    if (r->sprite_vao)
        gl.DeleteVertexArrays(1, &r->sprite_vao);
    if (r->sprite_vbo)
        gl.DeleteBuffers(1, &r->sprite_vbo);
    if (r->sprite_atlas)
        gl.DeleteTextures(1, &r->sprite_atlas);
    if (r->program)
        gl.DeleteProgram(r->program);
    if (r->vao)
        gl.DeleteVertexArrays(1, &r->vao);
    if (r->vbo)
        gl.DeleteBuffers(1, &r->vbo);
    for (int i = 0; i < SC_LOCAL_VIEWS; i++) {
        if (r->framebuffer[i])
            gl.DeleteFramebuffers(1, &r->framebuffer[i]);
        if (r->texture[i])
            gl.DeleteTextures(1, &r->texture[i]);
        if (r->depth[i])
            gl.DeleteRenderbuffers(1, &r->depth[i]);
    }
    for (int i = 0; i < SC_MAX_CHUNKS; i++)
        if (r->chunk_vbo[i])
            gl.DeleteBuffers(1, &r->chunk_vbo[i]);
    free(r->mesh.vertices);
    memset(r, 0, sizeof(*r));
}
static void resize_target(Renderer *r, int player, int w, int h) {
    if (r->width[player] == w && r->height[player] == h)
        return;
    r->width[player] = w;
    r->height[player] = h;
    r->target_resizes[player]++;
    gl.BindTexture(GL_TEXTURE_2D, r->texture[player]);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl.BindRenderbuffer(GL_RENDERBUFFER, r->depth[player]);
    gl.RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, w, h);
    gl.BindFramebuffer(GL_FRAMEBUFFER, r->framebuffer[player]);
    gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r->texture[player],
                            0);
    gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                               r->depth[player]);
    if (gl.CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SDL_Log("Incomplete render target");
        r->failed = true;
    }
}
void renderer_draw(Renderer *r, Game *g, const ViewBindings *views, int width, int height,
                   bool paused, bool touch, bool debug, double fps, const char *save_status) {
#ifdef SC_WEBXR
    r->quest_active=false;
#endif
    if (width <= 0 || height <= 0)
        return;
    gl.UseProgram(r->program);
    gl.BindVertexArray(r->vao);
    sc_render_upload_world(r, g);
    gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
    gl.Viewport(0, 0, width, height);
    gl.ClearColor(.008f, .014f, .022f, 1);
    gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    bool vertical = width >= height * 1.5;
    int count = (int)views->count;
    if (count > SC_LOCAL_VIEWS)
        count = SC_LOCAL_VIEWS;
    r->sprite_drawn[0] = r->sprite_drawn[1] = 0;
    r->tentacle_drawn[0] = r->tentacle_drawn[1] = 0;
    for (int view = 0; view < count; view++) {
        int p = game_view_slot(g, views, (unsigned)view);
        if (p < 0)
            continue;
        r->active_view = (unsigned)view;
        int vw = count == 1 ? width : (vertical ? width / 2 : width),
            vh = count == 1 ? height : (vertical ? height : height / 2);
        int vx = count == 1 ? 0 : (vertical ? view * vw : 0),
            vy = count == 1 ? 0 : (vertical ? 0 : (1 - view) * vh);
        if (vw < 1 || vh < 1)
            continue;
        int rh = preferences_height(&r->preferences, view, touch), rw = (int)((double)rh * vw / vh);
        r->view_height[view] = rh;
        if (rw < 160)
            rw = 160;
        if (rw > 960)
            rw = 960;
        resize_target(r, view, rw, rh);
        gl.BindFramebuffer(GL_FRAMEBUFFER, r->framebuffer[view]);
        gl.Viewport(0, 0, rw, rh);
        Player *player = &g->players[p];
        bool matrix = g->matrix.divers[p].active;
        bool outside = !matrix && !player->sheltered && position_absolute(player->pos).z < 0;
        gl.ClearColor(outside ? .001f : .012f, outside ? .002f : .024f, outside ? .004f : .039f, 1);
        gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl.Enable(GL_DEPTH_TEST);
        gl.Uniform1i(r->ui, 0);
        r->origin = matrix || player->sheltered
                        ? v3(0, 0, 0)
                        : world_center(world_cell(position_absolute(player->pos)));
        Vec3 eye = matrix ? matrix_eye(g, p) : vsub(player_eye(player), r->origin);
        float m[16];
        sc_render_camera(m, eye, player->yaw, player->pitch, (double)vw / vh,
                         outside ? fmax(6000, fabs(player_eye(player).z) + 6500) : 420);
        gl.UniformMatrix4fv(r->mvp, 1, GL_FALSE, m);
        gl.Uniform3f(r->eye, (float)eye.x, (float)eye.y, (float)eye.z);
        gl.Uniform3f(r->grid, (float)fmod(r->origin.x, 4096), (float)fmod(r->origin.y, 4096),
                     (float)fmod(r->origin.z, 4096));
        AtmosphereDef atmosphere = sc_atmospheres[STILL_AIR];
        if (g->world.generator >= 10 && !outside && !matrix && !player->sheltered) {
            RoomSpec room = world_describe_version(g->world.seed, g->world.generator,
                                                   world_cell(position_absolute(player->pos)));
            atmosphere = sc_atmospheres[room.habitat.atmosphere];
            const FactionRegion *region =
                g->factions.version ? factions_at(&g->factions, room.cell) : NULL;
            if (region && (region->flags & FS_GROWTH))
                atmosphere = sc_atmospheres[SPORE_AIR];
            const Room *live = world_find((World *)&g->world, room.cell);
            if (live && live->faction_edit == EDIT_WEATHER)
                atmosphere = sc_atmospheres[VOID_GLOW];
        }
        gl.Uniform1f(r->fog_density, outside ? 0
                                     : g->world.generator < 10 || matrix || player->sheltered
                                         ? .009f
                                         : atmosphere.density);
        gl.Uniform3f(r->fog_color, atmosphere.fog.r, atmosphere.fog.g, atmosphere.fog.b);
        sc_render_lights(r, g, player);
        if (matrix)
            sc_render_matrix_scene(r, g, p);
        else if (player->sheltered)
            sc_render_fold_interior(r, g, p);
        else {
            sc_render_starfield(r, player);
            sc_render_draw_chunks(r, g);
            sc_render_hull_view(r, g, player_eye(player));
            sc_render_boarding_skiff(r, g, player_eye(player));
            if (!outside)
                sc_render_infestation_tentacles(r, g, p);
            sc_render_actors(r, g, p);
            sc_render_campaign_scene(r, g, p);
            sc_render_district_scene(r, g, p);
            if (!outside)
                sc_render_navigation_scene(r, g, p);
            if (!outside)
                sc_render_encounter_scene(r, g, p);
            if (!outside)
                sc_render_politics_scene(r, g, p);
        }
        sc_render_flush(r);
        r->sprite_drawn[view] = 0;
        if (!outside && !player->sheltered && !matrix && g->world.generator >= 10)
            sc_render_atmosphere_draw(r, g, p, m, eye, atmosphere);
        if (!paused && !player->sheltered && !matrix) {
            gl.Clear(GL_DEPTH_BUFFER_BIT);
            sc_render_weapon(r, player);
            sc_render_flush(r);
        }
        gl.BindFramebuffer(GL_READ_FRAMEBUFFER, r->framebuffer[view]);
        gl.BindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        gl.BlitFramebuffer(0, 0, rw, rh, vx, vy, vx + vw, vy + vh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.Viewport(vx, vy, vw, vh);
        sc_render_hud(r, g, p, vw, vh, paused, touch && count == 1, debug, fps, save_status);
    }
    if (r->mesh.failed)
        r->failed = true;
}
bool renderer_capture(int width, int height, const char *path) {
    size_t n = (size_t)width * (size_t)height * 4;
    unsigned char *pixels = malloc(n);
    if (!pixels)
        return false;
    gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
    gl.ReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    FILE *f = fopen(path, "wb");
    if (!f) {
        free(pixels);
        return false;
    }
    fprintf(f, "P6\n%d %d\n255\n", width, height);
    bool ok = true;
    for (int y = height - 1; y >= 0; y--)
        for (int x = 0; x < width; x++)
            if (fwrite(pixels + ((size_t)y * width + x) * 4, 1, 3, f) != 3)
                ok = false;
    if (fclose(f) != 0)
        ok = false;
    free(pixels);
    return ok;
}
