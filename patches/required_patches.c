#include "patches.h"
#include "misc_funcs.h"
#include "sound.h"
#include "PR/os_pi.h"
#include "PR/sched.h"
#include "ultra_extensions.h"
#include "cheats.h"

#define ENABLE_WIDESCREEN_ASPECT_CHANGES

enum OverlayIDLoaded {
    UNSUPPORTED_OVERLAY = -1,
    // ovlbank 1
    LOADED_OVL_1_SUBOVL_1 = 1,
    LOADED_OVL_1_SUBOVL_2 = 2,
    LOADED_OVL_1_SUBOVL_3 = 3,
    LOADED_OVL_1_SUBOVL_4 = 4,
    // ovlbank 2
    LOADED_OVL_2_SUBOVL_1 = 5,
    LOADED_OVL_2_SUBOVL_2 = 6,
    LOADED_OVL_2_SUBOVL_3 = 7,
    LOADED_OVL_2_SUBOVL_4 = 8,
    LOADED_OVL_2_SUBOVL_5 = 9,
    LOADED_OVL_2_SUBOVL_6 = 10,
    // ovlbank 3
    LOADED_OVL_3_SUBOVL_1 = 11,
    LOADED_OVL_3_SUBOVL_2 = 12,
    LOADED_OVL_3_SUBOVL_3 = 13,
    LOADED_OVL_3_SUBOVL_4 = 14,
    LOADED_OVL_3_SUBOVL_5 = 15,
    LOADED_OVL_3_SUBOVL_6 = 16,
    LOADED_OVL_3_SUBOVL_7 = 17,
    // ovlbank 4
    LOADED_OVL_4_SUBOVL_1 = 18,
    LOADED_OVL_4_SUBOVL_2 = 19,
    LOADED_OVL_4_SUBOVL_3 = 20,
    LOADED_OVL_4_SUBOVL_4 = 21,
    LOADED_OVL_4_SUBOVL_5 = 22,
    LOADED_OVL_4_SUBOVL_6 = 23,
    LOADED_OVL_4_SUBOVL_7 = 24,
    LOADED_OVL_4_SUBOVL_8 = 25,
    LOADED_OVL_4_SUBOVL_9 = 26,
    LOADED_OVL_4_SUBOVL_10 = 27,
    // ovlbank 5
    LOADED_OVL_5_SUBOVL_1 = 28,
    LOADED_OVL_5_SUBOVL_2 = 29,
    LOADED_OVL_5_SUBOVL_3 = 30,
    LOADED_OVL_5_SUBOVL_4 = 31,
    LOADED_OVL_5_SUBOVL_5 = 32,
    LOADED_OVL_5_SUBOVL_6 = 33,
    LOADED_OVL_5_SUBOVL_7 = 34,
    // ovlbank 6
    LOADED_OVL_6_SUBOVL_1 = 35,
    LOADED_OVL_6_SUBOVL_2 = 36,
    LOADED_OVL_6_SUBOVL_3 = 37,
    LOADED_OVL_6_SUBOVL_4 = 38,
    LOADED_OVL_6_SUBOVL_5 = 39,
    LOADED_OVL_6_SUBOVL_6 = 40,
    LOADED_OVL_6_SUBOVL_7 = 41,
    LOADED_OVL_6_SUBOVL_8 = 42,
    LOADED_OVL_6_SUBOVL_9 = 43,
    LOADED_OVL_6_SUBOVL_10 = 44,
    LOADED_OVL_6_SUBOVL_11 = 45,
    LOADED_OVL_6_SUBOVL_12 = 46,
    LOADED_OVL_6_SUBOVL_13 = 47,
    LOADED_OVL_6_SUBOVL_14 = 48,
    LOADED_OVL_6_SUBOVL_15 = 49,
    // ovlbank 7
    LOADED_OVL_7_SUBOVL_1 = 50,
    LOADED_OVL_7_SUBOVL_2 = 51,
    LOADED_OVL_7_SUBOVL_3 = 52,
    LOADED_OVL_7_SUBOVL_4 = 53,
    LOADED_OVL_7_SUBOVL_5 = 54,
    LOADED_OVL_7_SUBOVL_6 = 55,
    LOADED_OVL_7_SUBOVL_7 = 56,
    LOADED_OVL_7_SUBOVL_8 = 57,
    LOADED_OVL_7_SUBOVL_9 = 58,
    LOADED_OVL_7_SUBOVL_10 = 59,
    // ovlbank 8
    LOADED_OVL_8_SUBOVL_1 = 60,
    LOADED_OVL_8_SUBOVL_2 = 61,
    LOADED_OVL_8_SUBOVL_3 = 62,
    LOADED_OVL_8_SUBOVL_4 = 63,
    LOADED_OVL_8_SUBOVL_5 = 64,
    LOADED_OVL_8_SUBOVL_6 = 65,
    LOADED_OVL_8_SUBOVL_7 = 66,
    LOADED_OVL_8_SUBOVL_8 = 67,
    LOADED_OVL_8_SUBOVL_9 = 68,
    LOADED_OVL_8_SUBOVL_10 = 69,
    LOADED_OVL_8_SUBOVL_11 = 70,
    LOADED_OVL_8_SUBOVL_12 = 71,
    LOADED_OVL_8_SUBOVL_13 = 72,
    // ovlbank 9
    LOADED_OVL_9_SUBOVL_1 = 73,
    LOADED_OVL_9_SUBOVL_2 = 74,
    LOADED_OVL_9_SUBOVL_3 = 75,
    LOADED_OVL_9_SUBOVL_4 = 76,
    LOADED_OVL_9_SUBOVL_5 = 77,
    LOADED_OVL_9_SUBOVL_6 = 78,
    // ovlbank 10
    LOADED_OVL_10_SUBOVL_1 = 79,
    LOADED_OVL_10_SUBOVL_2 = 80,
    // ovlbank 11
    LOADED_OVL_11_SUBOVL_1 = 81,
    LOADED_OVL_11_SUBOVL_2 = 82,
    // ovlbank 12
    LOADED_OVL_12_SUBOVL_1 = 83,
    LOADED_OVL_12_SUBOVL_2 = 84,
    // ovlbank 13
    LOADED_OVL_13_SUBOVL_1 = 85,
    // ovlbank 14
    LOADED_OVL_14_SUBOVL_1 = 86,
    LOADED_OVL_14_SUBOVL_2 = 87,
    LOADED_OVL_14_SUBOVL_3 = 88,
    LOADED_OVL_14_SUBOVL_4 = 89,
    LOADED_OVL_14_SUBOVL_5 = 90,
    LOADED_OVL_14_SUBOVL_6 = 91,
    LOADED_OVL_14_SUBOVL_7 = 92,
    LOADED_OVL_14_SUBOVL_8 = 93,
    LOADED_OVL_14_SUBOVL_9 = 94,
    LOADED_OVL_14_SUBOVL_10 = 95,
    LOADED_MAIN_AREA = 96, // hack
};

static enum OverlayIDLoaded gOverlayID = UNSUPPORTED_OVERLAY;

int dummy = 1;
int dummyBSS;

extern u32 gSecureCallArr[];

RECOMP_PATCH void set_zero_vaddr_tlb(void) {
    //load_from_rom_to_addr(D_80042000, (u32)&zerojump_ROM_END - (u32)&zerojump_ROM_START, (u32)&zerojump_ROM_START);
    // Not used.
    //osMapTLB(0, 0, NULL, (u32) (((u32) (&D_80042000)) - 0x80000000), -1, -1);
    gSecureCallArr[0] = 0x80019f80; // this feels dirty hardcoding it, but whatever.
}

static s16 scale_volume_s16(s16 volume, float scale) {
    return (s16)((f32)volume * scale);
}

enum {
    VOICE_GAIN_SHIFT = 6,
    CHANNEL_GAIN_SHIFT = 14,
    FINAL_GAIN_SHIFT = 15,
};

RECOMP_PATCH void alSeqpSetVol(ALSeqPlayer* seqp, s16 vol) {
    ALEvent evt;

    evt.type = AL_SEQP_VOL_EVT;
    evt.msg.spvol.vol = vol;
    alEvtqPostEvent(&seqp->evtq, &evt, 0);
}

typedef enum {
    AL_SNDP_PLAY_EVT,
    AL_SNDP_STOP_EVT,
    AL_SNDP_PAN_EVT,
    AL_SNDP_VOL_EVT,
    AL_SNDP_PITCH_EVT,
    AL_SNDP_API_EVT,
    AL_SNDP_DECAY_EVT,
    AL_SNDP_END_EVT,
    AL_SNDP_FX_EVT
} ALSndpMsgType;

typedef struct {
    ALVoice     voice;     
    ALSound     *sound;         /* sound referenced here */
    s16         priority;
    f32         pitch;          /* current playback pitch                    */
    s32         state;          /* play state for this sound                 */
    s16         vol;            /* volume - combined with volume from bank   */
    ALPan       pan;            /* pan - 0 = left, 127 = right               */
    u8          fxMix;          /* wet/dry mix - 0 = dry, 127 = wet          */
} ALSoundState;

typedef union {

    ALEvent             msg;

    struct {
        s16             type;
        ALSoundState    *state;
    } common;
    
    struct {
        s16             type;
        ALSoundState    *state;
        s16             vol;
    } vol;
    
    struct {
        s16             type;
        ALSoundState    *state;
        f32             pitch;
    } pitch;
    
    struct {
        s16             type;
        ALSoundState    *state;
        ALPan           pan;
    } pan;
    
    struct {
        s16             type;
        ALSoundState    *state;
        u8              mix;
    } fx;
    
} ALSndpEvent;

RECOMP_PATCH void alSndpSetVol(ALSndPlayer *sndp, s16 vol) {
    ALSndpEvent evt;
    ALSoundState  *sState = sndp->sndState;

    evt.vol.type = AL_SNDP_VOL_EVT;
    evt.vol.state = &sState[sndp->target];
    evt.vol.vol = scale_volume_s16(vol, recomp_get_sfx_volume());
    alEvtqPostEvent(&sndp->evtq, (ALEvent *)&evt, 0);
}

RECOMP_PATCH s16 __vsVol(ALVoiceState* vs, ALSeqPlayer* seqp) {
    const ALChanState* chan_state;
    u32 note_gain;
    u32 channel_gain;
    s16 native_voice_volume;

    chan_state = &seqp->chanState[(u32)vs->channel];

    // Voice-local gain (tremolo, note velocity, envelope).
    note_gain = (u32)vs->tremelo * (u32)vs->velocity;
    note_gain = (note_gain * (u32)vs->envGain) >> VOICE_GAIN_SHIFT;

    // Channel/sample gain and sequence master volume.
    channel_gain = (u32)chan_state->vol * (u32)vs->sound->sampleVolume;
    channel_gain = (channel_gain * (u32)seqp->vol) >> CHANNEL_GAIN_SHIFT;

    // Original libaudio fixed-point normalization.
    native_voice_volume = (s16)((note_gain * channel_gain) >> FINAL_GAIN_SHIFT);
    return scale_volume_s16(native_voice_volume, recomp_get_bgm_volume());
}

s32 func_8022979C(void*, s32, s32);                       /* extern */
s32 func_8022984C(void*, s32);                          /* extern */
s32 func_80229F74(void*, s32);                          /* extern */
void func_80231408(void*, s32);                          /* extern */
s32 func_802444B8(s32, s32);                            /* extern */
extern f32 D_802A3164;
extern s32 D_802AC89C;
extern s32 D_802AC8A0;

extern s32 D_80024818;
extern u8 gSectionSizes[];
extern OSThread gGameThread;

#define ZEROJMP_SECCALL_TABLE_ID 0
#define ZEROJMP_OS_TABLE_ID      1
#define ZEROJMP_DMA_TABLE_ID     2
#define ZEROJMP_OVL_TABLE_ID     3
// ID 4 Unknown
// ID 5 Unknown
#define ZEROJMP_VI_TABLE_ID      6
#define ZEROJMP_AI_TABLE_ID      7
#define ZEROJMP_DP_TABLE_ID      8
#define ZEROJMP_PI_TABLE_ID      9
#define ZEROJMP_CONT_TABLE_ID    10
// ID 11 Unknown (possibly unused/unmapped)
// ID 12 Unknown
// ID 13 Unknown
// ID 14 Unknown
// ID 15 Unknown
// ID 16 Unknown
// ID 17 Unknown
#define ZEROJMP_SYS_TABLE_ID     18
#define ZEROJMP_AL_TABLE_ID      19
// ID 20 Unknown
// ID 21 Unknown
#define ZEROJMP_PFS_TABLE_ID     22
// ID 23 Unknown
// ID 24 Unknown
// ID 25 Unknown
// ID 26 Unknown
// ID 27 Unknown
// ID 28 Unknown
// ID 29 Unknown
// ID 30 Unknown
// ID 31 Unknown (possibly unused/unmapped)
// ID 32 Unknown
// ID 33 Unknown
// ID 34 Unknown
// ID 35 Unknown
// ID 36 Unknown
// ID 37 Unknown
// ID 38 Unknown
// ID 39 Unknown
// ID 40 Unknown
// ID 41 Unknown
// ID 42 Unknown
// ID 43 Unknown
// ID 44 Unknown
// ID 45 Unknown
// ID 46 Unknown
// ID 47 Unknown
// ID 48 Unknown
// ID 49 Unknown

// FUNCTION TABLES
extern unsigned long gZeroJmpFuncs[]; // (ID: 0) ZeroJmp
extern unsigned long gOSFuncs[]; // (ID: 1) OS functions
extern unsigned long gDMAFuncs[]; // (ID: 2) DMA
extern unsigned long gOVLFuncs[]; // (ID: 3) Overlay load/exec?
extern unsigned long D_8029F570[]; // (ID: 4) Unknown
extern unsigned long D_8029F590[]; // (ID: 5) Unknown
extern unsigned long gVIFuncs[]; // (ID: 6) VI
extern unsigned long gAIFuncs[]; // (ID: 7) AI
extern unsigned long gDPFuncs[]; // (ID: 8) DP
extern unsigned long gPIFuncs[]; // (ID: 9) PI
extern unsigned long gContFuncs[]; // (ID: 10) CONT
extern unsigned long D_8029F5D0[]; // (ID: 11) Unknown
extern unsigned long D_802A0D50[]; // (ID: 12) Unknown
extern unsigned long D_802A1450[]; // (ID: 13) Unknown
extern unsigned long D_802A1610[]; // (ID: 14) Unknown
extern unsigned long D_802A0CF4[]; // (ID: 15) Unknown
extern unsigned long D_802A0E58[]; // (ID: 16) Unknown
// ID 17 Unknown (unused/unmapped)
extern unsigned long gSysFuncs[]; // (ID: 18) SYS (sort of treated like a misc)
extern unsigned long gAlFuncs[]; // (ID: 19) AL
extern unsigned long D_802A13D0[]; // (ID: 20) Unknown
extern unsigned long D_802A13E4[]; // (ID: 21) Unknown
extern unsigned long gPfsFuncs[]; // (ID: 22) PFS
extern unsigned long D_802A0050[]; // (ID: 23) Unknown
extern unsigned long D_8029FD40[]; // (ID: 24) Unknown
extern unsigned long D_802A0100[]; // (ID: 25) Unknown
extern unsigned long D_802A1B54[]; // (ID: 26) Unknown
extern unsigned long D_802A015C[]; // (ID: 27) Unknown
extern unsigned long D_802A1534[]; // (ID: 28) Unknown
extern unsigned long D_8029FE40[]; // (ID: 29) Unknown
extern unsigned long D_802A1C00[]; // (ID: 30) Unknown
// ID 31 Unknown (unused/unmapped)
extern unsigned long D_802A1A88[]; // (ID: 32) Unknown
extern unsigned long D_8029F820[]; // (ID: 33) Unknown
extern unsigned long D_8029F7C8[]; // (ID: 34) Unknown
extern unsigned long D_802A15AC[]; // (ID: 35) Unknown
extern unsigned long D_802A15DC[]; // (ID: 36) Unknown
extern unsigned long D_802A1A48[]; // (ID: 37) Unknown
extern unsigned long D_802A1CE4[]; // (ID: 38) Unknown
extern unsigned long D_802A14B8[]; // (ID: 39) Unknown
extern unsigned long D_802A1500[]; // (ID: 40) Unknown
extern unsigned long D_802A1D60[]; // (ID: 41) Unknown
extern unsigned long D_802A1AB4[]; // (ID: 42) Unknown
extern unsigned long D_802A1380[]; // (ID: 43) Unknown
extern unsigned long D_802A1D70[]; // (ID: 44) Unknown
extern unsigned long D_802A1184[]; // (ID: 45) Unknown
extern unsigned long D_802A1D00[]; // (ID: 46) Unknown
extern unsigned long D_802A1DBC[]; // (ID: 47) Unknown
extern unsigned long D_802A2C50[]; // (ID: 48) Unknown
extern unsigned long D_802A2CB0[]; // (ID: 49) Unknown

extern void set_secure_call_arr__0x0000__secure_call(int, void *);
extern void set_secure_call_arr(int, void *);

extern void OvlSetup_LoadSectionSizes(void);

void load_from_rom_to_addr(void* vAddr, s32 size, u32 devAddr);
void OvlSetup_LoadExecAddress(s32 id, void *vAddr, void *arg);

extern void* D_8001C3C0;
extern OSMesgQueue D_8001C3E0;
extern s32 D_8001C3F8;
extern OSIoMesg D_8001C400;
extern OSMesgQueue D_8001C418;
extern s32 D_8001C430;

RECOMP_PATCH void load_from_rom_to_addr(void* vAddr, s32 size, u32 devAddr) {
    //recomp_printf("Load (load_from_rom_to_addr): devAddr (0x%08X) vAddr (0x%08X) size (0x%08X)\n", devAddr, vAddr, size);
   
    switch(devAddr) {
        case 0x00122016: recomp_load_overlays(0x00800010, (void*) 0x80043000, 0x000019E0); gOverlayID = LOADED_OVL_1_SUBOVL_1; break;
        case 0x00122B34: recomp_load_overlays(0x008019F0, (void*) 0x80043000, 0x00001AD0); gOverlayID = LOADED_OVL_1_SUBOVL_2; break;
        case 0x001236E2: recomp_load_overlays(0x008034C0, (void*) 0x80043000, 0x00001A50); gOverlayID = LOADED_OVL_1_SUBOVL_3; break;
        case 0x00124264: recomp_load_overlays(0x00804F10, (void*) 0x80043000, 0x00001A40); gOverlayID = LOADED_OVL_1_SUBOVL_4; break;
        case 0x0014201C: recomp_load_overlays(0x00806970, (void*) 0x80043000, 0x00005CE0); gOverlayID = LOADED_OVL_2_SUBOVL_1; break;
        case 0x00145A94: recomp_load_overlays(0x0080C650, (void*) 0x80043000, 0x00005500); gOverlayID = LOADED_OVL_2_SUBOVL_2; break;
        case 0x00148D98: recomp_load_overlays(0x00811B50, (void*) 0x80043000, 0x00005800); gOverlayID = LOADED_OVL_2_SUBOVL_3; break;
        case 0x0014C38E: recomp_load_overlays(0x00817350, (void*) 0x80043000, 0x000036C0); gOverlayID = LOADED_OVL_2_SUBOVL_4; break;
        case 0x0014E26A: recomp_load_overlays(0x0081AA10, (void*) 0x80043000, 0x000068D0); gOverlayID = LOADED_OVL_2_SUBOVL_5; break;
        case 0x001521AC: recomp_load_overlays(0x008212E0, (void*) 0x80043000, 0x000044F0); gOverlayID = LOADED_OVL_2_SUBOVL_6; break;
        case 0x0016201E: recomp_load_overlays(0x008257F0, (void*) 0x80043000, 0x00002F80); gOverlayID = LOADED_OVL_3_SUBOVL_1; break;
        case 0x00163B62: recomp_load_overlays(0x00828770, (void*) 0x80043000, 0x00007380); gOverlayID = LOADED_OVL_3_SUBOVL_2; break;
        case 0x0016808C: recomp_load_overlays(0x0082FAF0, (void*) 0x80043000, 0x000062E0); gOverlayID = LOADED_OVL_3_SUBOVL_3; break;
        case 0x0016B954: recomp_load_overlays(0x00835DD0, (void*) 0x80043000, 0x00007290); gOverlayID = LOADED_OVL_3_SUBOVL_4; break;
        case 0x0016F620: recomp_load_overlays(0x0083D060, (void*) 0x80043000, 0x000037C0); gOverlayID = LOADED_OVL_3_SUBOVL_5; break;
        case 0x0017157E: recomp_load_overlays(0x00840820, (void*) 0x80043000, 0x00007590); gOverlayID = LOADED_OVL_3_SUBOVL_6; break;
        case 0x00175AE4: recomp_load_overlays(0x00847DB0, (void*) 0x80043000, 0x00003DF0); gOverlayID = LOADED_OVL_3_SUBOVL_7; break;
        case 0x00182028: recomp_load_overlays(0x0084BBC0, (void*) 0x80043000, 0x00007980); gOverlayID = LOADED_OVL_4_SUBOVL_1; break;
        case 0x001866D6: recomp_load_overlays(0x00853540, (void*) 0x80043000, 0x000070D0); gOverlayID = LOADED_OVL_4_SUBOVL_2; break;
        case 0x0018A962: recomp_load_overlays(0x0085A610, (void*) 0x80043000, 0x00002670); gOverlayID = LOADED_OVL_4_SUBOVL_3; break;
        case 0x0018BBB2: recomp_load_overlays(0x0085CC80, (void*) 0x80043000, 0x00002DD0); gOverlayID = LOADED_OVL_4_SUBOVL_4; break;
        case 0x0018D4BC: recomp_load_overlays(0x0085FA50, (void*) 0x80043000, 0x00002A60); gOverlayID = LOADED_OVL_4_SUBOVL_5; break;
        case 0x0018E9AC: recomp_load_overlays(0x008624B0, (void*) 0x80043000, 0x00006370); gOverlayID = LOADED_OVL_4_SUBOVL_6; break;
        case 0x0019247A: recomp_load_overlays(0x00868820, (void*) 0x80043000, 0x00004950); gOverlayID = LOADED_OVL_4_SUBOVL_7; break;
        case 0x00194D3C: recomp_load_overlays(0x0086D170, (void*) 0x80043000, 0x000025A0); gOverlayID = LOADED_OVL_4_SUBOVL_8; break;
        case 0x00196044: recomp_load_overlays(0x0086F710, (void*) 0x80043000, 0x00004AE0); gOverlayID = LOADED_OVL_4_SUBOVL_9; break;
        case 0x00198A78: recomp_load_overlays(0x008741F0, (void*) 0x80043000, 0x00003670); gOverlayID = LOADED_OVL_4_SUBOVL_10; break;
        case 0x001A201E: recomp_load_overlays(0x00877880, (void*) 0x80043000, 0x00004AD0); gOverlayID = LOADED_OVL_5_SUBOVL_1; break;
        case 0x001A48B4: recomp_load_overlays(0x0087C350, (void*) 0x80043000, 0x00004A00); gOverlayID = LOADED_OVL_5_SUBOVL_2; break;
        case 0x001A75E2: recomp_load_overlays(0x00880D50, (void*) 0x80043000, 0x00005140); gOverlayID = LOADED_OVL_5_SUBOVL_3; break;
        case 0x001AA720: recomp_load_overlays(0x00885E90, (void*) 0x80043000, 0x00004020); gOverlayID = LOADED_OVL_5_SUBOVL_4; break;
        case 0x001AC9C6: recomp_load_overlays(0x00889EB0, (void*) 0x80043000, 0x00003130); gOverlayID = LOADED_OVL_5_SUBOVL_5; break;
        case 0x001AE5AA: recomp_load_overlays(0x0088CFE0, (void*) 0x80043000, 0x000045B0); gOverlayID = LOADED_OVL_5_SUBOVL_6; break;
        case 0x001B0CD4: recomp_load_overlays(0x00891590, (void*) 0x80043000, 0x000036E0); gOverlayID = LOADED_OVL_5_SUBOVL_7; break;
        case 0x001C2036: recomp_load_overlays(0x00894CA0, (void*) 0x80043000, 0x00003CE0); gOverlayID = LOADED_OVL_6_SUBOVL_1; break;
        case 0x001C4300: recomp_load_overlays(0x00898980, (void*) 0x80043000, 0x00003760); gOverlayID = LOADED_OVL_6_SUBOVL_2; break;
        case 0x001C61CA: recomp_load_overlays(0x0089C0E0, (void*) 0x80043000, 0x00003C10); gOverlayID = LOADED_OVL_6_SUBOVL_3; break;
        case 0x001C83A8: recomp_load_overlays(0x0089FCF0, (void*) 0x80043000, 0x00003F00); gOverlayID = LOADED_OVL_6_SUBOVL_4; break;
        case 0x001CA71A: recomp_load_overlays(0x008A3BF0, (void*) 0x80043000, 0x00003210); gOverlayID = LOADED_OVL_6_SUBOVL_5; break;
        case 0x001CC23C: recomp_load_overlays(0x008A6E00, (void*) 0x80043000, 0x00002360); gOverlayID = LOADED_OVL_6_SUBOVL_6; break;
        case 0x001CD2FC: recomp_load_overlays(0x008A9160, (void*) 0x80043000, 0x00002360); gOverlayID = LOADED_OVL_6_SUBOVL_7; break;
        case 0x001CE3BA: recomp_load_overlays(0x008AB4C0, (void*) 0x80043000, 0x00002360); gOverlayID = LOADED_OVL_6_SUBOVL_8; break;
        case 0x001CF478: recomp_load_overlays(0x008AD820, (void*) 0x80043000, 0x00002F90); gOverlayID = LOADED_OVL_6_SUBOVL_9; break;
        case 0x001D0D7A: recomp_load_overlays(0x008B07B0, (void*) 0x80043000, 0x00002560); gOverlayID = LOADED_OVL_6_SUBOVL_10; break;
        case 0x001D1F36: recomp_load_overlays(0x008B2D10, (void*) 0x80043000, 0x000034C0); gOverlayID = LOADED_OVL_6_SUBOVL_11; break;
        case 0x001D3AF4: recomp_load_overlays(0x008B61D0, (void*) 0x80043000, 0x000032B0); gOverlayID = LOADED_OVL_6_SUBOVL_12; break;
        case 0x001D5620: recomp_load_overlays(0x008B9480, (void*) 0x80043000, 0x00003D50); gOverlayID = LOADED_OVL_6_SUBOVL_13; break;
        case 0x001D7808: recomp_load_overlays(0x008BD1D0, (void*) 0x80043000, 0x00002360); gOverlayID = LOADED_OVL_6_SUBOVL_14; break;
        case 0x001D88C6: recomp_load_overlays(0x008BF530, (void*) 0x80043000, 0x00003900); gOverlayID = LOADED_OVL_6_SUBOVL_15; break;
        case 0x001E2028: recomp_load_overlays(0x008C2E50, (void*) 0x80043000, 0x00005720); gOverlayID = LOADED_OVL_7_SUBOVL_1; break;
        case 0x001E5596: recomp_load_overlays(0x008C8570, (void*) 0x80043000, 0x00005D00); gOverlayID = LOADED_OVL_7_SUBOVL_2; break;
        case 0x001E8DE4: recomp_load_overlays(0x008CE270, (void*) 0x80043000, 0x00004090); gOverlayID = LOADED_OVL_7_SUBOVL_3; break;
        case 0x001EB36E: recomp_load_overlays(0x008D2300, (void*) 0x80043000, 0x00005A70); gOverlayID = LOADED_OVL_7_SUBOVL_4; break;
        case 0x001EEA8A: recomp_load_overlays(0x008D7D70, (void*) 0x80043000, 0x000034E0); gOverlayID = LOADED_OVL_7_SUBOVL_5; break;
        case 0x001F0882: recomp_load_overlays(0x008DB250, (void*) 0x80043000, 0x00001940); gOverlayID = LOADED_OVL_7_SUBOVL_6; break;
        case 0x001F136E: recomp_load_overlays(0x008DCB90, (void*) 0x80043000, 0x00002E30); gOverlayID = LOADED_OVL_7_SUBOVL_7; break;
        case 0x001F2B8A: recomp_load_overlays(0x008DF9C0, (void*) 0x80043000, 0x00002DA0); gOverlayID = LOADED_OVL_7_SUBOVL_8; break;
        case 0x001F42F8: recomp_load_overlays(0x008E2760, (void*) 0x80043000, 0x00003200); gOverlayID = LOADED_OVL_7_SUBOVL_9; break;
        case 0x001F5DBE: recomp_load_overlays(0x008E5960, (void*) 0x80043000, 0x00003AE0); gOverlayID = LOADED_OVL_7_SUBOVL_10; break;
        case 0x00202030: recomp_load_overlays(0x008E9470, (void*) 0x80043000, 0x000042F0); gOverlayID = LOADED_OVL_8_SUBOVL_1; break;
        case 0x002049BC: recomp_load_overlays(0x008ED760, (void*) 0x80043000, 0x00002D30); gOverlayID = LOADED_OVL_8_SUBOVL_2; break;
        case 0x00206284: recomp_load_overlays(0x008F0490, (void*) 0x80043000, 0x00003FB0); gOverlayID = LOADED_OVL_8_SUBOVL_3; break;
        case 0x00208A5C: recomp_load_overlays(0x008F4440, (void*) 0x80043000, 0x00002D10); gOverlayID = LOADED_OVL_8_SUBOVL_4; break;
        case 0x0020A324: recomp_load_overlays(0x008F7150, (void*) 0x80043000, 0x00003EE0); gOverlayID = LOADED_OVL_8_SUBOVL_5; break;
        case 0x0020CAC8: recomp_load_overlays(0x008FB030, (void*) 0x80043000, 0x00007B60); gOverlayID = LOADED_OVL_8_SUBOVL_6; break;
        case 0x002118BC: recomp_load_overlays(0x00902B90, (void*) 0x80043000, 0x00006B80); gOverlayID = LOADED_OVL_8_SUBOVL_7; break;
        case 0x0021617A: recomp_load_overlays(0x00909710, (void*) 0x80043000, 0x000066C0); gOverlayID = LOADED_OVL_8_SUBOVL_8; break;
        case 0x0021A53E: recomp_load_overlays(0x0090FDD0, (void*) 0x80043000, 0x000069B0); gOverlayID = LOADED_OVL_8_SUBOVL_9; break;
        case 0x0021EA7C: recomp_load_overlays(0x00916780, (void*) 0x80043000, 0x000040A0); gOverlayID = LOADED_OVL_8_SUBOVL_10; break;
        case 0x00221272: recomp_load_overlays(0x0091A820, (void*) 0x80043000, 0x000036A0); gOverlayID = LOADED_OVL_8_SUBOVL_11; break;
        case 0x00223304: recomp_load_overlays(0x0091DEC0, (void*) 0x80043000, 0x00005C30); gOverlayID = LOADED_OVL_8_SUBOVL_12; break;
        case 0x00226FE4: recomp_load_overlays(0x00923AF0, (void*) 0x80043000, 0x00007B50); gOverlayID = LOADED_OVL_8_SUBOVL_13; break;
        case 0x0024201C: recomp_load_overlays(0x0092B660, (void*) 0x80043000, 0x000034C0); gOverlayID = LOADED_OVL_9_SUBOVL_1; break;
        case 0x00243BAC: recomp_load_overlays(0x0092EB20, (void*) 0x80043000, 0x00002170); gOverlayID = LOADED_OVL_9_SUBOVL_2; break;
        case 0x00244B56: recomp_load_overlays(0x00930C90, (void*) 0x80043000, 0x00002EA0); gOverlayID = LOADED_OVL_9_SUBOVL_3; break;
        case 0x00246372: recomp_load_overlays(0x00933B30, (void*) 0x80043000, 0x00002070); gOverlayID = LOADED_OVL_9_SUBOVL_4; break;
        case 0x0024726C: recomp_load_overlays(0x00935BA0, (void*) 0x80043000, 0x00007BF0); gOverlayID = LOADED_OVL_9_SUBOVL_5; break;
        case 0x0024BD9E: recomp_load_overlays(0x0093D790, (void*) 0x80043000, 0x00002F80); gOverlayID = LOADED_OVL_9_SUBOVL_6; break;
        case 0x00262010: recomp_load_overlays(0x00940720, (void*) 0x80043000, 0x00009200); gOverlayID = LOADED_OVL_10_SUBOVL_1; break;
        case 0x00267B30: recomp_load_overlays(0x00949920, (void*) 0x80043000, 0x0000AC90); gOverlayID = LOADED_OVL_10_SUBOVL_2; break;
        case 0x00282010: recomp_load_overlays(0x009545C0, (void*) 0x80043000, 0x0000A4A0); gOverlayID = LOADED_OVL_11_SUBOVL_1; break;
        case 0x00288D88: recomp_load_overlays(0x0095EA60, (void*) 0x80043000, 0x0000D270); gOverlayID = LOADED_OVL_11_SUBOVL_2; break;
        case 0x002A2010: recomp_load_overlays(0x0096BCE0, (void*) 0x80043000, 0x0000AF30); gOverlayID = LOADED_OVL_12_SUBOVL_1; break;
        case 0x002A94A6: recomp_load_overlays(0x00976C10, (void*) 0x80043000, 0x0000E320); gOverlayID = LOADED_OVL_12_SUBOVL_2; break;
        case 0x002C200C: recomp_load_overlays(0x00984F40, (void*) 0x80043000, 0x000022A0); gOverlayID = LOADED_OVL_13_SUBOVL_1; break;
        case 0x002E2028: recomp_load_overlays(0x00987200, (void*) 0x80043000, 0x00002800); gOverlayID = LOADED_OVL_14_SUBOVL_1; break;
        case 0x002E35C0: recomp_load_overlays(0x00989A00, (void*) 0x80043000, 0x00001DC0); gOverlayID = LOADED_OVL_14_SUBOVL_2; break;
        case 0x002E4438: recomp_load_overlays(0x0098B7C0, (void*) 0x80043000, 0x00001E40); gOverlayID = LOADED_OVL_14_SUBOVL_3; break;
        case 0x002E52E2: recomp_load_overlays(0x0098D600, (void*) 0x80043000, 0x00001E30); gOverlayID = LOADED_OVL_14_SUBOVL_4; break;
        case 0x002E6188: recomp_load_overlays(0x0098F430, (void*) 0x80043000, 0x00001E50); gOverlayID = LOADED_OVL_14_SUBOVL_5; break;
        case 0x002E7044: recomp_load_overlays(0x00991280, (void*) 0x80043000, 0x00002840); gOverlayID = LOADED_OVL_14_SUBOVL_6; break;
        case 0x002E85FE: recomp_load_overlays(0x00993AC0, (void*) 0x80043000, 0x00002780); gOverlayID = LOADED_OVL_14_SUBOVL_7; break;
        case 0x002E9B68: recomp_load_overlays(0x00996240, (void*) 0x80043000, 0x00002C60); gOverlayID = LOADED_OVL_14_SUBOVL_8; break;
        case 0x002EB3EA: recomp_load_overlays(0x00998EA0, (void*) 0x80043000, 0x00003390); gOverlayID = LOADED_OVL_14_SUBOVL_9; break;
        case 0x002ED178: recomp_load_overlays(0x0099C230, (void*) 0x80043000, 0x000036F0); gOverlayID = LOADED_OVL_14_SUBOVL_10; break;
        default: {
            if (devAddr == 0x00040000) {
                size = 0xE0000; // broken size. correct it
            }
            recomp_load_overlays(devAddr, (void*) vAddr, size);
            break;
        }
    }

    osPiStartDma_recomp(&D_8001C400, 0, 0, devAddr, vAddr, (u32) size, &D_8001C418);

    osRecvMesg(&D_8001C418, NULL, 1);
    D_8001C3F8 = 0;
}

RECOMP_PATCH void func_8000064C(void* vramAddr, u32 size, u32 devAddr) {
    recomp_load_overlays(devAddr, vramAddr, size); //@recomp patch
    osPiStartDma_recomp(&D_8001C400, 0, 0, devAddr, vramAddr, size, &D_8001C418);
}

RECOMP_PATCH void func_8000059C(s32 devAddr, u32 size, void* vramAddr) {
    //osWritebackDCache(vramAddr, devAddr);
    recomp_load_overlays(devAddr, vramAddr, size); //@recomp patch
    osPiStartDma_recomp(&D_8001C400, 0, 1, (u32) devAddr, vramAddr, size, &D_8001C418);
    osRecvMesg(&D_8001C418, NULL, 1);
    D_8001C3F8 = 0;
}

RECOMP_PATCH void func_80000524(u32 devAddr, u32 size, void* vramAddr) {
    recomp_load_overlays(devAddr, vramAddr, size); //@recomp patch
    osPiStartDma_recomp(&D_8001C400, 0, 1, devAddr, vramAddr, size, &D_8001C418);
}

RECOMP_PATCH void func_800004D0(void* vramAddr, u32 size, u32 devAddr, s32 arg3) {
    recomp_load_overlays(devAddr, vramAddr, size); //@recomp patch
    osPiStartDma_recomp(&D_8001C400, 0, arg3, devAddr, vramAddr, size, &D_8001C418);
}

void func_80225840(void *);                               /* extern */;
void func_80225800(void *arg0);

void OvlSetup_LoadArea(s32 id, void* vAddr);

RECOMP_PATCH void OvlSetup_LoadExecAddress(s32 id, void *vAddr, void *arg) {
    void (*volatile localarg)(void *);
    // its also possible to match without fake code by omitting arg1 passed to func_800007F0. which would be UB
    OvlSetup_LoadArea(id, vAddr);
    (localarg = vAddr)(arg);
    if(!vAddr) {} // fake check to bump regalloc. see above note
}

enum BufferGetMode {
    BUFFER_GET_FRONT,
    BUFFER_GET_BACK,
};

s32 Gfx_GetAvailableBuffer(void);
void Gfx_SetScreenCoords(s32 ulx, s32 uly, s32 width, s32 height);
void Gfx_SetBackdropEnabled(s32 enable);
void Gfx_SetBackdropColor(s32 r, s32 g, s32 b);
void Gfx_SetMotionBlurEnabled(s32 enable);
void Gfx_SetMotionBlurStrength(s32 c);
void Gfx_SetBufferDisabled(s32 id);
s32 Gfx_GetBufferEnabledStatus(s32 id);
s32 Gfx_GetSubDLPtr(s32 id);
Gfx *Gfx_InitGfx(void);
void Gfx_DrawBackdrop(Gfx** gfxP);
void Gfx_EndRender(Gfx* gfx);
void *Gfx_GetFrameBufferPtr(enum BufferGetMode mode);
void Gfx_SetPauseRender(s32 enable);
void Gfx_CreateRenderThread(void);
void Gfx_Render(void *unused);
void Gfx_KillRenderer(void);

typedef struct GfxWork {
    u8 pad[0x10000];
    Gfx dls[4][0xA00];
    Gfx main[0x600];
} GfxWork; // size:0x27000

#define G_CC_CUSTOM 0, 0, 0, TEXEL0, 0, 0, 0, PRIMITIVE

extern Gfx D_200000[];
extern u8 D_1000000[];

extern u8 D_80100000[2][320 * 240 * sizeof(u16)];
extern GfxWork gGfxWork[];

extern u8 D_8019A8D0[];
extern u8 D_801E56C0[];

extern s32 gFrontBufferID;
extern s32 gDrawBackdrop;
extern s32 gPauseRender;
extern s32 gDrawBackdropRedColor;
extern s32 gDrawBackdropGreenColor;
extern s32 gDrawBackdropBlueColor;
extern s32 gMotionBlur;
extern s32 gMotionBlurStrength;
extern OSMesg D_8029F600;
extern OSScTask D_8029F608[2];

extern Gfx D_8029F6D8[];
extern Gfx D_8029F718[];
extern Gfx D_8029F7A0[];

extern u8 D_802A4840[];

extern OSMesgQueue D_802A5330;
extern s32 D_802A5368;

// .bss
extern s32 gUsedBuffers[4];

extern Gfx *gBufferEnds[];
extern u32 gBackBufferID;
extern GfxWork* gGfxWorkPtr;
extern Gfx* gMasterDisplayList; // D_802A5390
extern s32 gScreenUlx;
extern s32 gScreenUly;
extern s32 gScreenLrx;
extern s32 gScreenLry;

extern OSMesgQueue D_802A5348;
extern OSMesg D_802A5360;
extern OSMesg D_802A5364;
extern s32 D_802A5374;
extern s32 D_802A5378;
extern OSMesg D_802A5394;
extern OSMesg D_802A5398;
extern OSMesgQueue D_802A53A0;
extern OSMesg D_802A53B8;
extern s32 D_802A53CC;
extern s32 D_802A5800;

RECOMP_PATCH Gfx *Gfx_InitGfx(void) {
    gBackBufferID = gFrontBufferID; // we are now using the back buffer to start a new frame rendering.
    gGfxWorkPtr = &gGfxWork[gBackBufferID];
    gMasterDisplayList = &gGfxWorkPtr->main;
    D_802A5368 = 0;

    gEXEnable(gMasterDisplayList++);
    gEXSetRefreshRate(gMasterDisplayList++, 60); // this game runs at a native 60.
    gEXSetRectAspect(gMasterDisplayList++, G_EX_ASPECT_AUTO);

    gSPSegment(gMasterDisplayList++, 0x00, 0x00000000);
    gSPSegment(gMasterDisplayList++, 0x01, (void* ) ((u8*)&D_80100000[gBackBufferID] + 0x80000000));
    gSPDisplayList(gMasterDisplayList++, D_8029F718);
    if (gMotionBlur != 0) {
        gDPSetColorDither(gMasterDisplayList++, G_CD_DISABLE);
    } else {
        gDPSetColorDither(gMasterDisplayList++, G_CD_MAGICSQ);
    }
    gSPDisplayList(gMasterDisplayList++, D_8029F6D8);
    gDPSetDepthImage(gMasterDisplayList++, D_200000);
    gDPPipeSync(gMasterDisplayList++);
    gDPSetScissor(gMasterDisplayList++, G_SC_NON_INTERLACE, 0, 0, 320, 240);
    Gfx_DrawBackdrop(&gMasterDisplayList);
    gSPSetGeometryMode(gMasterDisplayList++, G_ZBUFFER | G_CULL_BACK | G_LIGHTING);
    gDPSetRenderMode(gMasterDisplayList++, G_RM_AA_ZB_TEX_EDGE, G_RM_AA_ZB_TEX_EDGE2);
    return gMasterDisplayList;
}

u8* Libc_Memset(u8* arg0, u8* arg1, s32 arg2);
void func_80225CA8();                         /* extern */
void func_80226860();                         /* extern */
void func_80227708(s32, s32, s32, s32);       /* extern */
void func_802277D0();                         /* extern */
void func_8022787C(Gfx **);                  /* extern */
void func_802290CC();                         /* extern */
void func_8022A858();                         /* extern */
s32 func_80232E60();                          /* extern */
s32 func_802341C8();                          /* extern */
s32 func_80237890();                          /* extern */
s32 func_8023A104();                          /* extern */
s32 func_8023A208();                          /* extern */
s32 func_8023A22C();                          /* extern */
s32 func_8023A318();                          /* extern */
s32 func_8023A3E0();                          /* extern */
s32 func_8025E16C();                          /* extern */
s32 func_8025E1D4(void **);                   /* extern */
s32 g_initRandom(u32);                       /* extern */
s32 func_80265C04();                          /* extern */
s32 func_8026C208();                          /* extern */
s32 func_8026C77C();                          /* extern */
s32 func_802817D0();                          /* extern */
s32 func_80292B54();                          /* extern */
s32 func_80294E54();                          /* extern */
s32 func_80297D20();                          /* extern */
void *func_802998EC(s32);                       /* extern */
extern u8 D_80063000[];
extern u8 D_800BEA60[];
extern s32 D_802A1230;
extern s32 D_802A1234;
extern s32 D_802A123C;
extern u8 D_802A5300[];

struct UnkStruct802AC5C0 {
    u16 unk0;
    u16 unk2;
    u16 unk4;
    u16 unk6;
    u16 unk8;
    u16 unkA;
    u16 unkC;
    u16 unkE;
    char pad10[0x4C];
    f32 unk5C;
    f32 unk60;
    f32 unk64;
    f32 unk68;
};

extern struct UnkStruct802AC5C0 *D_802AC5C0;
extern u8 D_802B36D0[];
extern void func_802334CC();
extern void func_80236F54();

extern void ThreadProc_Init(OSThread *thread, s32 baseID, s32 basePri);
extern void HuPrcInit(void);
extern void HuPrcCall(void);
extern s32 HuPrcCreate(void *func, s32 arg1, void* arg2, s32 arg3, u16 pri);
extern void ThreadProc_RunQueuedThreads(void);
extern void func_802381F8(void);

extern int (*D_802A532C)();

extern Lights2 D_8029F7F8;

struct FrameBuffer {
    Vp unk_00;
    u32 enabled;
    u32 unk14;
    Mtx unk_18;
    u32 unk58;
    f32 ulx;
    f32 uly;
    f32 lrx;
    f32 lry;
};

extern struct FrameBuffer gFrameBuffers[];

extern Mtx* D_802A538C;
extern Mtx D_802A53D8;

extern u16 D_802A53D0;

typedef unsigned int uintptr_t;

void *Libc_Memcpy(uintptr_t dest, uintptr_t source, s32 c);

RECOMP_PATCH void func_8022787C(Gfx** mainGfx) {
    Gfx* gfx;
    s32 i;
    struct FrameBuffer *buffer;

    gfx = *mainGfx;

    gSPSetLights2(gfx++, D_8029F7F8);
    gSPPerspNormalize(gfx++, D_802A53D0);

    for (i = 0; i < 4; i++) {
        buffer = &gFrameBuffers[i];
        if ((buffer->enabled) && (buffer->unk58 != 0)) {
            Libc_Memcpy((uintptr_t)gGfxWorkPtr + (D_802A5368 * sizeof(Mtx)), (uintptr_t)&D_802A53D8, sizeof(Mtx));
            gSPMatrix(gfx++, (uintptr_t)gGfxWorkPtr + (D_802A5368 * sizeof(Mtx)), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
            D_802A5368++;
            gSPViewport(gfx++, &buffer->unk_00);
            Libc_Memcpy((uintptr_t)gGfxWorkPtr + (D_802A5368 * sizeof(Mtx)), (uintptr_t)&buffer->unk_18, sizeof(Mtx));
            gSPMatrix(gfx++, (uintptr_t)gGfxWorkPtr + (D_802A5368 * sizeof(Mtx)), G_MTX_NOPUSH | G_MTX_MUL | G_MTX_PROJECTION);
            D_802A5368++;
            gDPSetScissor(gfx++, G_SC_NON_INTERLACE, buffer->ulx, buffer->uly, buffer->lrx, buffer->lry);
            gSPDisplayList(gfx++, Gfx_GetSubDLPtr(buffer->unk14));
        }
    }
    *mainGfx = gfx;
    osWritebackDCache(buffer, 0x1C0);
}

extern void osViSetSpecialFeatures__0x0000__secure_call(s32);

RECOMP_PATCH void Gfx_SetScreenCoords(s32 ulx, s32 uly, s32 width, s32 height) {
    gScreenUlx = ulx;
    gScreenUly = uly;
    gScreenLrx = (ulx + width);
    gScreenLry = (uly + height);
}

// FBInfo
typedef struct FBInfo {
    /* 0x00 */ Vp vp;
    /* 0x10 */ u32 enabled;
    /* 0x14 */ u32 unk14;
    /* 0x18 */ Mtx mtx;
    /* 0x58 */ u32 unk58;
    /* 0x5C */ f32 ulx;
    /* 0x60 */ f32 uly;
    /* 0x64 */ f32 lrx;
    /* 0x68 */ f32 lry;
} FBInfo; // size:0x6C

// The PC port shrinks the 3D viewport (scale 0.9625, offset 6) because RT64 extends the 3D view past it in
// widescreen. libultraship draws exactly the viewport it gets, so the shrink would leave a border of the backdrop
// color around the 3D view.
#define VIEWPORT_SCALE  1
#define VIEWPORT_OFFSET 0

RECOMP_PATCH void func_80227D50(FBInfo* fb, u32 ulx, u32 uly, u32 width, u32 height) {
    fb->vp.vp.vscale[0] = ((width * 2) * VIEWPORT_SCALE) - VIEWPORT_OFFSET;
    fb->vp.vp.vscale[1] = ((height * 2) * VIEWPORT_SCALE) - VIEWPORT_OFFSET;
    fb->vp.vp.vscale[2] = (511 * VIEWPORT_SCALE) - VIEWPORT_OFFSET;
    fb->vp.vp.vscale[3] = (0 * VIEWPORT_SCALE) - VIEWPORT_OFFSET;
    fb->vp.vp.vtrans[0] = (((ulx * 2) + width) * 2);
    fb->vp.vp.vtrans[1] = (((uly * 2) + height) * 2);
    fb->vp.vp.vtrans[2] = 511;
    fb->vp.vp.vtrans[3] = 0;
    fb->ulx = ulx;
    fb->uly = uly;
    fb->lrx = ((ulx + width));
    fb->lry = ((uly + height));

    if (gMasterDisplayList && fb)
        gEXViewport(gMasterDisplayList++, G_EX_ORIGIN_NONE, &fb->vp);
}

extern void (*D_802B36AC)(void *); // __free_hook
void free_hook(void *);

enum GenericOffOnOption {
    ON,
    OFF,
};

extern u32 D_80027060[];

static OSContPad tempPad;

typedef struct LevelClass
{
	int num;						// 0x00
	int type;						// 0x04
	int propertiesFlag;             // 0x08
	int u2;							// 0x0C
	int flag;						// 0x10
	int u4;							// 0x14
	int u5;							// 0x18
	int u6;							// 0x1C
	int u7;							// 0x20
	int u8;							// 0x24
	int objectID;					// 0x28 - Check ObjectVars.h for a list of object IDs
	int u10;						// 0x2C
	int u11;						// 0x30
	float radius;					// 0x34
	float unk_float;				// 0x38
	int u12;						// 0x3C
	void *ObjectPointer;			// 0x40 - Pointer to the Object this LevelClass is spawning
	struct LevelClass *CollisionPointer; 	// 0x44 - Pointer to the collision LevelClass
	int u13;						// 0x48
	float unk_vector[3];			// 0x4C - 0x54
	int u14[4];						// 0x58 - 0x64
	float unk_float1;				// 0x68
	float yVel;       				// 0x6C
	float unk_float3;				// 0x70
	float unk_float4;				// 0x74
	long unk_pointer2;				// 0x78
	long unk_pointer3;				// 0x7C
} LevelClass;

typedef struct Player {
	float spawnLocation[3];
	float directionAngle;
	int flag;
	int heartCount;
	short u1;
	short bombType;
	int bombCount;
	int bombExplosionLevel;
	int controlType;              // 0 = None, 1 = Player, 2 = AI, 3 = Dead (No Input)
	LevelClass *PlayerLevelClass; // Pointer to the LevelClass this Player is controlling
	LevelClass *HeldLevelClass;   // Pointer to the LevelClass of the held object
	LevelClass *OtherClass;
	int u8;
	int u9;
	int u10;
	int u11;
	int animationType;
	int collisionEnabled;
	int u13;
	int u14;
	int u15;
	float directionAngle2[4];
	short u12;
	char timerA_Enabled; // virusType has to be set for this to work
	char virusType;
	float virusFloat1;
	float virusFloat2;
	int timer2;
	int virusTimer;
	int u16[5];
	int timerA;
	short unkType;
	short unkType1;
	int unkType2;
	int u17[5];
	LevelClass *ComputationLevelClass;
	int u20[3];
	float unknownFloat1;
	int u21;
} Player;

extern Player g_Players[8]; //0x800AEDF0
extern u32 D_802AC624; // lives
extern u32 D_802AC628; // credits

RECOMP_EXPORT void Recomp_ProcessCheats() {
    osContGetReadData(&tempPad);

    /* Moonjump (Hold L to Moonjump) */
    switch (recomp_get_moonjump()) {
        default:
        case (enum GenericOffOnOption)OFF:
            break;
        case (enum GenericOffOnOption)ON:
            // is the player holding L?
            if (tempPad.button & 0x2000) {
                if (g_Players[0].PlayerLevelClass) {
                    g_Players[0].PlayerLevelClass->yVel = -32.0f;
                }
            }
            break;
    }

#define MAX_BOMB_COUNT_UPGRADE 0xFF
#define MAX_BOMB_FIRE_UPGRADE  0xFF

    /* Always Max Bomb Count */
    switch (recomp_get_always_max_bomb()) {
        default:
        case (enum GenericOffOnOption)OFF:
            break;
        case (enum GenericOffOnOption)ON:
            g_Players[0].bombCount = MAX_BOMB_COUNT_UPGRADE;
            break;
    }

    /* Always Max Fire Count */
    switch (recomp_get_always_max_fire()) {
        default:
        case (enum GenericOffOnOption)OFF:
            break;
        case (enum GenericOffOnOption)ON:
            g_Players[0].bombExplosionLevel = MAX_BOMB_FIRE_UPGRADE;
            break;
    }

#define HAVE_RED_BOMBS    (1 << 0) // 1
#define HAVE_REMOTE_BOMBS (1 << 1) // 2

    /* Always Have Remote Bombs */
    switch (recomp_get_always_have_remote_bombs()) {
        default:
        case (enum GenericOffOnOption)OFF:
            break;
        case (enum GenericOffOnOption)ON:
            g_Players[0].bombType |= HAVE_REMOTE_BOMBS;
            break;
    }

    /* Always Have Red Bombs */
    switch (recomp_get_always_have_red_bombs()) {
        default:
        case (enum GenericOffOnOption)OFF:
            break;
        case (enum GenericOffOnOption)ON:
            g_Players[0].bombType |= HAVE_RED_BOMBS;
            break;
    }

    /* Infinite Credits */
    switch (recomp_get_infinite_credits()) {
        default:
        case (enum GenericOffOnOption)OFF:
            break;
        case (enum GenericOffOnOption)ON:
            D_802AC628 = 99;
            break;
    }

    /* Infinite Lives */
    switch (recomp_get_infinite_lives()) {
        default:
        case (enum GenericOffOnOption)OFF:
            break;
        case (enum GenericOffOnOption)ON:
            D_802AC624 = 99;
            break;
    }
}

RECOMP_PATCH void func_80225840(void *arg0) {
    void *temp_s0; 
    Gfx *mainGfx;
    s32 id;
    s32 frameTimer = 8;
    s32 temp_v0;

    func_80297D20();
    Libc_Memset(&D_802A5300, 0, D_802B36D0 - D_802A5300);
    Libc_Memset(&D_80063000, 0, D_800BEA60 - D_80063000);
    func_80225CA8();
    set_secure_call_arr(4, &D_8029F570);
    set_secure_call_arr(5, &D_8029F590);
    func_802341C8();
    ThreadProc_Init(arg0, 0xA, 0xA); // <--------- this will call osCreateScheduler
    osViSetSpecialFeatures__0x0000__secure_call(OS_VI_GAMMA_OFF);
    osViSetSpecialFeatures__0x0000__secure_call(OS_VI_GAMMA_DITHER_ON);
    osViSetSpecialFeatures__0x0000__secure_call(OS_VI_DITHER_FILTER_ON);
    osViSetSpecialFeatures__0x0000__secure_call(OS_VI_DIVOT_ON);
    HuPrcInit();
    func_8023A318();
    func_8023A22C();
    func_80294E54();
    D_802A1230 = 0;
    D_802A1234 = 0;
    D_802A123C = 0;
    func_80226860();
    func_8022A858();
    func_80232E60();
    func_802817D0();
    func_80292B54();
    func_802277D0();
    Gfx_CreateRenderThread();
    func_8025E16C();
    id = Gfx_GetAvailableBuffer();
    func_8026C77C();
    g_initRandom(osGetTime());
    temp_s0 = func_802998EC(0x1000); // mistake. This is secure mapped via the earlier set_secure_call_arr(5) call. Hudson called the unsecure function.
    func_80237890();
    HuPrcCreate(&func_80236F54, 0, (void*)temp_s0, 0x1000, 0x401);
    HuPrcCreate(&func_802334CC, 0, 0, 0, 0x402);
    func_8023A208();
    func_8023A3E0();
    func_80265C04();

    D_802B36AC = free_hook;

    while (TRUE) {
        Recomp_ProcessCheats();
        func_8023A104();           // receive message from the cont mesg queue and run osContGetReadData
        mainGfx = Gfx_InitGfx();   // init gfx
        ThreadProc_RunQueuedThreads();           // get thread pri/start some kind of thread.
        func_802381F8();           // yield to that thread.
        HuPrcCall();               // run Hudson processes.
        func_8022787C(&mainGfx);   // process frame buffers (4 in the array).
        func_802290CC();           // something related to 3D model animations. stubbing this makes bomberman invisible and all objects "stop" animating.

        temp_v0 = Gfx_GetSubDLPtr(id);      // get ptr to main DL buffer to push to display list
        gSPDisplayList(mainGfx++, temp_v0); // put it on the list.

        func_8025E1D4(&mainGfx); // soft reset video effect
        func_8026C208();         // does something with audio
        func_8023A208();         // run osContStartReadData
        Gfx_EndRender(mainGfx);  // do wait/queue/mesg thing?

        // if frame timer is already at 0, ignore the bottom part.
        if (frameTimer == 0) {
            continue;
        }

        // decrement it. If the decrement resulted in a 0, run the below code, this will result in the below code running once.
        if (--frameTimer != 0) {
            continue;
        }

        // on the 8th frame, run these once. Otherwise, the loop is the above.
#ifdef ENABLE_WIDESCREEN_ASPECT_CHANGES
        func_80227D50(D_802AC5C0, 0, 0, 320, 240);
        Gfx_SetScreenCoords(0, 0, 320, 240);
#else
        func_80227D50(D_802AC5C0, 8, 6, 304, 228);
        Gfx_SetScreenCoords(8, 6, 304, 228);
#endif
        osViBlack(0);
    }
}

/*
RECOMP_PATCH void func_802268F4(void) {
    D_802A532C();
}
*/

extern void func_80237D4C(s32);
extern void func_80237AE4(s32 *, OSMesgQueue *);
extern void func_80237A90(OSScTask *t);
extern void func_80237A70(OSMesgQueue *, s32 *);
extern void func_8022997C(void);

// @recomp Set up the message queue and event for waiting on DL completion.
OSMesgQueue dl_complete_queue;
OSMesg dl_complete_queue_buf;

RECOMP_EXPORT void startGfxWait(Gfx *gfx_start) {
    osCreateMesgQueue(&dl_complete_queue, &dl_complete_queue_buf, 1);
    osExQueueDisplaylistEvent(&dl_complete_queue, NULL, gfx_start, OS_EX_DISPLAYLIST_EVENT_PARSED);
}

RECOMP_EXPORT void waitForGfxFinish(void) {
    // @recomp Wait for the displaylist to be completed after submitting it.
    osRecvMesg(&dl_complete_queue, NULL, OS_MESG_BLOCK);
}

/**
 * The thread for constructing the gfx task to send to the renderer.
 */
RECOMP_PATCH void Gfx_Render(void *unused) {
    OSScClient sp80;
    OSMesgQueue sp68;
    OSMesg sp64;
    u32 bufferIDbackup;
    OSMesg sp5C;
    OSScTask *scTask;
    OSMesg sp54;

    func_80237D4C(0);
    osCreateMesgQueue(&sp68, &sp64, 1);
    func_80237AE4(&sp80, &sp68);
    gFrontBufferID = 0;

    while(1) {
        if (D_802A5330.validCount > D_802A5330.msgCount) {
            recomp_printf("[Gfx_Render] validCount was > msgCount.\n");
            osRecvMesg(&D_802A5330, &sp54, 1);
        }
        // wait for Gfx_EndRender to message the GFX that its ready.
        osRecvMesg(&D_802A5330, &D_802A5398, 1);

        // the current ID is considered the front. Get ready to use it to build the final task to send.
        bufferIDbackup = gFrontBufferID;

        // switch the ID for everything else.
        gFrontBufferID ^= 1;

        D_802A5800 = 0;

        scTask = &D_8029F608[bufferIDbackup];
        scTask->framebuffer = (u8*)&D_80100000[bufferIDbackup];
        scTask->list.t.ucode_boot = rspbootTextStart;
        scTask->list.t.ucode_boot_size = (u32)rspbootTextEnd - (u32)rspbootTextStart;
        scTask->list.t.ucode = rspbootTextEnd;
        scTask->list.t.ucode_data = D_802A4840;
        scTask->list.t.dram_stack = D_8019A8D0;
        scTask->list.t.output_buff = D_801E56C0;
        scTask->list.t.output_buff_size = (u64 *) (((u32) D_801E56C0)) + 0x10000;
        scTask->list.t.data_ptr = &gGfxWork[bufferIDbackup].main;
        scTask->list.t.data_size = (gBufferEnds[bufferIDbackup] - gGfxWork[bufferIDbackup].main) * sizeof(Gfx);
        scTask->msgQ = &D_802A53A0;
        scTask->msg = &D_802A53B8;
        osWritebackDCache(gGfxWorkPtr, 0x10000);

        func_80237A90(scTask);
        func_80237A70(&D_802A53A0, &sp5C);               
        func_8022997C();
        D_802A5394 = D_802A5398;
    }
}

RECOMP_EXPORT void printGbi(const char *stringAddr) {
    if (gMasterDisplayList) {
        // UNSUPPORTED
        //gEXPrint(gMasterDisplayList++, stringAddr);
    } else {
        recomp_printf("printGbi was called, but gMasterDisplayList is not yet initialized. Function: %s\n", stringAddr);
    }
}

extern void func_80237F44(void*);

/**
 * The frame is done by now. Perform the last effect on the current buffer and send it off for rendering.
 */
RECOMP_PATCH void Gfx_EndRender(Gfx* gfx) {
    s32 i;

    // Certain events in game use a "motion blur"-ing for effects (example: boss intros). If
    // this is on, perform this blurring effect.
    if (gMotionBlur != 0) {
        gSPTexture(gfx++, 0x8000, 0x8000, 0, G_TX_RENDERTILE, G_ON);
        gDPSetRenderMode(gfx++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
        gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, gMotionBlurStrength);
        gDPSetCombineMode(gfx++, G_CC_CUSTOM, G_CC_CUSTOM);
        gDPSetTexturePersp(gfx++, G_TP_NONE);
        gDPSetTextureLUT(gfx++, G_TT_NONE);
        gDPSetAlphaCompare(gfx++, G_AC_NONE);

        for (i = 0; i < 240; i+=2) {
            gDPLoadTextureBlock(gfx++,
                (u32)OS_PHYSICAL_TO_K0((u8*)D_80100000 + ((gBackBufferID ^ 1) * 240 * 320 * sizeof(u16))) + i * 320 * (s32)sizeof(u16),
                G_IM_FMT_RGBA,
                G_IM_SIZ_16b,
                320,
                2,
                0,
                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

            gSPTextureRectangle(gfx++, 0 << 2, i << 2, 320 << 2, (i + 2) << 2, 0, 0, 0, 1 << 10, 1 << 10);
        }
    }
    gDPFullSync(gfx++);
    gSPEndDisplayList(gfx++);
    gBufferEnds[gBackBufferID] = gfx; // set the ptr to the end of the gfx area.

    // if rendering has paused, do not send it to the scheduler handler.
    if (gPauseRender != 0) {
        osSendMesg(&D_802A5330, D_8029F600, 0);
    }

    func_80237F44(&D_802A5348);
    D_8029F600++;
}

struct UnkStruct800AED78_InnerUnk0 {
    void *unk0;
    char pad4[0x8];
    s32 unkC;
    f32 unk10;
    f32 unk14;
    f32 unk18;
    char pad1C[0x1A];
    s16 unk36;
    char unk38[0x4];
    u32 unk3C; // model ptr
}; // size: at least 0x38

struct UnkStruct800AED78 {
    struct UnkStruct800AED78_InnerUnk0 *unk0;  
    void *unk4;
    s32 fileID;
};

extern struct UnkStruct800AED78 D_800AED78[];
extern s32 D_802AC884;

void *func_8026CE28(s32 fileID);

extern void *func_8022A6C4(void *, s32);

// Room_Load
RECOMP_PATCH void func_80243A50(s32 fileID, f32 arg1, f32 arg2, f32 arg3, s32 arg4, s16 arg5) {
    //recomp_printf("[Room_Load] (fileID) 0x%08X (xPos) %.6f (yPos) %.6f (zPos) %.6f (arg4) 0x%08X (arg5) 0x%08X\n", fileID, arg1, arg2, arg3, arg4, arg5);
    D_800AED78[D_802AC884].unk4 = func_8026CE28(fileID);
    //recomp_printf("[Room_Load] after func_8026CE28 call\n");
    D_800AED78[D_802AC884].unk0 = func_8022A6C4(D_800AED78[D_802AC884].unk4, arg4);
    //recomp_printf("[Room_Load] after func_8022A6C4 call\n");
    D_800AED78[D_802AC884].unk0->unk10 = arg1;
    D_800AED78[D_802AC884].unk0->unk14 = arg2;
    D_800AED78[D_802AC884].unk0->unk18 = arg3;
    if (arg4 == 0x60) {
        D_800AED78[D_802AC884].unk0->unkC |= 0x80;
        if (arg5 != 0) {
            D_800AED78[D_802AC884].unk0->unk36 = arg5;
        }
    }
    D_800AED78[D_802AC884].fileID = fileID;
    //recomp_printf("[Room_Load] Model ptr set to: 0x%08X (0x%08X)\n", D_800AED78[D_802AC884].unk4, D_800AED78[D_802AC884].unk0->unk3C);
    D_802AC884++;
}

extern s32 func_8022616C(s32*, s32, s32, s32);                   /* extern */
extern s32 func_80226368(s32);                               /* extern */
extern s32 func_80226604(s32, s32);                          /* extern */
extern s32 func_802267E0(s32);                                 /* extern */
extern s32 func_8026CD24(s32);                        /* extern */
extern s32 func_80292BD0(s32, void *, s32);                     /* extern */

struct UnkStructD_800A7F30 {
    s32 fileID;
    void *unk4;
    char pad8[0x4];
    s32 unkC;
};

extern struct UnkStructD_800A7F30 D_800A7F30[];
extern s32 D_802AFC30;

struct TextureTrack {
    int tracking;
    s32 fileID;
    void *basePtr;
    void *ptr;
};

// tracked skyboxes
struct TextureTrack gTrackedSkyboxes[2] = 
{
    {
        0, // disabled
        0,
        0,
        0,
    },
    {
        0, // disabled
        0,
        0,
        0,
    }
};

struct TextureTrack gSpecialLoadedBG = {
    {
        0, // disabled
        0,
        0,
        0,
    }
};

// interpolation file IDs
#define GG1_DIAMOND_ID 0x2A5 //  677 (Green Gardens 1 diamond [red])
#define GG3_DIAMOND_ID 0x2A6 //  678 (Green Gardens 3 diamond [blue])

// special BGs to tag as they are not skyboxes.
#define WORLD_SELECT_BG_FILE_ID       0x37
#define BATTLE_SELECT_BG_FILE_ID      0x30
#define CUSTOM_BOMBERMAN_MENU_FILE_ID 0x43

RECOMP_EXPORT int isFileTrackable(s32 fileID) {
    switch(fileID) {
        case WORLD_SELECT_BG_FILE_ID:
        case BATTLE_SELECT_BG_FILE_ID:
        case CUSTOM_BOMBERMAN_MENU_FILE_ID:
            return 1;
        default:
            return 0;
    }
}

RECOMP_EXPORT void clearTrackedTexture(void) {
    for(int i = 0; i < 2; i++) {
        gTrackedSkyboxes[i].tracking = 0;
        gTrackedSkyboxes[i].fileID = 0;
        gTrackedSkyboxes[i].basePtr = 0;
        gTrackedSkyboxes[i].ptr = 0;
    }
}

// allocate and load model
RECOMP_PATCH void *func_8026CE28(s32 fileID) {
    s32 id;
    s32 size;
    s32 sp24;
    s32 diamondWasLoaded = 0;
    s32 specialBGwasLoaded = 0;

    // handle diamond.
    if (fileID == GG1_DIAMOND_ID || fileID == GG3_DIAMOND_ID) {
        //recomp_printf("-------------------------------------\n");
        //recomp_printf("fileID %d load; diamond was loaded!\n\n", fileID);
        diamondWasLoaded = 1;
    }

    // used to track the 3 special BGs that require special handling.
    if (isFileTrackable(fileID)) {
        //recomp_printf("------------------------------------------\n");
        //recomp_printf("special BG 0x%08X was loaded.\n\n", fileID);
        gSpecialLoadedBG.tracking = 1;
        gSpecialLoadedBG.fileID = fileID;
        specialBGwasLoaded = 1;
    }

    if (fileID == -1) {
        return (void*)-1;
    }
    id = func_8026CD24(fileID);
    if (fileID != D_800A7F30[id].fileID) {
        recomp_printf("[func_8026CE28] Loading FileID: (0x%08X) %d\n", fileID, fileID);
        D_802AFC30 += 1;
        D_800A7F30[id].fileID = fileID;
        func_802267E0(0x18);
        sp24 = func_80226604(fileID, 1);
        func_8022616C(&size, 4, 1, sp24);
        D_800A7F30[id].unk4 = func_802998EC(size); // malloc
        func_80292BD0(sp24, D_800A7F30[id].unk4, size);
        func_80226368(sp24);
    }
    D_800A7F30[id].unkC++;

    /**
     * Handle @recomp fixes for specific files.
     */
    if (diamondWasLoaded) {
        u32 *assetPtr = (u32*)D_800A7F30[id].unk4;

        //recomp_printf("Diamond was loaded to: 0x%08X\n\n", D_800A7F30[id].unk4);

        // @recomp now that the diamond was loaded, we need to gracefully handle
        // fixing the animation depending on which one was loaded.
        switch(fileID) {
            case GG1_DIAMOND_ID: {
                assetPtr[0xB54/4] = 0x43B40000;   // 90.0f -> 360.0f
                assetPtr[0xB70/4] = 0x0000003C*4; // multiply anim timer by 4
                assetPtr[0xB7C/4] = 0x0000003C*4; // multiply anim timer by 4
                assetPtr[0xB90/4] = 0x0000003C*4; // multiply anim timer by 4
                break;
            }
            case GG3_DIAMOND_ID: {
                assetPtr[0xCEC/4] = 0x43B40000;   // 90.0f -> 360.0f
                assetPtr[0xD08/4] = 0x0000003C*4; // multiply anim timer by 4
                assetPtr[0xD14/4] = 0x0000003C*4; // multiply anim timer by 4
                assetPtr[0xD28/4] = 0x0000003C*4; // multiply anim timer by 4
                break;
            }
        }
    }

    if (specialBGwasLoaded) {
        gSpecialLoadedBG.basePtr = D_800A7F30[id].unk4;
        //recomp_printf("base ptr: 0x%08X\n", gSpecialLoadedBG.basePtr);
        //recomp_printf("------------------------------------------\n");
    }

    // we loaded in a different file into the EXACT same addr. Stop tracking it for safety.
    if (gSpecialLoadedBG.fileID != fileID && gSpecialLoadedBG.basePtr == D_800A7F30[id].unk4) {
        gSpecialLoadedBG.tracking = 0;
    }

    return D_800A7F30[id].unk4;
}

/**
 * Draw the backdrop color if its enabled and set.
 */
RECOMP_PATCH void Gfx_DrawBackdrop(Gfx** gfxP) {
    Gfx *gfx = *gfxP;

    gEXSetRectAspect(gfx++, G_EX_ASPECT_STRETCH);
    gSPDisplayList(gfx++, D_8029F7A0);
    gDPFillRectangle(gfx++, gScreenUlx, gScreenUly, gScreenLrx - 1, gScreenLry - 1);
    gDPPipeSync(gfx++);
    gDPSetColorImage(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, D_1000000);
    if (gDrawBackdrop != 0) {
        gDPSetFillColor(gfx++, (GPACK_RGBA5551(gDrawBackdropRedColor, gDrawBackdropGreenColor, gDrawBackdropBlueColor, 1) << 0x10) | GPACK_RGBA5551(gDrawBackdropRedColor, gDrawBackdropGreenColor, gDrawBackdropBlueColor, 1));
        gDPFillRectangle(gfx++, gScreenUlx, gScreenUly, gScreenLrx - 1, gScreenLry - 1);
        gDPPipeSync(gfx++);
    }
    gDPSetCycleType(gfx++, G_CYC_1CYCLE);
    gEXSetRectAspect(gfx++, G_EX_ASPECT_AUTO);
    *gfxP = gfx;
}

extern void func_8029A3F4(void*); // _free_internal

RECOMP_EXPORT void breakpoint_me(void *ptr) {
    //recomp_printf("[free] Hook called, ptr value was in suspect range! 0x%08X\n", (u32)ptr);
}

RECOMP_EXPORT void free_hook(void *ptr) {
    if ((u32)ptr >= 0x80310000 && (u32)ptr <= 0x80320000) {
        breakpoint_me(ptr);
    }
    func_8029A3F4(ptr);
}

struct UnkHackStruct802A8970 {
    /* 0x00 */ OSMesg mesg;
    u8 unk[0x38];
};

extern OSMesgQueue D_802A8938;
extern OSMesgQueue D_802A8958;
extern OSMesg D_802A8950;
extern struct UnkHackStruct802A8970 D_802A8970;
extern OSSched D_802A89B0;
extern OSScClient D_802AAC40;
extern s32 gThreadFrameCount;

/*
 * Run the current queues for the thread manager.
 */
RECOMP_PATCH void ThreadProc_RunQueues(void *unused) {
    int loop;
    u8 pad[4];
    OSMesg sendMsg;
    OSMesg recvMsg;

    recvMsg = NULL;
    osCreateMesgQueue(&D_802A8938, &D_802A8950, 1);
    osCreateMesgQueue(&D_802A8958, &D_802A8970.mesg, 0x10);
    osScAddClient(&D_802A89B0, &D_802AAC40, &D_802A8938);
    gThreadFrameCount = 0;

    loop = 1; // somehow, this avoids a thread nop loop.
    while (loop) {
        if (D_802A8938.validCount > D_802A8938.msgCount) {
            recomp_printf("[ThreadProc_RunQueues] validCount was > msgCount.\n");
            osRecvMesg(&D_802A8938, &recvMsg, 1);
        }
        osRecvMesg(&D_802A8938, &recvMsg, 1);

        switch (*(s16*)recvMsg) {
            case 4:
                break;
            case 1:
                gThreadFrameCount++; //! @bug: It will never happen in normal gameplay, but this will overflow after about 2 years of running. Does not seem to cause any crashes though.
                if (osRecvMesg(&D_802A8958, &sendMsg, 0) != -1) {
                    do {
                        osSendMesg(sendMsg, 0, 0);
                    } while (osRecvMesg(&D_802A8958, &sendMsg, 0) != -1);
                }
                break;
        }
    }
}

static int subFileIDLoaded = 0;

// @recomp We will check if the ptr is either of the loaded skyboxes being tracked.
int gfxCheckStretch(u32 *ptr, u32 var) {
    if (gTrackedSkyboxes[0].tracking && (gTrackedSkyboxes[0].ptr == (u8*)ptr)) {
        return 1;
    } else if (gTrackedSkyboxes[1].tracking && (gTrackedSkyboxes[1].ptr == (u8*)ptr)) {
        return 1;
    } else if (gSpecialLoadedBG.tracking && gSpecialLoadedBG.ptr == (u8*)ptr) {
        // custom bomberman BG?
        if (gSpecialLoadedBG.fileID == CUSTOM_BOMBERMAN_MENU_FILE_ID) {
            // dont stretch anything but the BG
            if (var == 0x00002400) {
                return 0;
            }
        }
        // is the currently loaded special file the world select graphics?
        if (gSpecialLoadedBG.fileID == WORLD_SELECT_BG_FILE_ID) {
            // are we trying to stretch the world select GFX? dont.
            if (var == 0x00000C00) {
                return 0;
            }
        }
        return 1;
    } else {
        return 0;
    }
}

typedef struct unk1 {
    s32 unk0;
    u16 unk4;
    u16 unk6;
    s32 unk8;
} unk1;

typedef struct unk2 {
    s8 unk0[0x20];
    s32 unk_20;
    void *unk_24;
    f32 unk_28;
    f32 unk_2C;
} unk2;

RECOMP_PATCH s32 func_80264070(Gfx** gfxP, unk1* arg1, s32 arg2, unk2* arg3) {
    Gfx* gfx;
    unk1* temp_v0;
    s32 var_t0;
    u16 temp_t3;
    u16 temp_t4;
    s32 temp_t5;
    void *temp_v0_2;
    s32 temp_t1;
    s32 pad;

    gfx = *gfxP;

    // FAKE
    temp_t4 = 1;
    temp_v0 = (u8*)arg1 + (arg2 + temp_t4) * sizeof(unk1);

    gDPSetTexturePersp(gfx++, G_TP_NONE);
    gDPSetCombineMode(gfx++, G_CC_DECALRGBA, G_CC_DECALRGBA);
    gSPTexture(gfx++, 0x8000, 0x8000, 0, G_TX_RENDERTILE, G_ON);
    temp_t1 = temp_v0->unk0;
    temp_t3 = temp_v0->unk4;
    temp_t4 = temp_v0->unk6;
    temp_t5 = temp_v0->unk8;

    switch (temp_t1) {
        case 18:
            temp_v0++;
            if (temp_v0->unk0 != 0x1A) {
                return 0;
            }
            temp_v0_2 = ((u8*)arg1 + temp_v0->unk8);

            gDPSetTextureLUT(gfx++, G_TT_RGBA16);
            gDPLoadTLUT_pal16(gfx++, 0, temp_v0_2);
            var_t0 = 0;
            break;
        case 21:
            temp_v0++;
            if (temp_v0->unk0 != 0x1A) {
                return 0;
            }
            temp_v0_2 = ((u8*)arg1 + temp_v0->unk8);
            gDPSetTextureLUT(gfx++, G_TT_RGBA16);
            gDPLoadTLUT_pal256(gfx++, temp_v0_2);
            var_t0 = 1;
            break;
        case 22:
            gDPSetTextureLUT(gfx++, G_TT_NONE);
            var_t0 = 2;
            break;
        case 25:
            gDPSetTextureLUT(gfx++, G_TT_NONE);
            var_t0 = 3;
            break;
        default:
            return 0;
    }

    *gfxP = gfx;
    arg3->unk_28 = temp_t3;
    arg3->unk_2C = temp_t4;
    arg3->unk_20 = var_t0;
    arg3->unk_24 = ((u8*)arg1 + temp_t5);

    if (gTrackedSkyboxes[0].tracking && gTrackedSkyboxes[0].basePtr == arg1) {
        gTrackedSkyboxes[0].ptr = arg3->unk_24; // update the ptr to the actual skybox texture ptr.
    } else if (gTrackedSkyboxes[1].tracking && gTrackedSkyboxes[1].basePtr == arg1) {
        gTrackedSkyboxes[1].ptr = arg3->unk_24; // update the ptr to the actual skybox texture ptr.
    }

    // Track the special BGs
    if (gSpecialLoadedBG.tracking && gSpecialLoadedBG.basePtr == arg1) {
        gSpecialLoadedBG.ptr = arg3->unk_24;
    }

    return 1;
}

extern s32 D_802A1B50;

typedef struct {
    int Unk00;     // Offset: 0x00
    int UnkFlags1;     // Offset: 0x04
    int FileID;     // Offset: 0x08
    int UnkFlags2;     // Offset: 0x0C
    int UnkFlags3;     // Offset: 0x10
    int UnkFlags4;     // Offset: 0x14
    float ScrollXSpeed;     // Offset: 0x18
    float ScrollYSpeed;     // Offset: 0x1C
    float ScrollX;     // Offset: 0x20
    float ScrollY;     // Offset: 0x24
    float OffsetX;     // Offset: 0x28
    float OffsetY;     // Offset: 0x2C
    float ResX;     // Offset: 0x30
    float ResY;     // Offset: 0x34
    float ShiftX;     // Offset: 0x38
    float ShiftY;     // Offset: 0x3C
    float ScaleX;     // Offset: 0x40
    float ScaleY;     // Offset: 0x44
    float ImageWidth;     // Offset: 0x48
    float ImageHeight;     // Offset: 0x4C
} SkyBox;

extern SkyBox D_802B0260;

struct UnkStruct802B0300_Inner {
    char pad[0x10];
    u16 unk10;
    u16 unk12;
};

struct UnkStruct802B0300 {
    struct UnkStruct802B0300_Inner *unk0;
};

struct UnkStruct802B0308_Inner {
    char pad[0xC];
    u32 unkC;
    f32 unk10;
    f32 unk14;
    f32 unk18;
    f32 unk1C;
    f32 unk20;
    f32 unk24;
};

struct UnkStruct802B0308 {
    struct UnkStruct802B0308_Inner *unk0;
};

extern struct UnkStruct802B0300 D_802B0300[];
extern struct UnkStruct802B0308 D_802B0308[];

extern void func_80287904();

// init_skybox
RECOMP_PATCH SkyBox* func_80287DC0(s32 fileID) {
    SkyBox* skybox = &D_802B0260;
    s32 i;

    for(i = 0; i < 2; i++) {
        // find a non-used skybox to init in.
        if (skybox->UnkFlags1 == 0) {
            skybox->Unk00 = i;
            skybox->FileID = fileID;
            skybox->UnkFlags1 = (s32) (skybox->UnkFlags1 | 0x80000000);
            skybox->UnkFlags2 = 0x400;
            skybox->UnkFlags3 = 0x40;
            D_802B0300[skybox->Unk00].unk0 = func_8026CE28(fileID);

            // track the skybox using the current skybox ID since 2 are loaded at the same time.
            gTrackedSkyboxes[skybox->Unk00].tracking = 1;
            gTrackedSkyboxes[skybox->Unk00].fileID = fileID;
            gTrackedSkyboxes[skybox->Unk00].basePtr = D_802B0300[skybox->Unk00].unk0;

            D_802B0308[skybox->Unk00].unk0 = func_8022A6C4(D_802B0300[skybox->Unk00].unk0, skybox->UnkFlags3);
            skybox->UnkFlags2 |= D_802B0308[skybox->Unk00].unk0->unkC;
            skybox->ImageWidth = D_802B0300[skybox->Unk00].unk0->unk10;
            skybox->ImageHeight = D_802B0300[skybox->Unk00].unk0->unk12;
            D_802B0308[skybox->Unk00].unk0->unk10 = skybox->OffsetX;
            D_802B0308[skybox->Unk00].unk0->unk14 = skybox->OffsetY;
            D_802B0308[skybox->Unk00].unk0->unk18 = skybox->ResX;
            D_802B0308[skybox->Unk00].unk0->unk1C = skybox->ResY;
            D_802B0308[skybox->Unk00].unk0->unk20 = 0;
            D_802B0308[skybox->Unk00].unk0->unk24 = 0;
            if (D_802A1B50 == -1) {
                D_802A1B50 = HuPrcCreate(&func_80287904, 0, 0, 0, 0x403);
                Gfx_SetBackdropEnabled(0);
            }
            return skybox;
        }
        skybox++;
    }
    return NULL;
}

// When not building with IDO, use the builtin vaarg macros for portability.
#ifndef __sgi
#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_arg __builtin_va_arg
#define va_end __builtin_va_end
#else

typedef char *va_list;
#define _FP 1
#define _INT 0
#define _STRUCT 2

#define _VA_FP_SAVE_AREA 0x10
#define _VA_ALIGN(p, a) (((unsigned int)(((char *)p) + ((a) > 4 ? (a) : 4) - 1)) & -((a) > 4 ? (a) : 4))
#define va_start(vp, parmN) (vp = ((va_list)&parmN + sizeof(parmN)))

#define __va_stack_arg(list, mode)                                 \
  (                                                                \
      ((list) = (char *)_VA_ALIGN(list, __builtin_alignof(mode)) + \
                _VA_ALIGN(sizeof(mode), 4)),                       \
      (((char *)list) - (_VA_ALIGN(sizeof(mode), 4) - sizeof(mode))))

#define __va_double_arg(list, mode)                                                                  \
  (                                                                                                  \
      (((long)list & 0x1) /* 1 byte aligned? */                                                      \
           ? (list = (char *)((long)list + 7), (char *)((long)list - 6 - _VA_FP_SAVE_AREA))          \
           : (((long)list & 0x2) /* 2 byte aligned? */                                               \
                  ? (list = (char *)((long)list + 10), (char *)((long)list - 24 - _VA_FP_SAVE_AREA)) \
                  : __va_stack_arg(list, mode))))

#define va_arg(list, mode) ((mode *)(((__builtin_classof(mode) == _FP &&          \
                                       __builtin_alignof(mode) == sizeof(double)) \
                                          ? __va_double_arg(list, mode)           \
                                          : __va_stack_arg(list, mode))))[-1]
#define va_end(__list)

#endif

// pass huprintf calls to recomp_printf
RECOMP_PATCH void func_800018F8(const char *fmt, ...) {
    recomp_printf("huprintf called\n");
    va_list arg;
    va_start(arg, fmt);
    recomp_printf(fmt, arg);
    va_end(arg);
}

extern s32 D_802A5418;
extern void func_8029B908(void *, f32, f32, f32, f32, f32, f32, f32, f32, f32);
extern void func_8029BB38(void *, void *);

typedef float Matrix [4][4];

RECOMP_EXPORT void Matrix_Scale(Matrix mf, float x, float y, float z) {
    mf[0][0] = x;
    mf[1][1] = y;
    mf[2][2] = z;
    mf[3][3] = 1;
}

static const f32 original_aspect_ratio = (f32)((f32)320/ (f32)240);

// compute floating point matrix from float values for the given FrameBuffer
RECOMP_PATCH void func_80227CD0(FBInfo *arg0, f32 xEye, f32 yEye, f32 zEye, f32 xAt, f32 yAt, f32 zAt, f32 xUp, f32 yUp, f32 zUp) {
    func_8029B908(&arg0->mtx, xEye, yEye, zEye, xAt, yAt, zAt, xUp, yUp, zUp); // guLookAt
    func_8029BB38(&D_802A5418, &arg0->mtx); // guMtxL2F

    // then scale the matrix to account for the border.
    //f32 scale = (1.0f / recomp_get_target_aspect_ratio(original_aspect_ratio) / original_aspect_ratio);;
    //Matrix_Scale(&D_802A5418, 1.0f, scale, 1.0f);
}

extern void func_8029BE20(void *, void *, f32, f32, f32, f32, f32);

RECOMP_PATCH void func_80227C50(f32 fovy, f32 aspect, f32 near, f32 far, f32 scale) {
    func_8029BE20(&D_802A53D8, &D_802A53D0, fovy, aspect, near, far, scale);
}

typedef struct Unk802A0774 {
    s32 unk0;
    s32 unk4;
    s32 unk8;
    s32 unkC;
    s32 unk10;
} Unk802A0774;

extern Unk802A0774 D_802A0774[2];

struct UnkStruct802AC610 {
    char pad[0x1C];
    s32 unk1C;
    char pad20[0x8];
    s32 unk28;
    char pad2C[0x4];
    s32 unk30;
};

extern struct UnkStruct802AC610 D_802AC610;

extern s32 D_802AC770[];

struct UnkStruct802A0758_InnerUnk0 {
    char pad0[0x14];
    u32 unk14;
    char pad18[0x8];
    u32 unk20;
};

struct UnkStruct802A0758 {
    s32 unk0;
    char pad4[0x10];
    u32 unk14[1][3];
    u32 unk20[1][3];
};

extern struct UnkStruct802A0758* D_802A0758;

struct UnkStruct802A075C_InnerC {
    u32 unk0[1][3];
    u8 fillerC[0x8];
    u32 unk14;
    u8 filler18[0x134-0x18];
    u32 unk134;
};

struct UnkStruct802A075C {
    s32 unk0;
    u8 pad[0x8];
    // u32 unk0[3]; // array of texture pointers?
    struct UnkStruct802A075C_InnerC unkC;
};

extern struct UnkStruct802A075C* D_802A075C;
extern s32 D_802A0764[];

extern s32 D_802A076C[];
extern s32 D_802A079C;

// draw HUD
RECOMP_PATCH void func_80242270(s32 arg0, Gfx** gfxP) {
    s32 i;
    s32 j;
    Gfx *gfx;
    s32 digits;
    s32 div;
    s32 var_v1;
    s32 var_s5;
    s32* var_t0;
    s32 var_t3;
    s32 count;
    struct UnkStruct802A075C* temp_t5;
    struct UnkStruct802A075C_InnerC* var_t2;

    count = 0;
    var_v1 = D_802AC610.unk30;
    digits = 10000000;
    // format time HUD values
    for(i = 0; i < 8; i++) {
        D_802AC770[count++] = (var_v1 / digits) * 2;
        div = (var_v1 / digits) * digits;
        if (var_v1 >= div) {
            var_v1 -= div;
        }
        digits /= 10;
    }

    // format gem HUD values
    digits = 10;
    var_v1 = D_802AC610.unk1C;
    for(i = 0; i != 2; i++) {
        D_802AC770[count++] = (var_v1 / digits) * 2;
        div = (var_v1 / digits) * digits;
        if (var_v1 >= div) {
            var_v1 -= div;
        }
        digits /= 10;
    }

    gfx = *gfxP;    

    gDPSetRenderMode(gfx++, G_RM_AA_TEX_EDGE, G_RM_AA_TEX_EDGE2);
    gDPSetCombineMode(gfx++, G_CC_DECALRGBA, G_CC_DECALRGBA);
    gSPTexture(gfx++, 0x8000, 0x8000, 0, G_TX_RENDERTILE, G_ON);
    gDPSetTexturePersp(gfx++, G_TP_NONE);
    gDPSetTextureLUT(gfx++, G_TT_RGBA16);

    // draw clock and gem icons
    for(i = 0; i != 2; i++) {
        struct UnkStruct802A0758* var_t5 = D_802A0758;
        gDPLoadTLUT_pal256(gfx++, var_t5->unk20[D_802A0774[i].unk0][0] + (u32)var_t5);
        gDPLoadTextureBlock_4b(gfx++, var_t5->unk14[D_802A0774[i].unk0][0] + (u32)var_t5, G_IM_FMT_CI, 16, 16, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

        if (i == 0) {
            // clock
            gEXPushScissor(gfx++);
            gEXSetScissor(gfx++, G_SC_NON_INTERLACE, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT, 0, 0, 0, 240);
            gEXSetRectAlign(gfx++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, 0, 0, 0, 0);
            gSPTextureRectangle(gfx++, D_802A0774[i].unk4, D_802A0774[i].unk8, D_802A0774[i].unkC, D_802A0774[i].unk10, 0, 0, 0, 0x0400, 0x0400);
            gEXSetRectAlign(gfx++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, 0, 0, 0);
            gEXPopScissor(gfx++);
        } else {
            // gem
            gEXPushScissor(gfx++);
            gEXSetScissor(gfx++, G_SC_NON_INTERLACE, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT, 0, 0, 0, 240);
            gEXSetRectAlign(gfx++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, -(320) * 4, 0, -(320) * 4, 0);
            gSPTextureRectangle(gfx++, D_802A0774[i].unk4, D_802A0774[i].unk8, D_802A0774[i].unkC, D_802A0774[i].unk10, 0, 0, 0, 0x0400, 0x0400);
            gEXSetRectAlign(gfx++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, 0, 0, 0);
            gEXPopScissor(gfx++);
        }        
    }

    var_s5 = 0;
    temp_t5 = D_802A075C;

    // draw both digit counters
    for(i = 0; i != 2; i++) {
        var_t3 = D_802A0764[i];

        for (j = 0; j < D_802A076C[i]; j++) {
            var_t2 = (struct UnkStruct802A075C_InnerC*)(&temp_t5->unkC);
            if ((i == 0) && (D_802AC610.unk28 < D_802AC610.unk30)) {
                gDPLoadTLUT_pal16(gfx++, 0, var_t2->unk134 + (u32)temp_t5);
            } else {
                gDPLoadTLUT_pal16(gfx++, 0, var_t2->unk14 + (u32)temp_t5);
            }
            gDPLoadTextureTile_4b(gfx++, var_t2->unk0[D_802AC770[var_s5++]][2] + (u32)temp_t5, G_IM_FMT_CI, 10, 18, 0, 0, 9, 17, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

            if (i == 0) {
                // clock
                gEXPushScissor(gfx++);
                gEXSetScissor(gfx++, G_SC_NON_INTERLACE, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT, 0, 0, 0, 240);
                gEXSetRectAlign(gfx++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, 0, 0, 0, 0);
                gSPTextureRectangle(gfx++, var_t3 << 2, 22 << 2, (var_t3 + 10) << 2, (22 + 18) << 2, 0, 0, 0, 0x0400, 0x0400);
                gEXSetRectAlign(gfx++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, 0, 0, 0);
                gEXPopScissor(gfx++);
            } else {
                // gem
                gEXPushScissor(gfx++);
                gEXSetScissor(gfx++, G_SC_NON_INTERLACE, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT, 0, 0, 0, 240);
                gEXSetRectAlign(gfx++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, -(320) * 4, 0, -(320) * 4, 0);
                gSPTextureRectangle(gfx++, var_t3 << 2, 22 << 2, (var_t3 + 10) << 2, (22 + 18) << 2, 0, 0, 0, 0x0400, 0x0400);
                gEXSetRectAlign(gfx++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, 0, 0, 0);
                gEXPopScissor(gfx++);
            }
            
            var_t3 += 10;
        }
    }
    gDPSetTexturePersp(gfx++, G_TP_PERSP);
    gDPSetTextureLUT(gfx++, G_TT_NONE);

    *gfxP = gfx;
}
