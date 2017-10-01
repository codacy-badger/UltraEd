#include <nusys.h>
#include <math.h>
#include "sos.h"
#include "segments.h"

#define GFX_GLIST_LEN 2048

char mem_heep[1024*512];
Gfx *glistp;
Gfx gfx_glist[GFX_GLIST_LEN];
struct transform world;
u16 perspNormal;
struct sos_model *models[1];

static Vp viewPort = {
  SCREEN_WD * 2, SCREEN_HT * 2, G_MAXZ / 2, 0,
    SCREEN_WD * 2, SCREEN_HT * 2, G_MAXZ / 2, 0,
};

Gfx rspState[] = {
  gsSPViewport(&viewPort),
    gsSPClearGeometryMode(0xFFFFFFFF),
    gsSPSetGeometryMode(G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH | G_CULL_BACK),
    gsSPTexture(0, 0, 0, 0, G_OFF),
    gsSPEndDisplayList()
};

Gfx rdpState[] = {
  gsDPSetRenderMode(G_RM_OPA_SURF, G_RM_OPA_SURF2),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsDPSetScissor(G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT),
    gsDPSetColorDither(G_CD_BAYER),
    gsSPEndDisplayList()
};

void rcpInit() {
  gSPSegment(glistp++, 0, 0x0);
  gSPDisplayList(glistp++, OS_K0_TO_PHYSICAL(rspState));
  gSPDisplayList(glistp++, OS_K0_TO_PHYSICAL(rdpState));
}

void clearFramBuffer() {
  gDPSetDepthImage(glistp++, OS_K0_TO_PHYSICAL(nuGfxZBuffer));
  gDPSetCycleType(glistp++, G_CYC_FILL);
  gDPSetColorImage(glistp++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WD,
    OS_K0_TO_PHYSICAL(nuGfxZBuffer));
  gDPSetFillColor(glistp++, (GPACK_ZDZ(G_MAXFBZ, 0) << 16 |
    GPACK_ZDZ(G_MAXFBZ, 0)));
  gDPFillRectangle(glistp++, 0, 0, SCREEN_WD - 1, SCREEN_HT - 1);
  gDPPipeSync(glistp++);
  
  gDPSetColorImage(glistp++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WD,
    osVirtualToPhysical(nuGfxCfb_ptr));
  gDPSetFillColor(glistp++, (GPACK_RGBA5551(255, 163, 204, 1) << 16 |
    GPACK_RGBA5551(255, 163, 204, 1)));
  gDPFillRectangle(glistp++, 0, 0, SCREEN_WD - 1, SCREEN_HT - 1);
  gDPPipeSync(glistp++);
}

void setup_world_matrix(Gfx **display_list) {
  guPerspective(&world.projection,
    &perspNormal,
    60.0F, SCREEN_WD / SCREEN_HT,
    1.0F, 1000.0F, 1.0F);
  
  guTranslate(&world.translation, 0, 0, 0);
  
  gSPPerspNormalize((*display_list)++, perspNormal);
  
  gSPMatrix((*display_list)++, OS_K0_TO_PHYSICAL(&world.projection),
    G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
  
  gSPMatrix((*display_list)++, OS_K0_TO_PHYSICAL(&world.translation),
    G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
}

void createDisplayList() {
  glistp = gfx_glist;
  rcpInit();
  clearFramBuffer();
  
  setup_world_matrix(&glistp);
  
  sos_draw(models[0], &glistp);
  
  gDPFullSync(glistp++);
  gSPEndDisplayList(glistp++);
  
  nuGfxTaskStart(gfx_glist, (s32)(glistp - gfx_glist) * sizeof(Gfx),
    NU_GFX_UCODE_F3DEX, NU_SC_SWAPBUFFER);
}

void gfxCallback(int pendingGfx) {
  if(pendingGfx < 1) createDisplayList();
}

int initHeapMemory() {
  if(InitHeap(mem_heep, sizeof(mem_heep)) == -1) {
    return -1;
  }
  
  return 0;
}

void load_models() {
  models[0] = (struct sos_model*)load_sos_model(
    _UER_A_MSegmentRomStart, _UER_A_MSegmentRomEnd,
    _UER_A_TSegmentRomStart, _UER_A_TSegmentRomEnd
    );
  
  models[0]->position->x = 0;
  models[0]->position->y = -1;
  models[0]->position->z = -5;
  models[0]->scale->x = 0.001;
  models[0]->scale->y = 0.001;
  models[0]->scale->z = 0.001;
  models[0]->rotation->x = 0;
  models[0]->rotation->y = 0;
  models[0]->rotation->z = 0;
}

void mainproc() {
  nuGfxInit();
  
  if(initHeapMemory() > -1) {
    load_models();
  }
  
  while(1) {
    nuGfxFuncSet((NUGfxFunc)gfxCallback);
    nuGfxDisplayOn();
  }
}
