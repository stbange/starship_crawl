static bool alien_form(Renderer *r, const Enemy *e, const BodyPose *pose, double time, Color shell,
                       Color flesh, float glow) {
    if (e->behavior < ALIEN_LISTENER)
        return false;
    BodyPose p = *pose;
    unsigned kind = e->behavior;
    Color signal = kind == ALIEN_BRITTLE ? rgb(.35f, .8f, 1) : rgb(.9f, .52f, .22f);
    double warn = e->motion == MOTION_WARN ? 1 - fmin(1, e->motion_time / 1.5) : 0;
    double breath = sin(time * 2 + e->id) * .04;
    if (kind == ALIEN_LISTENER) {
        organ(r, &p, v3(0, .7, 0), v3(1.3 + breath, 1.6, .12), shell, glow);
        for (int i = 0; i < 7; i++) {
            double a = i * SC_PI * 2 / 7, radius = .5 + warn * .6;
            organ(r, &p, v3(cos(a) * radius, .7 + sin(a) * radius, .18), v3(.13, .13, .08), flesh,
                  warn ? .7f : glow);
            bone(r, &p, v3(cos(a) * 1.2, .7 + sin(a) * 1.4, 0),
                 v3(cos(a) * 1.6, .7 + sin(a) * 1.9, -.2), .045, flesh, glow);
        }
        organ(r, &p, v3(0, .7, .2), v3(.15, .4, .05), signal, .5f + warn);
    } else if (kind == ALIEN_REVERSER) {
        for (int i = 0; i < 8; i++) {
            double wave = sin(time * 3 * (e->reversed ? -1 : 1) - i * .9);
            organ(r, &p, v3(wave * .16, wave * .06, -i * .3), v3(.44 + wave * .06, .4, .3),
                  i & 1 ? shell : flesh, glow);
            for (int side = -1; side <= 1; side += 2)
                bone(r, &p, v3(side * .4, -.2, -i * .3), v3(side * .7, -.5, -i * .3 + wave * .1),
                     .04, shell, glow);
        }
        for (int end = 0; end < 2; end++)
            for (int i = 0; i < 8; i++) {
                double a = i * SC_PI / 4;
                organ(r, &p, v3(cos(a) * .3, sin(a) * .3, end ? -2.3 : .2), v3(.06, .06, .15),
                      end == e->reversed ? signal : rgb(.38f, .32f, .24f),
                      end == e->reversed ? .5f + warn : .05f);
            }
    } else if (kind == ALIEN_COLONY) {
        for (int i = 0; i < 3; i++) {
            BodyPose part = p;
            if (e->alive && vlen(e->fragments[i]) > .1)
                part.at = e->fragments[i];
            else
                part.at = vadd(p.at, vmul(p.right, (i - 1) * .22));
            double spread = e->unfold;
            organ(r, &part, v3((i - 1) * .2 * (1 - spread), .1, 0), v3(.19, .8, .22), shell, glow);
            organ(r, &part, v3(0, .95, 0), v3(.14, .17, .15), flesh, glow);
            bone(r, &part, v3(0, -.2, 0), v3((i - 1) * .6, -.65, sin(e->stride * 4 + i) * .2), .07,
                 flesh, glow);
            if (spread > .1 && i != 1)
                tube(r, part.at, e->fragments[1], .025, .025, rgb(.4f, .55f, .23f), .6f);
        }
    } else if (kind == ALIEN_MACHINE) {
        double tilt = e->motion == MOTION_WARN ? .6 * warn : e->travel_speed < .1 ? .48 : -.15;
        p.up = vadd(vmul(pose->up, cos(tilt)), vmul(pose->right, sin(tilt)));
        p.right = vadd(vmul(pose->right, cos(tilt)), vmul(pose->up, -sin(tilt)));
        panel(r, &p, v3(0, .3, 0), v3(.48, .65, .28), shell, glow);
        panel(r, &p, v3(0, 1, .1), v3(.3, .12, .2), signal, .8f);
        for (int side = -1; side <= 1; side += 2) {
            double gait = sin(e->stride * 5) * side;
            bone(r, pose, v3(side * .3, 0, 0), v3(side * .55, -.5, gait * .3), .09, shell, glow);
            panel(r, pose, v3(side * .55, -.6, gait * .3), v3(.2, .1, .4), flesh, glow);
            bone(r, &p, v3(side * .45, .6, 0), v3(side * 1.3, .25, warn), .07, shell, glow);
            organ(r, &p, v3(side * .45, .6, 0), v3(.1, .1, .1), signal,
                  e->motion == MOTION_RECOVER ? 1.5f : .35f);
        }
    } else if (kind == ALIEN_AMBUSH) {
        double unfold = e->motion == MOTION_COMMIT ? 1 : e->motion == MOTION_WARN ? warn : .1;
        organ(r, &p, v3(0, .3, 0), v3(.13 + unfold * .4, .9, .16), shell, glow);
        for (int side = -1; side <= 1; side += 2)
            for (int arm = 0; arm < 3; arm++) {
                Vec3 end = v3(side * (.2 + unfold * 1.9), .2 + arm * .4, unfold * .7);
                bone(r, &p, v3(0, .3 + arm * .3, 0), end, .045, flesh, glow);
                organ(r, &p, end, v3(.04, .04, .04), signal, .6f);
            }
    } else if (kind == ALIEN_REFLECTION) {
        organ(r, &p, v3(0, .5, 0), v3(.2, 1, .25), shell, glow);
        for (int side = -1; side <= 1; side += 2)
            bone(r, &p, v3(0, .7, 0), v3(side * 1.3, .2 + breath, -.4), .08, flesh, glow);
        if (e->alive) {
            Vec3 pool = vadd(e->home, v3(2.4, .045, 0));
            block(r, pool, v3(6, .025, 4), rgb(.08f, .19f, .23f), .12f);
            for (int i = -2; i <= 2; i++)
                block(r, vadd(pool, v3(sin(time * 1.2 + i) * .35, .02, i * .45)),
                      v3(.4 + (2 - abs(i)) * .45, .025, .22), rgb(.22f, .31f, .38f), .25f);
            tube(r, vadd(pool, v3(-1, .04, sin(time))), vadd(pool, v3(1.7, .04, sin(time) + .2)),
                 .015, .015, signal, .5f);
        }
    } else if (kind == ALIEN_PRESSURE) {
        double swell = .65 + warn * .65;
        organ(r, &p, v3(0, .2, 0), v3(swell, swell * .8, swell), shell, glow);
        for (int i = 0; i < 5; i++) {
            double a = i * SC_PI * 2 / 5;
            bone(r, &p, v3(cos(a) * .5, -.1, sin(a) * .5), v3(cos(a) * 1.1, -.65, sin(a) * 1.1),
                 .08, flesh, glow);
        }
        organ(r, &p, v3(0, .2, swell), v3(.22, .22, .1), signal, .4f + warn);
    } else {
        bool molt = kind == ALIEN_MOLTER && e->molted;
        if (molt)
            p.size *= .65;
        double still = kind == ALIEN_WATCHED && e->observed ? 0 : sin(e->stride * (molt ? 7 : 4));
        organ(r, &p, v3(0, .2, 0), v3(molt ? .25 : .45, .65, .3), shell, glow);
        organ(r, &p, v3(0, 1.1, .05), v3(.2, .3, .22), flesh, glow);
        for (int side = -1; side <= 1; side += 2) {
            bone(r, &p, v3(side * .2, -.1, 0), v3(side * .4, -.65, still * side * .2), .1, flesh,
                 glow);
            bone(r, &p, v3(side * .35, .6, 0), v3(side * .9, -.3, -still * side * .2), .075, flesh,
                 glow);
            if (kind == ALIEN_BRITTLE || (kind == ALIEN_MOLTER && !molt))
                panel(r, &p, v3(side * .45, .4, 0), v3(.18, .65, .4), shell, .05f);
            if (kind == ALIEN_BRITTLE)
                bone(r, &p, v3(side * .3, .5, -.1), v3(side * .8, 1.35, -.3), .06, signal,
                     .2f + warn);
        }
        organ(r, &p, v3(0, 1.12, .29), v3(.13, .03, .02), signal, .7f);
        if (molt && e->alive) {
            block(r, vadd(e->home, v3(0, .18, 0)), v3(1.8, .35, 1.3), shell, 0);
            block(r, vadd(e->home, v3(0, .38, 0)), v3(.08, .06, 1.5), signal, .25f);
        }
    }
    return true;
}
