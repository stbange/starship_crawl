#include "save_internal.h"
void sc_save_narrative(Stream *s, Story *story, bool read, unsigned version) {
    story->version = sc_save_number(s, story->version, read);
    story->assigned = sc_save_number(s, story->assigned, read);
    story->seen = sc_save_number(s, story->seen, read);
    story->done = sc_save_number(s, story->done, read);
    story->tracked = sc_save_number(s, story->tracked, read);
    story->choice = sc_save_number(s, story->choice, read);
    for (int i = 0; i < (version < 8 ? 16 : version < 9 ? 21 : SC_STORY_SCENES); i++) {
        story->sites[i].x = (int32_t)sc_save_number(s, (uint32_t)story->sites[i].x, read);
        story->sites[i].y = (int32_t)sc_save_number(s, (uint32_t)story->sites[i].y, read);
        story->sites[i].z = (int32_t)sc_save_number(s, (uint32_t)story->sites[i].z, read);
    }
}
void sc_save_aftermath(Stream *s, Story *story, Player *players, RoomRecord *records, size_t count,
                       bool read, unsigned version) {
    for (int i = 0; i < 2; i++)
        story->support[i] = sc_save_number(s, story->support[i], read);
    story->council = sc_save_number(s, story->council, read);
    story->chapter = sc_save_number(s, story->chapter, read);
    story->arc = sc_save_number(s, story->arc, read);
    story->chapter_choice = sc_save_number(s, story->chapter_choice, read);
    story->remaining = sc_save_number(s, story->remaining, read);
    for (int i = 0; i < SC_OUTCOMES; i++)
        story->last[i] = sc_save_number(s, story->last[i], read);
    for (int i = 0; i < SC_OUTCOMES; i++)
        story->totals[i] = sc_save_number(s, story->totals[i], read);
    story->event_count = sc_save_number(s, story->event_count, read);
    for (int i = 0; i < SC_STORY_EVENTS; i++) {
        StoryEvent *e = &story->events[i];
        e->chapter = sc_save_number(s, e->chapter, read);
        e->scene = sc_save_number(s, e->scene, read);
        e->choice = sc_save_number(s, e->choice, read);
        e->arc = sc_save_number(s, e->arc, read);
        e->site.x = (int32_t)sc_save_number(s, (uint32_t)e->site.x, read);
        e->site.y = (int32_t)sc_save_number(s, (uint32_t)e->site.y, read);
        e->site.z = (int32_t)sc_save_number(s, (uint32_t)e->site.z, read);
        for (int k = 0; k < 4; k++)
            e->outcome[k] = sc_save_number(s, e->outcome[k], read);
    }
    for (int p = 0; p < SC_LEGACY_PLAYERS; p++)
        for (int k = 0; k < SC_OUTCOMES; k++) {
            players[p].career[k] = sc_save_number(s, players[p].career[k], read);
            if (read && players[p].career[k] > 1000000000)
                s->ok = false;
        }
    for (size_t i = 0; s->ok && i < count; i++) {
        records[i].systems = sc_save_number(s, records[i].systems, read);
        if (read &&
            (records[i].systems >= 16 ||
             ((records[i].systems & SYSTEM_POWER) && (records[i].systems & SYSTEM_BLACKOUT)) ||
             (records[i].systems && story->version < 3 && version < 10)))
            s->ok = false;
    }
}
