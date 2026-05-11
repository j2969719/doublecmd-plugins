library wlx_mdk_wayland;

{$mode objfpc}{$H+}

uses
  Classes, SysUtils, Interfaces, Forms, Controls, ExtCtrls, LCLType, mdk;

type
  { TPluginPanel - hosts MDK video rendering }
  TPluginPanel = class(TPanel)
  private
    FPlayer: TMdkHandle;
  protected
    procedure Resize; override;
  public
    constructor CreateForMedia(AOwner: TComponent; const AFileName: string);
    destructor Destroy; override;
  end;

constructor TPluginPanel.CreateForMedia(AOwner: TComponent; const AFileName: string);
var
  Decoders: array[0..5] of PChar;
begin
  inherited Create(AOwner);
  Color := 0; { black background }

  FPlayer := mdk_wrapper_create();
  if FPlayer = nil then
    Exit;

  { Set video decoders }
  Decoders[0] := 'VAAPI';
  Decoders[1] := 'VDPAU';
  Decoders[2] := 'CUDA';
  Decoders[3] := 'dav1d';
  Decoders[4] := 'FFmpeg';
  Decoders[5] := nil;
  mdk_wrapper_set_video_decoders(FPlayer, @Decoders[0], 5);

  { Set media first, then attach surface after parenting }
  mdk_wrapper_set_media(FPlayer, PChar(AFileName));
end;

destructor TPluginPanel.Destroy;
begin
  if FPlayer <> nil then
  begin
    mdk_wrapper_destroy(FPlayer);
    FPlayer := nil;
  end;
  inherited Destroy;
end;

procedure TPluginPanel.Resize;
begin
  inherited Resize;
  if (FPlayer <> nil) and HandleAllocated and (Width > 0) and (Height > 0) then
  begin
    { Pass the native window handle to MDK for rendering.
      On Qt6, Handle is a TQtWidget; we pass it and let MDK figure out
      the surface type automatically. }
    mdk_wrapper_update_native_surface(FPlayer,
      {%H-}Pointer(Self.Handle), Width, Height, MDK_SurfaceType_Auto);
  end;
end;

{ ====== WLX Plugin Exports ====== }

function ListLoad(ParentWin: HWND; FileToLoad: PChar; ShowFlags: integer): HWND; stdcall;
var
  Panel: TPluginPanel;
begin
  Result := 0;
  try
    { Set global options - this triggers lazy loading of libmdk via dlopen }
    mdk_wrapper_set_global_option('logLevel', 'Info');

    Panel := TPluginPanel.CreateForMedia(nil, StrPas(FileToLoad));
    Panel.ParentWindow := ParentWin;
    Panel.Align := alClient;
    Panel.Visible := True;

    { Now the handle exists - trigger surface attachment and play }
    if Panel.FPlayer <> nil then
    begin
      mdk_wrapper_update_native_surface(Panel.FPlayer,
        {%H-}Pointer(Panel.Handle), Panel.Width, Panel.Height, MDK_SurfaceType_Auto);
      mdk_wrapper_play(Panel.FPlayer);
    end;

    Result := Panel.Handle;
  except
    on E: Exception do
      { swallow to avoid crashing the host }
  end;
end;

procedure ListCloseWindow(ListWin: HWND); stdcall;
var
  Ctrl: TWinControl;
begin
  Ctrl := FindControl(ListWin);
  if Ctrl <> nil then
    Ctrl.Free;
end;

exports
  ListLoad,
  ListCloseWindow;

begin
  { Nothing here - MDK is lazy-loaded on first use via dlopen }
end.
