unit mdk;

{$mode objfpc}{$H+}

interface

uses
  ctypes;

const
  MDK_WRAPPER_LIB = 'mdk_wrapper';

const
  // MDK_State
  MDK_State_Stopped = 0;
  MDK_State_Playing = 1;
  MDK_State_Paused  = 2;

  // MDK_SurfaceType
  MDK_SurfaceType_Auto    = 0;
  MDK_SurfaceType_X11     = 1;
  MDK_SurfaceType_GBM     = 2;
  MDK_SurfaceType_Wayland = 3;

type
  { Opaque handle to the C wrapper }
  TMdkHandle = Pointer;

{ Player lifecycle }
function mdk_wrapper_create(): TMdkHandle; cdecl; external MDK_WRAPPER_LIB;
procedure mdk_wrapper_destroy(h: TMdkHandle); cdecl; external MDK_WRAPPER_LIB;

{ Playback control }
procedure mdk_wrapper_set_media(h: TMdkHandle; url: PChar); cdecl; external MDK_WRAPPER_LIB;
procedure mdk_wrapper_play(h: TMdkHandle); cdecl; external MDK_WRAPPER_LIB;
procedure mdk_wrapper_pause(h: TMdkHandle); cdecl; external MDK_WRAPPER_LIB;
procedure mdk_wrapper_stop(h: TMdkHandle); cdecl; external MDK_WRAPPER_LIB;
function mdk_wrapper_get_state(h: TMdkHandle): cint; cdecl; external MDK_WRAPPER_LIB;

{ Configuration }
procedure mdk_wrapper_set_video_decoders(h: TMdkHandle; decoders: PPChar; count: cint); cdecl; external MDK_WRAPPER_LIB;
procedure mdk_wrapper_set_volume(h: TMdkHandle; vol: cfloat); cdecl; external MDK_WRAPPER_LIB;
procedure mdk_wrapper_set_mute(h: TMdkHandle; mute: cint); cdecl; external MDK_WRAPPER_LIB;
procedure mdk_wrapper_set_playback_rate(h: TMdkHandle; rate: cfloat); cdecl; external MDK_WRAPPER_LIB;

{ Rendering }
procedure mdk_wrapper_update_native_surface(h: TMdkHandle; surface: Pointer; w, h2, surface_type: cint); cdecl; external MDK_WRAPPER_LIB;

{ Global options - call before creating any player }
procedure mdk_wrapper_set_global_option(key, value: PChar); cdecl; external MDK_WRAPPER_LIB;
function mdk_wrapper_version(): cint; cdecl; external MDK_WRAPPER_LIB;

implementation

end.
