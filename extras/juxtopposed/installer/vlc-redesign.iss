; Copyright (C) 2026 VLC authors and VideoLAN
; SPDX-License-Identifier: GPL-2.0-or-later
;
; Windows installer of VLC Redesign (Inno Setup 6.7 or later), drawn like
; the redesign's own windows: the library's sidebar card with the steps,
; Inter, the dialogs' buttons, the settings' ON / OFF toggles and fields,
; the orange accent. The pieces come from make-art.py (art\ui, art\fonts).
; Built by make-release.ps1:
;   ISCC /DAppVersion=0.2.0 /DSourceDir=<package> /DOutputDir=<dist> vlc-redesign.iss
;
; Updates: a running VLC Redesign from the install folder is closed politely
; (then forced after 15 s) and opened again afterwards. The app's updater
; passes /UPDATE /RESUME=<media> /TIME=<seconds> so playback continues where
; it was; a manual run reopens the media VLC was started with.
;
; File types: VLC Redesign is always offered in "Open with" and in Windows'
; Default apps for the types VLC plays. The Options page can also make it
; the default player of video, music and playlists where no other app was
; chosen (Windows keeps the user's choices: the last page opens Default
; apps for the rest), and add "Play with VLC Redesign" to the right-click
; menus. All of it goes away with the uninstaller.

#ifndef AppVersion
  #error Define AppVersion, e.g. /DAppVersion=0.2.0
#endif
#ifndef SourceDir
  #error Define SourceDir, the portable package folder
#endif
#ifndef OutputDir
  #define OutputDir "."
#endif

#define AppName "VLC Redesign"
#define AppIdGuid "8F3C2A61-5B7E-4D9A-9C1E-6A2F0B4D7E15"
#define RepoUrl "https://github.com/KuchiSofts/vlc-redesign"

[Setup]
AppId={{{#AppIdGuid}}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=KuchiSofts
AppPublisherURL={#RepoUrl}
AppSupportURL={#RepoUrl}/issues
AppUpdatesURL={#RepoUrl}/releases/latest
AppCopyright=GPL v2 or later. Interface design by Juxtopposed (CC BY 4.0). VLC is a trademark of VideoLAN.
VersionInfoVersion={#AppVersion}
VersionInfoDescription={#AppName} Setup
; Per-user (no administrator rights, no conflict with an installed official
; VLC); "Install for everyone" on the Options page starts Setup again with
; /ALLUSERS, for Program Files. An update keeps the mode of the install.
DefaultDirName={autopf}\{#AppName}
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=commandline
UsePreviousPrivileges=yes
UsePreviousAppDir=yes
; The pages: Welcome, Options (ours: the folder and the toggles), the
; installation, Done. The folder and tasks pages give way to Options.
DisableWelcomePage=no
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=yes
ShowLanguageDialog=auto
UninstallDisplayName={#AppName}
UninstallDisplayIcon={app}\vlc.exe
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; Qt 6.8 supports Windows 10 from version 1809 (build 17763)
MinVersion=10.0.17763
OutputDir={#OutputDir}
OutputBaseFilename=vlc-redesign-{#AppVersion}-win64-setup
SetupIconFile=art\setup.ico
Compression=lzma2/ultra64
SolidCompression=yes
; The files keep the times they have in the package, in any time zone: VLC
; trusts the plugin cache of the package (plugins\plugins.dat) only for
; plugins of the same time and size. With local times (the default) the cache
; was stale away from the time zone of the release, and making it again here
; (vlc-cache-gen, which loads every plugin) could hang the setup.
TimeStampsInUTC=yes
ChangesAssociations=yes
; We close VLC ourselves (and reopen it), not through the Restart Manager
CloseApplications=no
RestartApplications=no
; The redesign's look: forced dark, the library's background; the window
; is drawn by the [Code] below over art\ui\back-*.png
WizardStyle=modern dark hidebevels
WizardBackColor=#0E0E0E
WizardImageFile=
WizardSmallImageFile=

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "he"; MessagesFile: "compiler:Languages\Hebrew.isl"

[CustomMessages]
en.StepWelcome=Welcome
he.StepWelcome=ברוכים הבאים
en.StepOptions=Options
he.StepOptions=אפשרויות
en.StepInstall=Install
he.StepInstall=התקנה
en.StepDone=Done
he.StepDone=סיום
en.Unofficial=Unofficial build
he.Unofficial=גרסה לא רשמית
en.DesignedBy=Interface designed by
he.DesignedBy=הממשק עוצב בידי
en.WelcomeTitle=Welcome to {#AppName}
he.WelcomeTitle=ברוכים הבאים ל-{#AppName}
en.WelcomeText=The real VLC 4 engine with the interface designed by Juxtopposed. Setup takes about a minute.
he.WelcomeText=מנוע VLC 4 האמיתי עם הממשק שעיצבה Juxtopposed. ההתקנה נמשכת כדקה.
en.CreditTitle=Interface designed by Juxtopposed
he.CreditTitle=את הממשק עיצבה Juxtopposed
en.CreditText=Every screen comes from the "VLC Redesign (Community)" Figma file, licensed under CC BY 4.0. Juxtopposed does not endorse this project.
he.CreditText=כל מסך מגיע מקובץ ה-Figma "VLC Redesign (Community)", ברישיון CC BY 4.0. Juxtopposed אינה מעורבת בפרויקט הזה ואינה ממליצה עליו.
en.LinkVideo=Watch the video
he.LinkVideo=לצפייה בסרטון
en.LinkChannel=YouTube channel
he.LinkChannel=ערוץ היוטיוב
en.LinkFigma=Figma file
he.LinkFigma=קובץ ה-Figma
en.WelcomeNotice=An unofficial build, not made or endorsed by VideoLAN. VLC and the cone logo are trademarks of VideoLAN. It installs next to an official VLC without touching it.
he.WelcomeNotice=גרסה לא רשמית, שאינה של VideoLAN ואינה מאושרת על ידה. VLC ולוגו הקונוס הם סימנים מסחריים של VideoLAN. היא מותקנת לצד VLC רשמי בלי לגעת בו.
en.LinkNotice=Notice and licenses
he.LinkNotice=הודעה ורישיונות
en.LinkSource=Source code
he.LinkSource=קוד מקור
en.VersionLine=Version %1 · free software under the GNU GPL v2 or later
he.VersionLine=גרסה %1 · תוכנה חופשית ברישיון GNU GPL גרסה 2 ומעלה
en.OptionsTitle=Options
he.OptionsTitle=אפשרויות
en.OptionsText=Where it goes, and what it opens.
he.OptionsText=לאן להתקין, ומה לפתוח איתה.
en.InstallTo=Install to
he.InstallTo=התקנה אל
en.Browse=Browse
he.Browse=עיון
en.ForYou=For you only, without administrator rights.
he.ForYou=עבורך בלבד, בלי הרשאות מנהל.
en.ForEveryone=Install for everyone
he.ForEveryone=התקנה לכל המשתמשים
en.ForAll=For everyone on this PC.
he.ForAll=לכל המשתמשים במחשב.
en.UpdatingHere=Updates the copy in this folder.
he.UpdatingHere=מעדכן את העותק שבתיקייה הזו.
en.OpenVideo=Open videos with {#AppName}
he.OpenVideo=פתיחת סרטונים ב-{#AppName}
en.OpenAudio=Open music with {#AppName}
he.OpenAudio=פתיחת מוזיקה ב-{#AppName}
en.OpenPlaylists=Open playlists with {#AppName}
he.OpenPlaylists=פתיחת רשימות השמעה ב-{#AppName}
en.ContextMenu=Play with {#AppName} in right-click menus
he.ContextMenu="הפעלה ב-{#AppName}" בתפריט הקליק הימני
en.DesktopIcon=Desktop shortcut
he.DesktopIcon=קיצור דרך בשולחן העבודה
en.InstallingTitle=Installing
he.InstallingTitle=מתקין
en.InstallingText=This takes about a minute.
he.InstallingText=זה לוקח כדקה.
en.TipTitle=Make the player yours
he.TipTitle=התאימו את הנגן לעצמכם
en.TipText=Drag buttons into and out of the control bar with ⋯ → Customize Player, and keep your layouts as profiles.
he.TipText=גררו כפתורים אל סרגל השליטה וממנו דרך ⋯ ← Customize Player, ושמרו את הפריסות שלכם כפרופילים.
en.DoneTitle={#AppName} is ready
he.DoneTitle={#AppName} מוכן
en.DoneText=Find it in the Start menu.
he.DoneText=אפשר למצוא אותו בתפריט התחל.
en.DefaultTitle=Make it your default player
he.DefaultTitle=הגדרה כנגן ברירת המחדל
en.DefaultText=Windows keeps the apps you chose before. Open Default apps and choose {#AppName} for your media.
he.DefaultText=Windows שומרת את האפליקציות שבחרת בעבר. פתחו את "אפליקציות ברירת מחדל" ובחרו ב-{#AppName} עבור קובצי המדיה.
en.OpenDefaultApps=Open Default apps
he.OpenDefaultApps=לאפליקציות ברירת מחדל
en.LaunchNow=Open {#AppName} now
he.LaunchNow=פתיחת {#AppName} עכשיו
en.DoneCredit=Enjoy the design by Juxtopposed.
he.DoneCredit=תיהנו מהעיצוב של Juxtopposed.
en.LinkSubscribe=Subscribe on YouTube
he.LinkSubscribe=הרשמה לערוץ ביוטיוב
en.NoticeTitle=Notice and licenses
he.NoticeTitle=הודעה ורישיונות
en.Close=Close
he.Close=סגירה
en.BadDir=Choose a folder to install to.
he.BadDir=בחרו תיקייה להתקנה.
en.TypeVideo=%1 video
he.TypeVideo=וידאו %1
en.TypeAudio=%1 audio
he.TypeAudio=שמע %1
en.TypePlaylist=%1 playlist
he.TypePlaylist=רשימת השמעה %1
en.VerbPlay=Play
he.VerbPlay=הפעלה
en.VerbEnqueue=Add to queue
he.VerbEnqueue=הוספה לתור
en.ContextPlay=Play with {#AppName}
he.ContextPlay=הפעלה ב-{#AppName}
en.ContextEnqueue=Add to {#AppName} queue
he.ContextEnqueue=הוספה לתור של {#AppName}
en.Running={#AppName} is open. Setup will close it now and open it again when it's done.
he.Running={#AppName} פתוח. ההתקנה תסגור אותו עכשיו ותפתח אותו מחדש בסיום.
en.StillRunning={#AppName} didn't close. Close it and click Retry.
he.StillRunning={#AppName} לא נסגר. סגור אותו ולחץ על "נסה שוב".
en.RemoveData=Also delete your {#AppName} settings, library and history?
he.RemoveData=למחוק גם את ההגדרות, הספרייה וההיסטוריה של {#AppName}?
en.ShortcutComment=VLC media player with the interface designed by Juxtopposed
he.ShortcutComment=נגן VLC עם הממשק שעיצבה Juxtopposed

[Files]
; The wizard's pieces and fonts: read by the [Code], never installed
Source: "art\ui\*.png"; Flags: dontcopy noencryption
Source: "art\fonts\*.ttf"; Flags: dontcopy noencryption
Source: "notice.txt"; Flags: dontcopy noencryption
; portable\ holds the user's settings and library: never shipped, never replaced
Source: "{#SourceDir}\*"; DestDir: "{app}"; Excludes: "\portable\*,*.old-*"; Flags: recursesubdirs createallsubdirs ignoreversion

[Dirs]
; Its presence keeps VLC's data next to vlc.exe instead of %APPDATA%\vlc,
; which an installed official VLC uses
Name: "{app}\portable"; Flags: uninsneveruninstall; Check: not IsAdminInstallMode
; Installed for all users (Program Files): its users must be able to write
; their settings there
Name: "{app}\portable"; Flags: uninsneveruninstall; Permissions: users-modify; Check: IsAdminInstallMode

[InstallDelete]
; Plugins of a previous version that this one no longer has
Type: filesandordirs; Name: "{app}\plugins"
Type: files; Name: "{app}\plugins.dat"
; A small update the app staged and never put in place: this one is newer
Type: filesandordirs; Name: "{app}\.update"

[UninstallDelete]
; What small updates added since the install (the uninstaller only knows the
; files it installed), and their working folder; portable\ stays
Type: filesandordirs; Name: "{app}\.update"
Type: filesandordirs; Name: "{app}\plugins"
Type: filesandordirs; Name: "{app}\lua"
Type: filesandordirs; Name: "{app}\skins"
Type: filesandordirs; Name: "{app}\hrtfs"
Type: filesandordirs; Name: "{app}\licenses"
Type: filesandordirs; Name: "{app}\sdk"
Type: files; Name: "{app}\*.dll"

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\vlc.exe"; Comment: "{cm:ShortcutComment}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\vlc.exe"; Comment: "{cm:ShortcutComment}"; Check: WantDesktopIcon

[Run]
; Reopen VLC as it was (update or running instance); otherwise the last
; page's "Open VLC Redesign now" does (CurStepChanged)
Filename: "{app}\vlc.exe"; Parameters: "{code:RelaunchArgs}"; Flags: nowait; Check: ShouldRelaunch

[Code]
const
  { The wizard's client area at 100 % DPI: every position below is in its
    units, scaled to the window (as the art: see make-art.py) }
  BASE_W = 596;
  BASE_H = 432;
  CONTENT_L = 190;
  CONTENT_W = 390;
  BODY_T = 100;
  BODY_H = 270;
  BUTTONS_T = 380;
  TOGGLE_L = 512;

  { VLCStyle.redesign, as TColor ($BBGGRR) }
  CLR_BACK = $0E0E0E;
  CLR_CARD = $191919;
  CLR_WELL = $090909;
  CLR_PILL = $061F37;
  CLR_BUTTON = $1B1B1B;
  CLR_BUTTON_HOVER = $222222;
  CLR_BUTTON_OFF = $151515;
  CLR_ACCENT_BACK = $061F37;
  CLR_ACCENT_HOVER = $07243F;
  CLR_ACCENT_BACK_OFF = $0A1622;
  CLR_TEXT = $EAEAEA;
  CLR_TEXT2 = $969696;
  CLR_MUTED = $8F8F8F;
  CLR_MUTED_OFF = $555555;
  CLR_FAINT = $6B6B6B;
  CLR_ACCENT = $1879D3;
  CLR_ACCENT_DIM = $1463AB;
  CLR_ACCENT_OFF = $0F4C85;

  ACT_CANCEL = 1;
  ACT_BACK = 2;
  ACT_NEXT = 3;
  ACT_BROWSE = 4;
  ACT_DEFAULTAPPS = 5;

  T_VIDEO = 0;
  T_AUDIO = 1;
  T_PLAYLIST = 2;
  T_CONTEXT = 3;
  T_DESKTOP = 4;
  T_LAUNCH = 5;

  FR_PRIVATE = $10;

  { The types VLC plays (extras/package/win32/NSIS/helpers/extensions.nsh) }
  VIDEO_EXTS = '.3g2 .3gp .3gp2 .3gpp .amrec .amv .asf .avi .bik .dav .divx .drc .dv .evo .f4v .flv .gvi .gxf .k3g .m1v .m2t .m2v .m2ts .m4v .mkv .mov .mp2v .mp4 .mp4v .mpa .mpe .mpeg .mpeg1 .mpeg2 .mpeg4 .mpg .mpv2 .mts .mtv .mxf .nsv .nuv .ogg .ogm .ogx .ogv .qt .rec .rm .rmvb .rpl .skm .thp .tod .tp .ts .tts .vob .vp6 .vro .webm .wmv .wtv .xesc';
  AUDIO_EXTS = '.3ga .669 .a52 .aac .ac3 .adt .adts .aif .aifc .aiff .alac .amr .aob .au .ape .caf .cda .dts .dsf .dff .flac .it .m4a .m4p .mka .mlp .mod .mp1 .mp2 .mp3 .mpc .mpga .oga .oma .opus .qcp .ra .rmi .snd .s3m .spx .tak .tta .voc .vqf .w64 .wav .wma .wv .xa .xm';
  PLAYLIST_EXTS = '.asx .b4s .cue .ifo .m3u .m3u8 .pls .ram .sdp .vlc .wvx .xspf .wpl .zpl';
  { "Open with" of 0.1 and 0.2, replaced by the types above }
  OLD_EXTS = '.mp4 .m4v .mkv .webm .avi .mov .wmv .flv .ts .m2ts .mpg .mpeg .3gp .ogv .mp3 .m4a .aac .flac .wav .ogg .opus .wma .m3u .m3u8 .xspf';

  PROGID_PREFIX = 'VLCRedesign';
  BACKUP_VALUE = 'VLCRedesign.backup';
  CAPABILITIES = 'Software\{#AppName}\Capabilities';

type
  TWinPoint = record
    X, Y: Longint;
  end;
  TWinRect = record
    Left, Top, Right, Bottom: Longint;
  end;

  { A button of the redesign's dialogs (RedesignButton.qml): its box, the
    label on it, and what it does }
  TDesignButton = record
    Box: TBitmapButton;
    Text: TLabel;
    Kind: String;
    Action: Integer;
    Hover: Boolean;
    Off: Boolean;
  end;

  { An ON / OFF toggle of the settings (SettingsToggle.qml) and its label }
  TDesignToggle = record
    Box: TBitmapButton;
    Text: TLabel;
    Checked: Boolean;
  end;

var
  Scale: Integer;
  FontsLoaded: Boolean;
  IsRTL: Boolean;
  TimerId: Longword;
  WasRunning: Boolean;
  RunningArgs: String;
  Relaunching: Boolean;

  OptionsPage: TWizardPage;
  Buttons: array of TDesignButton;
  Toggles: array of TDesignToggle;
  NavCancel, NavBack, NavNext: Integer;

  SideMark, Pill: TBitmapImage;
  SideTitle, SideSub, SideBy, SideName: TLabel;
  StepNum, StepText: array[0..3] of TLabel;
  CurStep: Integer;

  InnerTitle, InnerText: TLabel;
  Track, Fill: TBitmapImage;
  StatusText, FileText: TLabel;
  FinishCard: TBitmapImage;
  FinishCardTitle, FinishCardText: TLabel;
  DefaultAppsButton: Integer;
  ForEveryoneLink: TLabel;

function AddFontResourceEx(FileName: String; Flags: Longword; Reserved: Longword): Integer;
  external 'AddFontResourceExW@gdi32.dll stdcall';
function RemoveFontResourceEx(FileName: String; Flags: Longword; Reserved: Longword): Integer;
  external 'RemoveFontResourceExW@gdi32.dll stdcall';
function SetTimer(Wnd: Longword; IdEvent, Elapse: Longword; TimerFunc: Longword): Longword;
  external 'SetTimer@user32.dll stdcall';
function KillTimer(Wnd: Longword; IdEvent: Longword): Boolean;
  external 'KillTimer@user32.dll stdcall';
function GetCursorPos(var Point: TWinPoint): Boolean;
  external 'GetCursorPos@user32.dll stdcall';
function GetWindowRect(Wnd: HWND; var Rect: TWinRect): Boolean;
  external 'GetWindowRect@user32.dll stdcall';

{ ---- Closing and reopening a running copy --------------------------------- }

function AppExe(): String;
begin
  Result := ExpandConstant('{app}\vlc.exe');
end;

{ Process ids (as taskkill arguments) and command line of the VLC Redesign
  instances started from the install folder; other VLCs are left alone. }
function FindRunning(var Pids: String; var CmdLine: String): Boolean;
var
  Locator, Service, Procs, Proc: Variant;
  I: Integer;
begin
  Result := False;
  Pids := '';
  CmdLine := '';
  try
    Locator := CreateOleObject('WbemScripting.SWbemLocator');
    Service := Locator.ConnectServer('.', 'root\CIMV2');
    Procs := Service.ExecQuery('SELECT ProcessId, ExecutablePath, CommandLine FROM Win32_Process WHERE Name = ''vlc.exe''');
    for I := 0 to Procs.Count - 1 do
    begin
      Proc := Procs.ItemIndex(I);
      if (not VarIsNull(Proc.ExecutablePath)) and (CompareText(Proc.ExecutablePath, AppExe()) = 0) then
      begin
        Result := True;
        Pids := Pids + ' /PID ' + IntToStr(Proc.ProcessId);
        if not VarIsNull(Proc.CommandLine) then
          CmdLine := Proc.CommandLine;
      end;
    end;
  except
    Log('Cannot list processes: ' + GetExceptionMessage);
  end;
end;

{ The arguments of a command line, without the program }
function ArgsOf(CmdLine: String): String;
var
  P: Integer;
begin
  CmdLine := Trim(CmdLine);
  if Copy(CmdLine, 1, 1) = '"' then
  begin
    Delete(CmdLine, 1, 1);
    P := Pos('"', CmdLine);
  end
  else
    P := Pos(' ', CmdLine);
  if P = 0 then
    Result := ''
  else
    Result := Trim(Copy(CmdLine, P + 1, Length(CmdLine)));
end;

function IsUpdate(): Boolean;
begin
  Result := ExpandConstant('{param:UPDATE|0}') <> '0';
end;

function ShouldRelaunch(): Boolean;
begin
  Result := WasRunning or (ExpandConstant('{param:RESUME|}') <> '');
end;

function RelaunchArgs(Param: String): String;
var
  Media, Time: String;
begin
  Media := ExpandConstant('{param:RESUME|}');
  Time := ExpandConstant('{param:TIME|}');
  if Media <> '' then
  begin
    { Not :start-time, which makes the media a clip that starts there }
    Result := '"' + Media + '"';
    if (Time <> '') and (StrToIntDef(Time, -1) > 0) then
      Result := '--redesign-resume-time=' + Time + ' ' + Result;
  end
  else
    Result := RunningArgs;
end;

function CloseRunning(): Boolean;
var
  Pids, CmdLine: String;
  Code, I: Integer;
begin
  Result := True;
  if not FindRunning(Pids, CmdLine) then
    Exit;
  WasRunning := True;
  RunningArgs := ArgsOf(CmdLine);
  if (not WizardSilent) and (not IsUpdate()) then
    MsgBox(CustomMessage('Running'), mbInformation, MB_OK);
  { Ask politely (WM_CLOSE) so VLC saves its settings and position }
  Exec(ExpandConstant('{sys}\taskkill.exe'), Trim(Pids), '', SW_HIDE, ewWaitUntilTerminated, Code);
  for I := 1 to 30 do
  begin
    Sleep(500);
    if not FindRunning(Pids, CmdLine) then
      Exit;
  end;
  Exec(ExpandConstant('{sys}\taskkill.exe'), '/F ' + Trim(Pids), '', SW_HIDE, ewWaitUntilTerminated, Code);
  Sleep(1000);
  Result := not FindRunning(Pids, CmdLine);
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  if CloseRunning() then
    Result := ''
  else
    Result := CustomMessage('StillRunning');
end;

{ ---- Layout -------------------------------------------------------------- }

{ (in floating point: "/" of two integers divides as "div" here) }
function SX(V: Integer): Integer;
var
  F: Extended;
begin
  F := V;
  Result := Round(F * WizardForm.ClientWidth / BASE_W);
end;

function SY(V: Integer): Integer;
var
  F: Extended;
begin
  F := V;
  Result := Round(F * WizardForm.ClientHeight / BASE_H);
end;

{ The height of a font of Px pixels at 100 % }
function FontPx(Px: Integer): Integer;
begin
  Result := -SY(Px);
end;

{ Places C at (L, T), W x H in the units of the 100 % window. Body: C sits
  on an inner page, whose notebook starts at (CONTENT_L, BODY_T). }
procedure Place(C: TControl; Body: Boolean; L, T, W, H: Integer);
var
  X0, Y0: Integer;
begin
  X0 := 0;
  Y0 := 0;
  if Body then
  begin
    X0 := SX(CONTENT_L);
    Y0 := SY(BODY_T);
  end;
  C.SetBounds(SX(L) - X0, SY(T) - Y0, SX(L + W) - SX(L), SY(T + H) - SY(T));
end;

{ The size of the art made for this window (make-art.py SIZES) }
function PickScale(): Integer;
var
  Widths: array[0..6] of Integer;
  I, D, Best: Integer;
begin
  Widths[0] := 596; Widths[1] := 796; Widths[2] := 994; Widths[3] := 1193;
  Widths[4] := 1272; Widths[5] := 1471; Widths[6] := 1630;
  Result := 0;
  Best := MaxInt;
  for I := 0 to 6 do
  begin
    D := Widths[I] - WizardForm.ClientWidth;
    if D < 0 then
      D := -D;
    if D < Best then
    begin
      Best := D;
      Result := I;
    end;
  end;
end;

function Png(Name: String): TPngImage;
var
  FileName: String;
begin
  FileName := Name + '-' + IntToStr(Scale) + '.png';
  if not FileExists(ExpandConstant('{tmp}\' + FileName)) then
    ExtractTemporaryFile(FileName);
  Result := TPngImage.Create;
  Result.LoadFromFile(ExpandConstant('{tmp}\' + FileName));
end;

procedure SetImage(Image: TBitmapImage; Name: String);
var
  P: TPngImage;
begin
  P := Png(Name);
  try
    Image.PngImage := P;
  finally
    P.Free;
  end;
end;

procedure SetBoxImage(Box: TBitmapButton; Name: String);
var
  P: TPngImage;
begin
  P := Png(Name);
  try
    Box.PngImage := P;
  finally
    P.Free;
  end;
end;

function NewImage(Parent: TWinControl; Body: Boolean; Name: String; L, T, W, H: Integer): TBitmapImage;
begin
  Result := TBitmapImage.Create(WizardForm);
  Result.Parent := Parent;
  Result.Stretch := True;
  SetImage(Result, Name);
  Place(Result, Body, L, T, W, H);
end;

{ ---- Text ---------------------------------------------------------------- }

function Face(Bold: Boolean): String;
begin
  { Inter has no Hebrew: Segoe UI then, as Windows draws Hebrew }
  if FontsLoaded and (ActiveLanguage <> 'he') then
  begin
    if Bold then
      Result := 'Inter Setup SemiBold'
    else
      Result := 'Inter Setup';
  end
  else if Bold then
    Result := 'Segoe UI Semibold'
  else
    Result := 'Segoe UI';
end;

procedure StyleText(L: TLabel; Px: Integer; Bold: Boolean; Color, Back: TColor);
begin
  L.StyleElements := [];
  L.Font.Name := Face(Bold);
  L.Font.Height := FontPx(Px);
  L.Font.Color := Color;
  L.Color := Back;
end;

{ A line of text at (L, T); W > 0 wraps it in that width }
function NewLabel(Parent: TWinControl; Body: Boolean; L, T, W, Px: Integer; Bold: Boolean;
  Color, Back: TColor; Text: String): TLabel;
var
  X0, Y0: Integer;
begin
  Result := TLabel.Create(WizardForm);
  Result.Parent := Parent;
  StyleText(Result, Px, Bold, Color, Back);
  Result.Caption := Text;
  X0 := 0;
  Y0 := 0;
  if Body then
  begin
    X0 := SX(CONTENT_L);
    Y0 := SY(BODY_T);
  end;
  if W > 0 then
  begin
    Result.AutoSize := False;
    Result.WordWrap := True;
    Result.Width := SX(L + W) - SX(L);
    Result.AutoSize := True;
    if IsRTL then
      Result.Alignment := taRightJustify;
  end;
  Result.Left := SX(L) - X0;
  Result.Top := SY(T) - Y0;
end;

procedure OpenUrl(Url: String);
var
  Code: Integer;
begin
  ShellExecAsOriginalUser('open', Url, '', '', SW_SHOWNORMAL, ewNoWait, Code);
end;

procedure VideoClick(Sender: TObject);
begin
  OpenUrl('https://www.youtube.com/watch?v=ERFeOgqk324');
end;

procedure ChannelClick(Sender: TObject);
begin
  OpenUrl('https://www.youtube.com/@juxtopposed');
end;

procedure FigmaClick(Sender: TObject);
begin
  OpenUrl('https://www.figma.com/community/file/1691465999134370980');
end;

procedure SourceClick(Sender: TObject);
begin
  OpenUrl('{#RepoUrl}');
end;

function NewLink(Parent: TWinControl; Body: Boolean; L, T, Px: Integer; Back: TColor; Text: String;
  OnClick: TNotifyEvent): TLabel;
begin
  Result := NewLabel(Parent, Body, L, T, 0, Px, False, CLR_ACCENT, Back, Text);
  Result.Cursor := crHand;
  Result.OnClick := OnClick;
end;

{ ---- Buttons ------------------------------------------------------------- }

procedure PaintButton(I: Integer);
var
  Name: String;
  Back, Fore: TColor;
begin
  Name := Buttons[I].Kind;
  if Buttons[I].Off then
    Name := Name + '-off'
  else if Buttons[I].Hover then
    Name := Name + '-hover';
  SetBoxImage(Buttons[I].Box, Name);
  if Pos('accent', Buttons[I].Kind) = 1 then
  begin
    if Buttons[I].Off then begin Back := CLR_ACCENT_BACK_OFF; Fore := CLR_ACCENT_OFF; end
    else if Buttons[I].Hover then begin Back := CLR_ACCENT_HOVER; Fore := CLR_ACCENT; end
    else begin Back := CLR_ACCENT_BACK; Fore := CLR_ACCENT; end;
  end
  else
  begin
    if Buttons[I].Off then begin Back := CLR_BUTTON_OFF; Fore := CLR_MUTED_OFF; end
    else if Buttons[I].Hover then begin Back := CLR_BUTTON_HOVER; Fore := CLR_MUTED; end
    else begin Back := CLR_BUTTON; Fore := CLR_MUTED; end;
  end;
  Buttons[I].Text.Color := Back;
  Buttons[I].Text.Font.Color := Fore;
end;

procedure CentreButtonText(I: Integer);
begin
  with Buttons[I] do
  begin
    Text.Left := (Box.ClientWidth - Text.Width) div 2;
    Text.Top := (Box.ClientHeight - Text.Height) div 2;
  end;
end;

function ButtonOf(Sender: TObject): Integer;
var
  I: Integer;
begin
  Result := -1;
  for I := 0 to GetArrayLength(Buttons) - 1 do
    if (Sender = Buttons[I].Box) or (Sender = Buttons[I].Text) then
      Result := I;
end;

procedure BrowseClick();
var
  Dir: String;
begin
  Dir := WizardForm.DirEdit.Text;
  if BrowseForFolder(SetupMessage(msgBrowseDialogLabel), Dir, True) then
  begin
    { as the folder page does: the program's own folder in the one picked }
    if CompareText(ExtractFileName(RemoveBackslashUnlessRoot(Dir)), '{#AppName}') <> 0 then
      Dir := AddBackslash(Dir) + '{#AppName}';
    WizardForm.DirEdit.Text := Dir;
  end;
end;

procedure DefaultAppsClick();
var
  Url: String;
  Version: TWindowsVersion;
begin
  { Windows 11 opens the page of the app (with "Set default"), Windows 10
    the list of default apps }
  GetWindowsVersionEx(Version);
  Url := 'ms-settings:defaultapps';
  if Version.Build >= 22000 then
  begin
    if IsAdminInstallMode then
      Url := Url + '?registeredAppMachine=VLC%20Redesign'
    else
      Url := Url + '?registeredAppUser=VLC%20Redesign';
  end;
  OpenUrl(Url);
end;

procedure DesignButtonClick(Sender: TObject);
var
  I: Integer;
begin
  I := ButtonOf(Sender);
  if (I < 0) or Buttons[I].Off or not Buttons[I].Box.Visible then
    Exit;
  case Buttons[I].Action of
    ACT_CANCEL: WizardForm.CancelButton.OnClick(WizardForm.CancelButton);
    ACT_BACK: WizardForm.BackButton.OnClick(WizardForm.BackButton);
    ACT_NEXT: WizardForm.NextButton.OnClick(WizardForm.NextButton);
    ACT_BROWSE: BrowseClick();
    ACT_DEFAULTAPPS: DefaultAppsClick();
  end;
end;

function NewButton(Parent: TWinControl; Body: Boolean; Kind: String; L, T, W: Integer; Caption: String;
  Action: Integer): Integer;
var
  I: Integer;
begin
  I := GetArrayLength(Buttons);
  SetArrayLength(Buttons, I + 1);
  Buttons[I].Kind := Kind;
  Buttons[I].Action := Action;
  Buttons[I].Box := TBitmapButton.Create(WizardForm);
  Buttons[I].Box.Parent := Parent;
  Buttons[I].Box.Stretch := True;
  Buttons[I].Box.Caption := Caption;
  Buttons[I].Box.Cursor := crHand;
  Buttons[I].Box.OnClick := @DesignButtonClick;
  Place(Buttons[I].Box, Body, L, T, W, 32);
  Buttons[I].Text := TLabel.Create(WizardForm);
  Buttons[I].Text.Parent := Buttons[I].Box;
  StyleText(Buttons[I].Text, 14, True, CLR_MUTED, CLR_BUTTON);
  Buttons[I].Text.Caption := Caption;
  Buttons[I].Text.Cursor := crHand;
  Buttons[I].Text.OnClick := @DesignButtonClick;
  PaintButton(I);
  CentreButtonText(I);
  Result := I;
end;

{ The labels of Setup's own buttons, as the design writes them }
function CleanCaption(S: String): String;
begin
  StringChangeEx(S, '&', '', True);
  StringChangeEx(S, '<', '', True);
  StringChangeEx(S, '>', '', True);
  Result := Trim(S);
end;

{ Our buttons stand for Setup's own, kept out of sight (they still answer
  Enter, Escape and their access keys) }
procedure Mirror(I: Integer; Native: TNewButton);
var
  Caption: String;
begin
  if (Native.Left > -1000) and (Native.Left < WizardForm.ClientWidth) then
    Native.Left := -2000;
  Buttons[I].Box.Visible := Native.Visible;
  if Buttons[I].Off <> not Native.Enabled then
  begin
    Buttons[I].Off := not Native.Enabled;
    Buttons[I].Box.Enabled := Native.Enabled;
    PaintButton(I);
  end;
  Caption := CleanCaption(Native.Caption);
  if Buttons[I].Text.Caption <> Caption then
  begin
    Buttons[I].Text.Caption := Caption;
    Buttons[I].Box.Caption := Caption;
    CentreButtonText(I);
  end;
end;

procedure SyncButtons();
begin
  Mirror(NavCancel, WizardForm.CancelButton);
  Mirror(NavBack, WizardForm.BackButton);
  Mirror(NavNext, WizardForm.NextButton);
end;

procedure UpdateHover();
var
  P: TWinPoint;
  R: TWinRect;
  I: Integer;
  Inside: Boolean;
begin
  if not GetCursorPos(P) then
    Exit;
  for I := 0 to GetArrayLength(Buttons) - 1 do
    if Buttons[I].Box.Visible and not Buttons[I].Off then
    begin
      GetWindowRect(Buttons[I].Box.Handle, R);
      Inside := (P.X >= R.Left) and (P.X < R.Right) and (P.Y >= R.Top) and (P.Y < R.Bottom);
      if Inside <> Buttons[I].Hover then
      begin
        Buttons[I].Hover := Inside;
        PaintButton(I);
      end;
    end;
end;

procedure OnTimer(Wnd: Longword; Msg: Longword; IdEvent: Longword; Time: Longword);
begin
  SyncButtons();
  UpdateHover();
  if WizardForm.CurPageID = wpInstalling then
  begin
    StatusText.Caption := WizardForm.StatusLabel.Caption;
    FileText.Caption := WizardForm.FilenameLabel.Caption;
  end;
end;

{ ---- Toggles ------------------------------------------------------------- }

function ToggleOf(Sender: TObject): Integer;
var
  I: Integer;
begin
  Result := -1;
  for I := 0 to GetArrayLength(Toggles) - 1 do
    if (Sender = Toggles[I].Box) or (Sender = Toggles[I].Text) then
      Result := I;
end;

procedure PaintToggle(I: Integer);
begin
  if Toggles[I].Checked then
  begin
    SetBoxImage(Toggles[I].Box, 'toggle-on');
    Toggles[I].Box.Caption := Toggles[I].Text.Caption + ': ON';
  end
  else
  begin
    SetBoxImage(Toggles[I].Box, 'toggle-off');
    Toggles[I].Box.Caption := Toggles[I].Text.Caption + ': OFF';
  end;
end;

procedure ToggleClick(Sender: TObject);
var
  I: Integer;
begin
  I := ToggleOf(Sender);
  if I < 0 then
    Exit;
  Toggles[I].Checked := not Toggles[I].Checked;
  PaintToggle(I);
end;

{ A row of the settings: the label, then the toggle at the right }
procedure NewToggle(Index: Integer; Parent: TWinControl; Body: Boolean; T: Integer; Caption: String;
  Checked: Boolean);
begin
  if GetArrayLength(Toggles) <= Index then
    SetArrayLength(Toggles, Index + 1);
  Toggles[Index].Checked := Checked;
  Toggles[Index].Text := NewLabel(Parent, Body, CONTENT_L, T + 9, 0, 14, False, CLR_MUTED, CLR_BACK, Caption);
  Toggles[Index].Text.Cursor := crHand;
  Toggles[Index].Text.OnClick := @ToggleClick;
  Toggles[Index].Box := TBitmapButton.Create(WizardForm);
  Toggles[Index].Box.Parent := Parent;
  Toggles[Index].Box.Stretch := True;
  Toggles[Index].Box.Cursor := crHand;
  Toggles[Index].Box.OnClick := @ToggleClick;
  Place(Toggles[Index].Box, Body, TOGGLE_L, T + 2, 68, 32);
  PaintToggle(Index);
end;

function Checked(Index: Integer): Boolean;
begin
  Result := (GetArrayLength(Toggles) > Index) and Toggles[Index].Checked;
end;

function WantDesktopIcon(): Boolean;
begin
  Result := Checked(T_DESKTOP);
end;

{ ---- The sidebar --------------------------------------------------------- }

procedure ShowStep(Step: Integer; Parent: TWinControl);
var
  I: Integer;
begin
  CurStep := Step;
  SideMark.Parent := Parent;
  Pill.Parent := Parent;
  SideTitle.Parent := Parent;
  SideSub.Parent := Parent;
  SideBy.Parent := Parent;
  SideName.Parent := Parent;
  Pill.Top := SY(84 + 36 * Step);
  for I := 0 to 3 do
  begin
    StepNum[I].Parent := Parent;
    StepText[I].Parent := Parent;
    if I = Step then
    begin
      StepNum[I].Caption := IntToStr(I + 1);
      StepNum[I].Font.Color := CLR_ACCENT;
      StepNum[I].Color := CLR_PILL;
      StepText[I].Font.Color := CLR_ACCENT;
      StepText[I].Color := CLR_PILL;
    end
    else
    begin
      StepNum[I].Color := CLR_CARD;
      StepText[I].Color := CLR_CARD;
      if I < Step then
      begin
        StepNum[I].Caption := #$2713;
        StepNum[I].Font.Color := CLR_ACCENT_DIM;
        StepText[I].Font.Color := CLR_MUTED;
      end
      else
      begin
        StepNum[I].Caption := IntToStr(I + 1);
        StepNum[I].Font.Color := CLR_FAINT;
        StepText[I].Font.Color := CLR_FAINT;
      end;
    end;
  end;
end;

procedure CreateSidebar();
var
  Parent: TWinControl;
  Names: array[0..3] of String;
  I: Integer;
begin
  Parent := WizardForm.WelcomePage;
  SideMark := NewImage(Parent, False, 'mark', 24, 24, 28, 28);
  SideTitle := NewLabel(Parent, False, 60, 24, 0, 14, True, CLR_TEXT, CLR_CARD, '{#AppName}');
  SideSub := NewLabel(Parent, False, 60, 42, 0, 11, False, CLR_FAINT, CLR_CARD, CustomMessage('Unofficial'));
  Pill := NewImage(Parent, False, 'pill', 18, 84, 144, 32);
  Names[0] := CustomMessage('StepWelcome');
  Names[1] := CustomMessage('StepOptions');
  Names[2] := CustomMessage('StepInstall');
  Names[3] := CustomMessage('StepDone');
  for I := 0 to 3 do
  begin
    StepNum[I] := NewLabel(Parent, False, 30, 84 + 36 * I + 8, 0, 13, True, CLR_FAINT, CLR_CARD, IntToStr(I + 1));
    StepText[I] := NewLabel(Parent, False, 50, 84 + 36 * I + 8, 0, 13, False, CLR_FAINT, CLR_CARD, Names[I]);
  end;
  SideBy := NewLabel(Parent, False, 24, 368, 0, 11, False, CLR_FAINT, CLR_CARD, CustomMessage('DesignedBy'));
  SideName := NewLink(Parent, False, 24, 384, 13, CLR_CARD, 'Juxtopposed', @ChannelClick);
  SideName.Font.Name := Face(True);
end;

{ ---- The notice ---------------------------------------------------------- }

procedure NoticeClick(Sender: TObject);
var
  Form: TSetupForm;
  Memo: TNewMemo;
  Close: TNewButton;
  Lines: TArrayOfString;
  Text: String;
  I: Integer;
begin
  ExtractTemporaryFile('notice.txt');
  Text := '';
  if LoadStringsFromFile(ExpandConstant('{tmp}\notice.txt'), Lines) then
    for I := 0 to GetArrayLength(Lines) - 1 do
      Text := Text + Lines[I] + #13#10;
  Form := CreateCustomForm(SX(480), SY(380), False, False);
  try
    Form.Caption := CustomMessage('NoticeTitle');
    Memo := TNewMemo.Create(Form);
    Memo.Parent := Form;
    Memo.SetBounds(SX(16), SY(16), Form.ClientWidth - SX(32), Form.ClientHeight - SY(16 + 16 + 32 + 16));
    Memo.ReadOnly := True;
    Memo.ScrollBars := ssVertical;
    Memo.BorderStyle := bsNone;
    Memo.StyleElements := [seBorder];
    Memo.Color := CLR_WELL;
    Memo.Font.Name := Face(False);
    Memo.Font.Height := FontPx(12);
    Memo.Font.Color := CLR_TEXT2;
    Memo.Text := Text;
    Close := TNewButton.Create(Form);
    Close.Parent := Form;
    Close.Caption := CustomMessage('Close');
    Close.SetBounds(Form.ClientWidth - SX(16 + 100), Form.ClientHeight - SY(16 + 32), SX(100), SY(32));
    Close.ModalResult := mrOk;
    Close.Cancel := True;
    Close.Default := True;
    Form.ActiveControl := Close;
    Form.ShowModal();
  finally
    Form.Free();
  end;
end;

{ ---- "Install for everyone": Setup again, elevated ------------------------ }

procedure ForEveryoneClick(Sender: TObject);
var
  Code: Integer;
begin
  if ShellExec('runas', ExpandConstant('{srcexe}'), '/ALLUSERS /LANG=' + ActiveLanguage, '', SW_SHOWNORMAL,
               ewNoWait, Code) then
  begin
    Relaunching := True;
    WizardForm.Close();
  end;
end;

procedure CancelButtonClick(CurPageID: Integer; var Cancel, Confirm: Boolean);
begin
  if Relaunching then
    Confirm := False;
end;

{ ---- The pages ----------------------------------------------------------- }

procedure CreateWelcome();
var
  P: TWinControl;
  L: TLabel;
  X: Integer;
begin
  P := WizardForm.WelcomePage;
  NewLabel(P, False, CONTENT_L, 24, CONTENT_W, 22, True, CLR_TEXT, CLR_BACK, CustomMessage('WelcomeTitle'));
  NewLabel(P, False, CONTENT_L, 58, CONTENT_W, 13, False, CLR_TEXT2, CLR_BACK, CustomMessage('WelcomeText'));
  NewImage(P, False, 'card-credit', CONTENT_L, 100, CONTENT_W, 120);
  NewLabel(P, False, CONTENT_L + 16, 114, CONTENT_W - 32, 14, True, CLR_TEXT, CLR_CARD, CustomMessage('CreditTitle'));
  NewLabel(P, False, CONTENT_L + 16, 138, CONTENT_W - 32, 12, False, CLR_TEXT2, CLR_CARD, CustomMessage('CreditText'));
  { the links in a row, as in About }
  L := NewLink(P, False, CONTENT_L + 16, 192, 12, CLR_CARD, CustomMessage('LinkVideo'), @VideoClick);
  X := L.Left + L.Width + SX(16);
  L := NewLink(P, False, CONTENT_L + 16, 192, 12, CLR_CARD, CustomMessage('LinkChannel'), @ChannelClick);
  L.Left := X;
  X := L.Left + L.Width + SX(16);
  L := NewLink(P, False, CONTENT_L + 16, 192, 12, CLR_CARD, CustomMessage('LinkFigma'), @FigmaClick);
  L.Left := X;
  NewLabel(P, False, CONTENT_L, 236, CONTENT_W, 11, False, CLR_FAINT, CLR_BACK, CustomMessage('WelcomeNotice'));
  L := NewLink(P, False, CONTENT_L, 292, 12, CLR_BACK, CustomMessage('LinkNotice'), @NoticeClick);
  X := L.Left + L.Width + SX(16);
  L := NewLink(P, False, CONTENT_L, 292, 12, CLR_BACK, CustomMessage('LinkSource'), @SourceClick);
  L.Left := X;
  NewLabel(P, False, CONTENT_L, 350, CONTENT_W, 11, False, CLR_FAINT, CLR_BACK,
           FmtMessage(CustomMessage('VersionLine'), ['{#AppVersion}']));
end;

procedure CreateOptions();
var
  P: TWinControl;
  Edit: TNewPathEdit;
  Upgrade: Boolean;
  Note: TLabel;
begin
  OptionsPage := CreateCustomPage(wpWelcome, CustomMessage('OptionsTitle'), CustomMessage('OptionsText'));
  P := OptionsPage.Surface;
  Upgrade := WizardForm.PrevAppDir <> '';

  NewLabel(P, True, CONTENT_L, 100, 0, 13, False, CLR_MUTED, CLR_BACK, CustomMessage('InstallTo'));
  NewImage(P, True, 'well', CONTENT_L, 120, 282, 36);
  { the folder field of the design: borderless, in the well drawn above }
  Edit := WizardForm.DirEdit;
  Edit.Parent := P;
  Edit.BorderStyle := bsNone;
  Edit.StyleElements := [];
  Edit.Color := CLR_WELL;
  Edit.Font.Name := Face(False);
  Edit.Font.Height := FontPx(13);
  Edit.Font.Color := CLR_MUTED;
  Place(Edit, True, CONTENT_L + 12, 129, 258, 18);
  Edit.Top := SY(120) - SY(BODY_T) + (SY(156) - SY(120) - Edit.Height) div 2;
  if Upgrade then
  begin
    Edit.ReadOnly := True;
    NewLabel(P, True, CONTENT_L, 162, 0, 11, False, CLR_FAINT, CLR_BACK, CustomMessage('UpdatingHere'));
  end
  else
  begin
    NewButton(P, True, 'button', CONTENT_L + CONTENT_W - 100, 122, 100, CustomMessage('Browse'), ACT_BROWSE);
    if IsAdminInstallMode then
      NewLabel(P, True, CONTENT_L, 162, 0, 11, False, CLR_FAINT, CLR_BACK, CustomMessage('ForAll'))
    else
    begin
      Note := NewLabel(P, True, CONTENT_L, 162, 0, 11, False, CLR_FAINT, CLR_BACK, CustomMessage('ForYou'));
      ForEveryoneLink := NewLink(P, True, CONTENT_L, 162, 11, CLR_BACK, CustomMessage('ForEveryone'), @ForEveryoneClick);
      ForEveryoneLink.Left := Note.Left + Note.Width + SX(6);
    end;
  end;
end;

procedure CreateOptionToggles();
var
  P: TWinControl;
  Fresh: Boolean;
  OldTasks, Def: String;
begin
  P := OptionsPage.Surface;
  { A first install offers everything; an update keeps the last choices
    (and from 0.1 or 0.2, which had no such options: the desktop shortcut
    as it was, nothing else) }
  Fresh := WizardForm.PrevAppDir = '';
  OldTasks := '';
  RegQueryStringValue(HKA, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{{#AppIdGuid}}_is1',
                      'Inno Setup: Selected Tasks', OldTasks);
  if Fresh then Def := '1' else Def := '0';
  NewToggle(T_VIDEO, P, True, 184, CustomMessage('OpenVideo'), GetPreviousData('OpenVideo', Def) = '1');
  NewToggle(T_AUDIO, P, True, 220, CustomMessage('OpenAudio'), GetPreviousData('OpenAudio', Def) = '1');
  NewToggle(T_PLAYLIST, P, True, 256, CustomMessage('OpenPlaylists'), GetPreviousData('OpenPlaylists', Def) = '1');
  NewToggle(T_CONTEXT, P, True, 292, CustomMessage('ContextMenu'), GetPreviousData('ContextMenu', Def) = '1');
  if Fresh or (Pos('desktopicon', OldTasks) > 0) then Def := '1' else Def := '0';
  NewToggle(T_DESKTOP, P, True, 328, CustomMessage('DesktopIcon'), GetPreviousData('DesktopIcon', Def) = '1');
end;

procedure CreateInstalling();
var
  P: TWinControl;
begin
  P := WizardForm.InstallingPage;
  WizardForm.StatusLabel.Visible := False;
  WizardForm.FilenameLabel.Visible := False;
  WizardForm.ProgressGauge.Visible := False;
  Track := NewImage(P, True, 'track', CONTENT_L, 104, CONTENT_W, 6);
  Fill := TBitmapImage.Create(WizardForm);
  Fill.Parent := P;
  { the whole bar, shown up to the progress }
  SetImage(Fill, 'fill');
  Fill.Stretch := False;
  Fill.Center := False;
  Place(Fill, True, CONTENT_L, 104, CONTENT_W, 6);
  Fill.Width := 0;
  StatusText := NewLabel(P, True, CONTENT_L, 122, 0, 13, False, CLR_MUTED, CLR_BACK, '');
  StatusText.AutoSize := False;
  StatusText.Width := SX(CONTENT_W);
  StatusText.Height := SY(18);
  FileText := NewLabel(P, True, CONTENT_L, 144, 0, 11, False, CLR_FAINT, CLR_BACK, '');
  FileText.AutoSize := False;
  FileText.Width := SX(CONTENT_W);
  FileText.Height := SY(16);
  NewImage(P, True, 'card-tip', CONTENT_L, 178, CONTENT_W, 92);
  NewLabel(P, True, CONTENT_L + 16, 192, CONTENT_W - 32, 13, True, CLR_TEXT, CLR_CARD, CustomMessage('TipTitle'));
  NewLabel(P, True, CONTENT_L + 16, 214, CONTENT_W - 32, 12, False, CLR_TEXT2, CLR_CARD, CustomMessage('TipText'));
end;

procedure CreateFinished();
var
  P: TWinControl;
  L: TLabel;
begin
  P := WizardForm.FinishedPage;
  WizardForm.WizardBitmapImage2.Visible := False;
  WizardForm.FinishedHeadingLabel.Visible := False;
  WizardForm.FinishedLabel.Visible := False;
  NewLabel(P, False, CONTENT_L, 24, CONTENT_W, 22, True, CLR_TEXT, CLR_BACK, CustomMessage('DoneTitle'));
  NewLabel(P, False, CONTENT_L, 58, CONTENT_W, 13, False, CLR_TEXT2, CLR_BACK, CustomMessage('DoneText'));
  FinishCard := NewImage(P, False, 'card-finish', CONTENT_L, 100, CONTENT_W, 112);
  FinishCardTitle := NewLabel(P, False, CONTENT_L + 16, 114, CONTENT_W - 32, 13, True, CLR_TEXT, CLR_CARD,
                              CustomMessage('DefaultTitle'));
  FinishCardText := NewLabel(P, False, CONTENT_L + 16, 136, CONTENT_W - 32, 12, False, CLR_TEXT2, CLR_CARD,
                             CustomMessage('DefaultText'));
  DefaultAppsButton := NewButton(P, False, 'accent-wide', CONTENT_L + 16, 168, 180, CustomMessage('OpenDefaultApps'),
                                 ACT_DEFAULTAPPS);
  NewToggle(T_LAUNCH, P, False, 228, CustomMessage('LaunchNow'), True);
  L := NewLabel(P, False, CONTENT_L, 334, 0, 11, False, CLR_FAINT, CLR_BACK, CustomMessage('DoneCredit'));
  NewLink(P, False, CONTENT_L, 334, 11, CLR_BACK, CustomMessage('LinkSubscribe'), @ChannelClick).Left :=
    L.Left + L.Width + SX(6);
end;

procedure InitializeWizard();
var
  Back: TArrayOfGraphic;
begin
  IsRTL := WizardForm.RightToLeft;
  Scale := PickScale();
  Log(Format('Wizard client area %dx%d, art size %d', [WizardForm.ClientWidth, WizardForm.ClientHeight, Scale]));

  SetArrayLength(Back, 1);
  if IsRTL then
    Back[0] := Png('back-rtl')
  else
    Back[0] := Png('back');
  try
    WizardSetBackImage(Back, True, False, 255);
  finally
    Back[0].Free;
  end;

  { The window is ours: no header, no Setup images or buttons in sight; the
    pages fill it, the inner ones under the title of the content }
  WizardForm.MainPanel.Visible := False;
  WizardForm.BeveledLabel.Visible := False;
  WizardForm.WizardBitmapImage.Visible := False;
  WizardForm.WelcomeLabel1.Visible := False;
  WizardForm.WelcomeLabel2.Visible := False;
  WizardForm.OuterNotebook.SetBounds(0, 0, WizardForm.ClientWidth, WizardForm.ClientHeight);
  WizardForm.InnerNotebook.SetBounds(SX(CONTENT_L), SY(BODY_T), SX(CONTENT_L + CONTENT_W) - SX(CONTENT_L),
                                     SY(BODY_T + BODY_H) - SY(BODY_T));

  CreateSidebar();
  CreateWelcome();
  CreateOptions();
  CreateOptionToggles();
  CreateInstalling();
  CreateFinished();
  InnerTitle := NewLabel(WizardForm.InnerPage, False, CONTENT_L, 24, CONTENT_W, 22, True, CLR_TEXT, CLR_BACK, '');
  InnerText := NewLabel(WizardForm.InnerPage, False, CONTENT_L, 58, CONTENT_W, 13, False, CLR_TEXT2, CLR_BACK, '');

  NavCancel := NewButton(WizardForm, False, 'button', CONTENT_L, BUTTONS_T, 100, SetupMessage(msgButtonCancel), ACT_CANCEL);
  NavBack := NewButton(WizardForm, False, 'button', CONTENT_L + CONTENT_W - 208, BUTTONS_T, 100,
                       CleanCaption(SetupMessage(msgButtonBack)), ACT_BACK);
  NavNext := NewButton(WizardForm, False, 'accent', CONTENT_L + CONTENT_W - 100, BUTTONS_T, 100,
                       CleanCaption(SetupMessage(msgButtonNext)), ACT_NEXT);
  Buttons[NavCancel].Box.BringToFront();
  Buttons[NavBack].Box.BringToFront();
  Buttons[NavNext].Box.BringToFront();

  ShowStep(0, WizardForm.WelcomePage);
  SyncButtons();
  TimerId := SetTimer(0, 0, 60, CreateCallback(@OnTimer));
end;

{ The text of a label wrapped in the width of the content (set again: an
  empty label shrinks to nothing) }
procedure SetContentText(L: TLabel; Text: String);
begin
  L.AutoSize := False;
  L.Width := SX(CONTENT_L + CONTENT_W) - SX(CONTENT_L);
  L.Caption := Text;
  L.AutoSize := True;
  { right to left: Setup mirrored it while it was empty, and the label
    shrinks to its text: its right edge at the content's }
  if IsRTL then
    L.Left := WizardForm.ClientWidth - SX(CONTENT_L) - L.Width;
end;

procedure CurPageChanged(CurPageID: Integer);
var
  Assoc: Boolean;
begin
  if CurPageID = wpWelcome then
    ShowStep(0, WizardForm.WelcomePage)
  else if CurPageID = wpFinished then
  begin
    ShowStep(3, WizardForm.FinishedPage);
    Assoc := Checked(T_VIDEO) or Checked(T_AUDIO) or Checked(T_PLAYLIST);
    FinishCard.Visible := Assoc;
    FinishCardTitle.Visible := Assoc;
    FinishCardText.Visible := Assoc;
    Buttons[DefaultAppsButton].Box.Visible := Assoc;
    { an update or a running copy reopens by itself }
    Toggles[T_LAUNCH].Box.Visible := not ShouldRelaunch();
    Toggles[T_LAUNCH].Text.Visible := not ShouldRelaunch();
  end
  else
  begin
    if CurPageID = OptionsPage.ID then
    begin
      ShowStep(1, WizardForm.InnerPage);
      SetContentText(InnerTitle, OptionsPage.Caption);
      SetContentText(InnerText, OptionsPage.Description);
      { the last page before the installation (no Ready page) }
      WizardForm.NextButton.Caption := SetupMessage(msgButtonInstall);
    end
    else if CurPageID = wpInstalling then
    begin
      ShowStep(2, WizardForm.InnerPage);
      SetContentText(InnerTitle, CustomMessage('InstallingTitle'));
      SetContentText(InnerText, CustomMessage('InstallingText'));
    end
    else
    begin
      ShowStep(2, WizardForm.InnerPage);
      SetContentText(InnerTitle, WizardForm.PageNameLabel.Caption);
      SetContentText(InnerText, WizardForm.PageDescriptionLabel.Caption);
    end;
  end;
  { the focus on Setup's own Next (out of sight): Enter goes on, and no
    field or button of ours starts selected }
  if WizardForm.NextButton.Visible and WizardForm.NextButton.Enabled then
    WizardForm.ActiveControl := WizardForm.NextButton;
  SyncButtons();
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
  Dir: String;
begin
  Result := True;
  if CurPageID = OptionsPage.ID then
  begin
    Dir := Trim(WizardForm.DirEdit.Text);
    if (Length(Dir) < 3) or ((Copy(Dir, 2, 2) <> ':\') and (Copy(Dir, 1, 2) <> '\\')) or
       (Pos('*', Dir) > 0) or (Pos('?', Dir) > 0) or (Pos('"', Dir) > 0) or (Pos('<', Dir) > 0) or
       (Pos('>', Dir) > 0) or (Pos('|', Dir) > 0) then
    begin
      MsgBox(CustomMessage('BadDir'), mbError, MB_OK);
      Result := False;
    end;
  end;
end;

procedure CurInstallProgressChanged(CurProgress, MaxProgress: Integer);
var
  W: Integer;
begin
  if MaxProgress <= 0 then
    Exit;
  { (whole numbers: "/" of two integers divides as "div" here) }
  W := Integer(Int64(Track.Width) * CurProgress div MaxProgress);
  if W < 0 then W := 0;
  if W > Track.Width then W := Track.Width;
  Fill.Width := W;
  if IsRTL then
    Fill.Left := Track.Left + Track.Width - W;
  StatusText.Caption := WizardForm.StatusLabel.Caption;
  FileText.Caption := WizardForm.FilenameLabel.Caption;
end;

procedure RegisterPreviousData(PreviousDataKey: Integer);
var
  Names: array[0..4] of String;
  I: Integer;
begin
  Names[0] := 'OpenVideo'; Names[1] := 'OpenAudio'; Names[2] := 'OpenPlaylists';
  Names[3] := 'ContextMenu'; Names[4] := 'DesktopIcon';
  for I := 0 to 4 do
    if Checked(I) then
      SetPreviousData(PreviousDataKey, Names[I], '1')
    else
      SetPreviousData(PreviousDataKey, Names[I], '0');
end;

{ ---- Fonts --------------------------------------------------------------- }

function InitializeSetup(): Boolean;
begin
  Result := True;
  try
    ExtractTemporaryFile('InterSetup-Regular.ttf');
    ExtractTemporaryFile('InterSetup-SemiBold.ttf');
    FontsLoaded :=
      (AddFontResourceEx(ExpandConstant('{tmp}\InterSetup-Regular.ttf'), FR_PRIVATE, 0) > 0) and
      (AddFontResourceEx(ExpandConstant('{tmp}\InterSetup-SemiBold.ttf'), FR_PRIVATE, 0) > 0);
  except
    FontsLoaded := False;
  end;
  if not FontsLoaded then
    Log('Inter not loaded: Segoe UI instead');
end;

procedure DeinitializeSetup();
begin
  if TimerId <> 0 then
    KillTimer(0, TimerId);
  if FontsLoaded then
  begin
    RemoveFontResourceEx(ExpandConstant('{tmp}\InterSetup-Regular.ttf'), FR_PRIVATE, 0);
    RemoveFontResourceEx(ExpandConstant('{tmp}\InterSetup-SemiBold.ttf'), FR_PRIVATE, 0);
  end;
end;

{ ---- File types ---------------------------------------------------------- }

{ The next extension of a list like '.mp4 .mkv', '' at its end }
function NextExt(var List: String): String;
var
  P: Integer;
begin
  List := TrimLeft(List);
  P := Pos(' ', List);
  if P = 0 then
  begin
    Result := List;
    List := '';
  end
  else
  begin
    Result := Copy(List, 1, P - 1);
    List := Copy(List, P + 1, Length(List));
  end;
end;

procedure DeleteKeyIfEmpty(Key: String);
begin
  RegDeleteKeyIfEmpty(HKA, Key);
end;

{ The type of an extension, "Open with" and Default apps: always }
procedure RegisterType(Ext, TypeMessage: String);
var
  ProgId, Key, Exe: String;
begin
  ProgId := PROGID_PREFIX + Ext;
  Key := 'Software\Classes\' + ProgId;
  Exe := '"' + AppExe() + '"';
  RegWriteStringValue(HKA, Key, '', FmtMessage(CustomMessage(TypeMessage), [Uppercase(Copy(Ext, 2, Length(Ext)))]));
  RegWriteStringValue(HKA, Key + '\DefaultIcon', '', Exe + ',0');
  RegWriteStringValue(HKA, Key + '\Application', 'ApplicationName', '{#AppName}');
  RegWriteStringValue(HKA, Key + '\Application', 'ApplicationIcon', Exe + ',0');
  RegWriteStringValue(HKA, Key + '\Application', 'ApplicationCompany', 'KuchiSofts');
  RegWriteStringValue(HKA, Key + '\shell', '', 'open');
  RegWriteStringValue(HKA, Key + '\shell\open', '', CustomMessage('VerbPlay'));
  RegWriteStringValue(HKA, Key + '\shell\open', 'MultiSelectModel', 'Player');
  RegWriteStringValue(HKA, Key + '\shell\open\command', '', Exe + ' --started-from-file "%1"');
  RegWriteStringValue(HKA, Key + '\shell\enqueue', '', CustomMessage('VerbEnqueue'));
  RegWriteStringValue(HKA, Key + '\shell\enqueue', 'MultiSelectModel', 'Player');
  RegWriteStringValue(HKA, Key + '\shell\enqueue\command', '', Exe + ' --started-from-file --playlist-enqueue "%1"');
  RegWriteStringValue(HKA, 'Software\Classes\' + Ext + '\OpenWithProgids', ProgId, '');
  RegWriteStringValue(HKA, CAPABILITIES + '\FileAssociations', Ext, ProgId);
end;

{ Default player of an extension where no other app was chosen (Windows
  keeps the user's choices); the previous default is kept to restore }
procedure ClaimType(Ext: String);
var
  ProgId, Key, Old: String;
begin
  ProgId := PROGID_PREFIX + Ext;
  Key := 'Software\Classes\' + Ext;
  if RegQueryStringValue(HKA, Key, '', Old) and (Old <> '') and (Old <> ProgId) then
    RegWriteStringValue(HKA, Key, BACKUP_VALUE, Old);
  RegWriteStringValue(HKA, Key, '', ProgId);
end;

{ Gives an extension back to its previous default, if it is still ours }
procedure ReleaseType(Ext: String);
var
  ProgId, Key, Cur, Old: String;
begin
  ProgId := PROGID_PREFIX + Ext;
  Key := 'Software\Classes\' + Ext;
  if RegQueryStringValue(HKA, Key, '', Cur) and (Cur = ProgId) then
  begin
    if RegQueryStringValue(HKA, Key, BACKUP_VALUE, Old) and (Old <> '') then
      RegWriteStringValue(HKA, Key, '', Old)
    else
      RegDeleteValue(HKA, Key, '');
  end;
  RegDeleteValue(HKA, Key, BACKUP_VALUE);
end;

procedure SetContextMenu(Key: String; Add: Boolean);
var
  Exe: String;
begin
  if not Add then
  begin
    RegDeleteKeyIncludingSubkeys(HKA, Key + '\VLCRedesign.Play');
    RegDeleteKeyIncludingSubkeys(HKA, Key + '\VLCRedesign.Enqueue');
    DeleteKeyIfEmpty(Key);
    Exit;
  end;
  Exe := '"' + AppExe() + '"';
  RegWriteStringValue(HKA, Key + '\VLCRedesign.Play', '', CustomMessage('ContextPlay'));
  RegWriteStringValue(HKA, Key + '\VLCRedesign.Play', 'Icon', Exe + ',0');
  RegWriteStringValue(HKA, Key + '\VLCRedesign.Play', 'MultiSelectModel', 'Player');
  RegWriteStringValue(HKA, Key + '\VLCRedesign.Play\command', '', Exe + ' --started-from-file --no-playlist-enqueue "%1"');
  RegWriteStringValue(HKA, Key + '\VLCRedesign.Enqueue', '', CustomMessage('ContextEnqueue'));
  RegWriteStringValue(HKA, Key + '\VLCRedesign.Enqueue', 'Icon', Exe + ',0');
  RegWriteStringValue(HKA, Key + '\VLCRedesign.Enqueue', 'MultiSelectModel', 'Player');
  RegWriteStringValue(HKA, Key + '\VLCRedesign.Enqueue\command', '', Exe + ' --started-from-file --playlist-enqueue "%1"');
end;

procedure ApplyTypes(List, TypeMessage: String; Claim, Context: Boolean);
var
  Ext: String;
begin
  repeat
    Ext := NextExt(List);
    if Ext <> '' then
    begin
      RegisterType(Ext, TypeMessage);
      if Claim then
        ClaimType(Ext)
      else
        ReleaseType(Ext);
      SetContextMenu('Software\Classes\SystemFileAssociations\' + Ext + '\shell', Context);
    end;
  until List = '';
end;

procedure RemoveOldOpenWith();
var
  List, Ext: String;
begin
  List := OLD_EXTS;
  repeat
    Ext := NextExt(List);
    if Ext <> '' then
      RegDeleteValue(HKA, 'Software\Classes\' + Ext + '\OpenWithProgids', 'VLCRedesign.Media');
  until List = '';
  RegDeleteKeyIncludingSubkeys(HKA, 'Software\Classes\VLCRedesign.Media');
end;

procedure RegisterFileTypes();
begin
  RemoveOldOpenWith();
  RegWriteStringValue(HKA, CAPABILITIES, 'ApplicationName', '{#AppName}');
  RegWriteStringValue(HKA, CAPABILITIES, 'ApplicationDescription', CustomMessage('ShortcutComment'));
  RegWriteStringValue(HKA, CAPABILITIES, 'ApplicationIcon', '"' + AppExe() + '",0');
  RegWriteStringValue(HKA, 'Software\RegisteredApplications', '{#AppName}', CAPABILITIES);
  ApplyTypes(VIDEO_EXTS, 'TypeVideo', Checked(T_VIDEO), Checked(T_CONTEXT));
  ApplyTypes(AUDIO_EXTS, 'TypeAudio', Checked(T_AUDIO), Checked(T_CONTEXT));
  ApplyTypes(PLAYLIST_EXTS, 'TypePlaylist', Checked(T_PLAYLIST), Checked(T_CONTEXT));
  SetContextMenu('Software\Classes\Directory\shell', Checked(T_CONTEXT));
end;

procedure UnregisterList(List: String);
var
  Ext, Key: String;
begin
  repeat
    Ext := NextExt(List);
    if Ext <> '' then
    begin
      ReleaseType(Ext);
      Key := 'Software\Classes\' + Ext;
      RegDeleteValue(HKA, Key + '\OpenWithProgids', PROGID_PREFIX + Ext);
      DeleteKeyIfEmpty(Key + '\OpenWithProgids');
      SetContextMenu('Software\Classes\SystemFileAssociations\' + Ext + '\shell', False);
      DeleteKeyIfEmpty('Software\Classes\SystemFileAssociations\' + Ext);
      DeleteKeyIfEmpty(Key);
      RegDeleteKeyIncludingSubkeys(HKA, 'Software\Classes\' + PROGID_PREFIX + Ext);
    end;
  until List = '';
end;

procedure UnregisterFileTypes();
begin
  UnregisterList(VIDEO_EXTS);
  UnregisterList(AUDIO_EXTS);
  UnregisterList(PLAYLIST_EXTS);
  SetContextMenu('Software\Classes\Directory\shell', False);
  RemoveOldOpenWith();
  RegDeleteValue(HKA, 'Software\RegisteredApplications', '{#AppName}');
  RegDeleteKeyIncludingSubkeys(HKA, 'Software\{#AppName}');
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  Code: Integer;
begin
  if CurStep = ssPostInstall then
  begin
    RegisterFileTypes();
    if not WantDesktopIcon() then
      DeleteFile(ExpandConstant('{autodesktop}\{#AppName}.lnk'));
  end
  else if (CurStep = ssDone) and (not WizardSilent) and (not ShouldRelaunch()) and Checked(T_LAUNCH) then
    ExecAsOriginalUser(AppExe(), '', '', SW_SHOWNORMAL, ewNoWait, Code);
end;

{ ---- Uninstall ----------------------------------------------------------- }

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  Pids, CmdLine: String;
  Code: Integer;
begin
  if CurUninstallStep = usUninstall then
  begin
    if FindRunning(Pids, CmdLine) then
    begin
      Exec(ExpandConstant('{sys}\taskkill.exe'), Trim(Pids), '', SW_HIDE, ewWaitUntilTerminated, Code);
      Sleep(3000);
      if FindRunning(Pids, CmdLine) then
        Exec(ExpandConstant('{sys}\taskkill.exe'), '/F ' + Trim(Pids), '', SW_HIDE, ewWaitUntilTerminated, Code);
    end;
    UnregisterFileTypes();
  end;
  if CurUninstallStep = usPostUninstall then
  begin
    { A silent uninstall keeps the user's data; otherwise ask, defaulting to keep }
    if DirExists(ExpandConstant('{app}\portable')) and (not UninstallSilent) and
       (MsgBox(CustomMessage('RemoveData'), mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES) then
      DelTree(ExpandConstant('{app}\portable'), True, True, True);
    RemoveDir(ExpandConstant('{app}'));
  end;
end;
