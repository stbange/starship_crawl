#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv){Game *g=malloc(sizeof(*g));if(!g||argc!=2)return 2;game_init(g,713);if(!game_load(g,argv[1]))return 3;Vec3 p=position_absolute(g->players[0].pos);world_refresh(&g->world,&p,1);while(g->world.pending&&!g->world.failed)world_stream(&g->world,SC_STREAM_BUDGET);Room *r=world_find(&g->world,world_cell(p));printf("%u,%.5f,%.5f,%.5f,%u,%d,%d,%d\n",g->world.seed,p.x,p.y,p.z,campaign_layer(world_cell(p)),world_spawn_supported(&g->world,p,r?fmin(r->height-.1,1.8):1.8),r?r->ready:0,g->world.failed);game_destroy(g);free(g);return 0;}
