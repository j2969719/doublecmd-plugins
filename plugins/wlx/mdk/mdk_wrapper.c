/*
 * mdk_wrapper.c - Thin C wrapper around MDK SDK C API for Pascal FFI
 *
 * Uses dlopen/dlsym to lazily load libmdk at first use, avoiding
 * static initialization conflicts between libc++ (MDK) and
 * libstdc++ (Double Commander/Qt6) when the WLX plugin is loaded.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <stdint.h>

/* ---- Minimal type definitions mirroring mdk/c headers ---- */

typedef enum {
    MDK_State_Stopped = 0,
    MDK_State_Playing = 1,
    MDK_State_Paused  = 2,
} MDK_State;

typedef enum {
    MDK_SurfaceType_Auto = 0,
    MDK_SurfaceType_X11 = 1,
    MDK_SurfaceType_GBM = 2,
    MDK_SurfaceType_Wayland = 3,
} MDK_SurfaceType;

typedef enum {
    MDK_MediaType_Video = 0,
    MDK_MediaType_Audio = 1,
} MDK_MediaType;

/* Forward declare the opaque player struct */
struct mdkPlayer;

/* The mdkPlayerAPI struct - we only declare fields we need */
typedef struct mdkPlayerAPI {
    struct mdkPlayer *object;
    void (*setMute)(struct mdkPlayer*, bool value);
    void (*setVolume)(struct mdkPlayer*, float value);
    void (*setMedia)(struct mdkPlayer*, const char* url);
    void (*setMediaForType)(struct mdkPlayer*, const char* url, int type);
    const char* (*url)(struct mdkPlayer*);
    void (*setPreloadImmediately)(struct mdkPlayer*, bool value);
    void (*setNextMedia)(struct mdkPlayer*, const char* url, int64_t startPosition, int flags);
    void *currentMediaChanged;
    void (*setAudioBackends)(struct mdkPlayer*, const char** names);
    void (*setAudioDecoders)(struct mdkPlayer*, const char** names);
    void (*setVideoDecoders)(struct mdkPlayer*, const char* names[]);
    void *setTimeout_;
    void *prepare;
    void *mediaInfo;
    void (*setState)(struct mdkPlayer*, MDK_State value);
    MDK_State (*state)(struct mdkPlayer*);
    void *onStateChanged;
    bool (*waitFor)(struct mdkPlayer*, MDK_State value, long timeout);
    void *mediaStatus;
    void *onMediaStatusChanged;
    void (*updateNativeSurface)(struct mdkPlayer*, void* surface, int width, int height, MDK_SurfaceType type);
    void (*createSurface)(struct mdkPlayer*, void* nativeHandle, MDK_SurfaceType type);
    void (*resizeSurface)(struct mdkPlayer*, int w, int h);
    void (*showSurface)(struct mdkPlayer*);
    /* ... remaining fields not needed ... */
} mdkPlayerAPI;

/* ---- dlopen-based lazy loading ---- */

static void *g_mdk_lib = NULL;
static bool  g_mdk_init_attempted = false;

/* Function pointer types */
typedef const mdkPlayerAPI* (*fn_mdkPlayerAPI_new)(void);
typedef void (*fn_mdkPlayerAPI_delete)(const mdkPlayerAPI**);
typedef void (*fn_MDK_setGlobalOptionString)(const char*, const char*);
typedef int  (*fn_MDK_version)(void);

/* Loaded function pointers */
static fn_mdkPlayerAPI_new         p_mdkPlayerAPI_new = NULL;
static fn_mdkPlayerAPI_delete      p_mdkPlayerAPI_delete = NULL;
static fn_MDK_setGlobalOptionString p_MDK_setGlobalOptionString = NULL;
static fn_MDK_version              p_MDK_version = NULL;

static bool mdk_ensure_loaded(void)
{
    if (g_mdk_lib) return true;
    if (g_mdk_init_attempted) return false;
    g_mdk_init_attempted = true;

    fprintf(stderr, "[mdk_wrapper] Loading libmdk.so...\n");

    /* RTLD_LAZY: resolve symbols on demand
     * RTLD_LOCAL: don't pollute global symbol namespace (avoids libc++/libstdc++ conflicts) */
    g_mdk_lib = dlopen("libmdk.so.0", RTLD_LAZY | RTLD_LOCAL);
    if (!g_mdk_lib) {
        /* Try without version */
        g_mdk_lib = dlopen("libmdk.so", RTLD_LAZY | RTLD_LOCAL);
    }
    if (!g_mdk_lib) {
        fprintf(stderr, "[mdk_wrapper] Failed to load libmdk: %s\n", dlerror());
        return false;
    }

    p_mdkPlayerAPI_new = (fn_mdkPlayerAPI_new)dlsym(g_mdk_lib, "mdkPlayerAPI_new");
    p_mdkPlayerAPI_delete = (fn_mdkPlayerAPI_delete)dlsym(g_mdk_lib, "mdkPlayerAPI_delete");
    p_MDK_setGlobalOptionString = (fn_MDK_setGlobalOptionString)dlsym(g_mdk_lib, "MDK_setGlobalOptionString");
    p_MDK_version = (fn_MDK_version)dlsym(g_mdk_lib, "MDK_version");

    if (!p_mdkPlayerAPI_new || !p_mdkPlayerAPI_delete) {
        fprintf(stderr, "[mdk_wrapper] Failed to resolve MDK symbols\n");
        dlclose(g_mdk_lib);
        g_mdk_lib = NULL;
        return false;
    }

    fprintf(stderr, "[mdk_wrapper] libmdk loaded successfully\n");
    return true;
}

/* ---- Opaque handle for Pascal ---- */

typedef struct {
    const mdkPlayerAPI *api;
} MdkHandle;

MdkHandle* mdk_wrapper_create(void)
{
    fprintf(stderr, "[mdk_wrapper] create: enter\n");
    if (!mdk_ensure_loaded()) return NULL;

    MdkHandle *h = (MdkHandle*)malloc(sizeof(MdkHandle));
    if (!h) return NULL;

    h->api = p_mdkPlayerAPI_new();
    fprintf(stderr, "[mdk_wrapper] create: api=%p\n", (void*)h->api);
    if (!h->api) {
        free(h);
        return NULL;
    }
    fprintf(stderr, "[mdk_wrapper] create: object=%p\n", (void*)h->api->object);
    return h;
}

void mdk_wrapper_destroy(MdkHandle *h)
{
    fprintf(stderr, "[mdk_wrapper] destroy: enter h=%p\n", (void*)h);
    if (!h) return;
    if (h->api) {
        fprintf(stderr, "[mdk_wrapper] destroy: stopping\n");
        h->api->setState(h->api->object, MDK_State_Stopped);
        fprintf(stderr, "[mdk_wrapper] destroy: deleting\n");
        p_mdkPlayerAPI_delete(&h->api);
        fprintf(stderr, "[mdk_wrapper] destroy: deleted\n");
    }
    free(h);
    fprintf(stderr, "[mdk_wrapper] destroy: done\n");
}

void mdk_wrapper_set_video_decoders(MdkHandle *h, const char *decoders[], int count)
{
    fprintf(stderr, "[mdk_wrapper] set_video_decoders: enter\n");
    if (!h || !h->api) return;
    h->api->setVideoDecoders(h->api->object, decoders);
    fprintf(stderr, "[mdk_wrapper] set_video_decoders: done\n");
}

void mdk_wrapper_set_media(MdkHandle *h, const char *url)
{
    fprintf(stderr, "[mdk_wrapper] set_media: url=%s\n", url ? url : "(null)");
    if (!h || !h->api) return;
    h->api->setMedia(h->api->object, url);
    fprintf(stderr, "[mdk_wrapper] set_media: done\n");
}

void mdk_wrapper_play(MdkHandle *h)
{
    fprintf(stderr, "[mdk_wrapper] play: enter\n");
    if (!h || !h->api) return;
    h->api->setState(h->api->object, MDK_State_Playing);
    fprintf(stderr, "[mdk_wrapper] play: done\n");
}

void mdk_wrapper_pause(MdkHandle *h)
{
    if (!h || !h->api) return;
    h->api->setState(h->api->object, MDK_State_Paused);
}

void mdk_wrapper_stop(MdkHandle *h)
{
    if (!h || !h->api) return;
    h->api->setState(h->api->object, MDK_State_Stopped);
}

void mdk_wrapper_update_native_surface(MdkHandle *h, void *surface, int w, int h2, int surface_type)
{
    fprintf(stderr, "[mdk_wrapper] update_native_surface: surface=%p w=%d h=%d type=%d\n",
            surface, w, h2, surface_type);
    if (!h || !h->api) return;
    h->api->updateNativeSurface(h->api->object, surface, w, h2, (MDK_SurfaceType)surface_type);
    fprintf(stderr, "[mdk_wrapper] update_native_surface: done\n");
}

void mdk_wrapper_set_volume(MdkHandle *h, float vol)
{
    if (!h || !h->api) return;
    h->api->setVolume(h->api->object, vol);
}

void mdk_wrapper_set_mute(MdkHandle *h, int mute)
{
    if (!h || !h->api) return;
    h->api->setMute(h->api->object, mute ? true : false);
}

void mdk_wrapper_set_playback_rate(MdkHandle *h, float rate)
{
    /* Not yet implemented - requires extending local struct definition */
    (void)h; (void)rate;
}

int mdk_wrapper_get_state(MdkHandle *h)
{
    if (!h || !h->api) return 0;
    return (int)h->api->state(h->api->object);
}

void mdk_wrapper_set_global_option(const char *key, const char *value)
{
    fprintf(stderr, "[mdk_wrapper] set_global_option: key=%s value=%s\n", key, value);
    if (!mdk_ensure_loaded()) return;
    if (p_MDK_setGlobalOptionString)
        p_MDK_setGlobalOptionString(key, value);
    fprintf(stderr, "[mdk_wrapper] set_global_option: done\n");
}

int mdk_wrapper_version(void)
{
    if (!mdk_ensure_loaded()) return 0;
    return p_MDK_version ? p_MDK_version() : 0;
}
