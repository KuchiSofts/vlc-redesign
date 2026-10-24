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
;
; Privacy: the two features that go online by themselves, the daily update
; check (GitHub) and the automatic online subtitles, are asked on the
; Privacy page, both on to begin with, with a link to CODE_SIGNING_POLICY.md.
; The answers go to {app}\portable\setup-choices.ini, which VLC Redesign puts
; into its settings at its next start (MainCtx::applySetupChoices) and then
; deletes; the first-run setup doesn't ask them again. Not the settings
; themselves: a new install has none yet, and vlc.exe fills an empty profile
; with the settings of the copy started last. An update shows the page with
; the profile's choices when they were never asked (installs from before the
; page), and skips it otherwise: they stay in Settings; it writes only what
; was changed. A silent install keeps the defaults (on), or what was chosen
; before, unless told otherwise:
;   /NOUPDATECHECK   no daily update check
;   /NOONLINESUBS    no automatic online subtitles
; (also on the page of an install that shows it: both start off).
;
; Languages: Setup speaks English and the languages VLC Redesign is fully
; translated into (the list under [Languages]), and the app starts in the
; language of Setup: {app}\portable\language holds its code, which
; bin/winvlc.c reads (no file: English, as the portable zip starts). When
; Windows is shown in one of them (GetUILanguage), Setup first asks, in it
; and in English side by side, whether to install in it; Yes starts Setup
; again in it (/LANG=, as the wizard can't change its language once it
; runs) and this one ends, No goes on in English and notes English for the
; app. A copy that has a language already keeps it and is not asked (it
; was answered, or chosen in the app's settings); Setup then runs in the
; app's language if it has it. Right to left (Hebrew) the wizard is drawn
; right to left and the Layout page offers the app's right-to-left layout,
; off to begin with as in the app, for setup-choices.ini ([interface]) as
; the Privacy page's answers. An update runs in the language of the
; install it updates (UsePreviousLanguage), and so does the uninstaller.
; A silent install never asks: the app in English (or as it was), unless
; told otherwise:
;   /LANG=hebrew        Setup in Hebrew, and the app too (a copy that has a
;                       language keeps it); the names are those of
;                       [Languages]: english, hebrew, brazilianportuguese
;   /RIGHTTOLEFT        the app's right-to-left layout (also on the Layout
;                       page of an install that shows it: it starts on)
;   /TESTUILANG=0x040D  for tests: the Windows display language Setup sees,
;                       to ask its question on any Windows

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
; English, whatever the language of Windows (as VLC Redesign starts),
; unless the question at the start chose another (see the top); an update
; in the language of the install it updates
ShowLanguageDialog=no
LanguageDetectionMethod=none
UsePreviousLanguage=yes
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

; The languages (see the top): English first, the default, then those VLC
; Redesign is fully translated into, and no other (a partial translation
; would leave the app half in English). One line each:
;   its name in Setup: /LANG=, the prefix of its [CustomMessages]
;   Inno Setup's translation of its own texts
;   its code in the app (portable\language, as its settings write it)
;   the Windows display language it is offered on: the primary language in
;     the low 10 bits, and above them the sublanguage, 0 for any
;   right to left (1): Setup draws it so (its .isl says it too) and offers
;     the app's right-to-left layout
;   Inter has its letters (1); otherwise Segoe UI, as Windows draws them
;   its name in English, for the question
; A new one also needs every one of the [CustomMessages] in it, its
; question (LanguageContext_<name> and the three after it) and its
; notice.<name>.txt, and is written into [Code] by LoadLanguages.
#define LangCount 0
#define LangI 0
#dim LangName[16]
#dim LangIsl[16]
#dim LangCode[16]
#dim LangId[16]
#dim LangRTL[16]
#dim LangInter[16]
#dim LangEnglish[16]
#define AddLanguage(str Name, str Isl, str Code, int Id, int RTL, int Inter, str English) \
  LangName[LangCount] = Name, LangIsl[LangCount] = Isl, LangCode[LangCount] = Code, \
  LangId[LangCount] = Id, LangRTL[LangCount] = RTL, LangInter[LangCount] = Inter, \
  LangEnglish[LangCount] = English, ++LangCount
#expr AddLanguage("english", "compiler:Default.isl", "en", 0x0009, 0, 1, "English")
#expr AddLanguage("hebrew", "compiler:Languages\Hebrew.isl", "he", 0x000D, 1, 0, "Hebrew")
#expr AddLanguage("brazilianportuguese", "compiler:Languages\BrazilianPortuguese.isl", "pt_BR", 0x0416, 0, 1, "Portuguese")

[Languages]
#sub EmitLanguage
Name: "{#LangName[LangI]}"; MessagesFile: "{#LangIsl[LangI]}"
#endsub
#for {LangI = 0; LangI < LangCount; LangI++} EmitLanguage

[CustomMessages]
; Every message in every language of [Languages], in the same order
english.StepWelcome=Welcome
hebrew.StepWelcome=ברוכים הבאים
brazilianportuguese.StepWelcome=Boas-vindas
english.StepOptions=Options
hebrew.StepOptions=אפשרויות
brazilianportuguese.StepOptions=Opções
english.StepPrivacy=Privacy
hebrew.StepPrivacy=פרטיות
brazilianportuguese.StepPrivacy=Privacidade
english.StepInstall=Install
hebrew.StepInstall=התקנה
brazilianportuguese.StepInstall=Instalação
english.StepDone=Done
hebrew.StepDone=סיום
brazilianportuguese.StepDone=Concluído
english.Unofficial=Unofficial build
hebrew.Unofficial=גרסה לא רשמית
brazilianportuguese.Unofficial=Não oficial
english.DesignedBy=Interface designed by
hebrew.DesignedBy=הממשק עוצב בידי
brazilianportuguese.DesignedBy=Interface criada por
english.WelcomeTitle=Welcome to {#AppName}
hebrew.WelcomeTitle=ברוכים הבאים ל-{#AppName}
brazilianportuguese.WelcomeTitle=Boas-vindas ao {#AppName}
english.WelcomeText=The real VLC 4 engine with the interface designed by Juxtopposed. Setup takes about a minute.
hebrew.WelcomeText=מנוע VLC 4 האמיתי עם הממשק שעיצבה Juxtopposed. ההתקנה נמשכת כדקה.
brazilianportuguese.WelcomeText=O verdadeiro motor do VLC 4, com a interface criada por Juxtopposed. A instalação leva cerca de um minuto.
english.CreditTitle=Interface designed by Juxtopposed
hebrew.CreditTitle=את הממשק עיצבה Juxtopposed
brazilianportuguese.CreditTitle=Interface criada por Juxtopposed
english.CreditText=Every screen comes from the "VLC Redesign (Community)" Figma file, licensed under CC BY 4.0. Juxtopposed does not endorse this project.
hebrew.CreditText=כל מסך מגיע מקובץ ה-Figma "VLC Redesign (Community)", ברישיון CC BY 4.0. Juxtopposed אינה מעורבת בפרויקט הזה ואינה ממליצה עליו.
brazilianportuguese.CreditText=Cada tela vem do arquivo do Figma "VLC Redesign (Community)", sob a licença CC BY 4.0. Juxtopposed não endossa este projeto.
english.LinkVideo=Watch the video
hebrew.LinkVideo=לצפייה בסרטון
brazilianportuguese.LinkVideo=Assistir ao vídeo
english.LinkChannel=YouTube channel
hebrew.LinkChannel=ערוץ היוטיוב
brazilianportuguese.LinkChannel=Canal no YouTube
english.LinkFigma=Figma file
hebrew.LinkFigma=קובץ ה-Figma
brazilianportuguese.LinkFigma=Arquivo do Figma
english.WelcomeNotice=An unofficial build, not made or endorsed by VideoLAN. VLC and the cone logo are trademarks of VideoLAN. It installs next to an official VLC without touching it.
hebrew.WelcomeNotice=גרסה לא רשמית, שאינה של VideoLAN ואינה מאושרת על ידה. VLC ולוגו הקונוס הם סימנים מסחריים של VideoLAN. היא מותקנת לצד VLC רשמי בלי לגעת בו.
brazilianportuguese.WelcomeNotice=Uma versão não oficial, que não é feita nem endossada pela VideoLAN. VLC e o logotipo do cone são marcas da VideoLAN. Ela é instalada ao lado de um VLC oficial, sem mexer nele.
english.LinkNotice=Notice and licenses
hebrew.LinkNotice=הודעה ורישיונות
brazilianportuguese.LinkNotice=Aviso e licenças
english.LinkSource=Source code
hebrew.LinkSource=קוד מקור
brazilianportuguese.LinkSource=Código-fonte
english.VersionLine=Version %1 · free software under the GNU GPL v2 or later
hebrew.VersionLine=גרסה %1 · תוכנה חופשית ברישיון GNU GPL גרסה 2 ומעלה
brazilianportuguese.VersionLine=Versão %1 · software livre sob a GNU GPL v2 ou posterior
english.OptionsTitle=Options
hebrew.OptionsTitle=אפשרויות
brazilianportuguese.OptionsTitle=Opções
english.OptionsText=Where it goes, and what it opens.
hebrew.OptionsText=לאן להתקין, ומה לפתוח איתה.
brazilianportuguese.OptionsText=Onde instalar e o que abrir com ele.
english.InstallTo=Install to
hebrew.InstallTo=התקנה אל
brazilianportuguese.InstallTo=Instalar em
english.Browse=Browse
hebrew.Browse=עיון
brazilianportuguese.Browse=Procurar
english.ForYou=For you only, without administrator rights.
hebrew.ForYou=עבורך בלבד, בלי הרשאות מנהל.
brazilianportuguese.ForYou=Só para você, sem direitos de administrador.
english.ForEveryone=Install for everyone
hebrew.ForEveryone=התקנה לכל המשתמשים
brazilianportuguese.ForEveryone=Instalar para todos
english.ForAll=For everyone on this PC.
hebrew.ForAll=לכל המשתמשים במחשב.
brazilianportuguese.ForAll=Para todos neste computador.
english.UpdatingHere=Updates the copy in this folder.
hebrew.UpdatingHere=מעדכן את העותק שבתיקייה הזו.
brazilianportuguese.UpdatingHere=Atualiza a cópia que está nesta pasta.
english.OpenVideo=Open videos with {#AppName}
hebrew.OpenVideo=פתיחת סרטונים ב-{#AppName}
brazilianportuguese.OpenVideo=Abrir vídeos com o {#AppName}
english.OpenAudio=Open music with {#AppName}
hebrew.OpenAudio=פתיחת מוזיקה ב-{#AppName}
brazilianportuguese.OpenAudio=Abrir músicas com o {#AppName}
english.OpenPlaylists=Open playlists with {#AppName}
hebrew.OpenPlaylists=פתיחת רשימות השמעה ב-{#AppName}
brazilianportuguese.OpenPlaylists=Abrir playlists com o {#AppName}
english.ContextMenu=Play with {#AppName} in right-click menus
hebrew.ContextMenu="הפעלה ב-{#AppName}" בתפריט הקליק הימני
brazilianportuguese.ContextMenu={#AppName} no menu do clique direito
english.DesktopIcon=Desktop shortcut
hebrew.DesktopIcon=קיצור דרך בשולחן העבודה
brazilianportuguese.DesktopIcon=Atalho na área de trabalho
english.LayoutTitle=Layout
hebrew.LayoutTitle=פריסה
brazilianportuguese.LayoutTitle=Layout
english.LayoutText=Left to right as designed, or mirrored.
hebrew.LayoutText=משמאל לימין כמו בעיצוב, או בתמונת ראי.
brazilianportuguese.LayoutText=Da esquerda para a direita, como no design, ou espelhado.
english.LayoutRTL=Right-to-left layout
hebrew.LayoutRTL=פריסה מימין לשמאל
brazilianportuguese.LayoutRTL=Layout da direita para a esquerda
english.LayoutRTLText=The sidebar on the right and the controls in reverse order, for reading from right to left. Off: the design's layout, from left to right.
hebrew.LayoutRTLText=סרגל הצד מימין והפקדים בסדר הפוך, לקריאה מימין לשמאל. כבוי: הפריסה של העיצוב, משמאל לימין.
brazilianportuguese.LayoutRTLText=A barra lateral à direita e os controles em ordem inversa, para ler da direita para a esquerda. Desligado: o layout do design, da esquerda para a direita.
english.LayoutCardTitle=Change it any time
hebrew.LayoutCardTitle=אפשר לשנות בכל עת
brazilianportuguese.LayoutCardTitle=Dá para mudar quando quiser
english.LayoutCardText=Settings → Appearance: Layout direction. The language of the app is there too, in Interface language.
hebrew.LayoutCardText=בהגדרות ← מראה: כיוון הפריסה. גם שפת האפליקציה נמצאת שם, בשפת הממשק.
brazilianportuguese.LayoutCardText=Configurações → Aparência: direção do layout. O idioma do app também fica lá, em idioma da interface.
english.PrivacyTitle=Privacy
hebrew.PrivacyTitle=פרטיות
brazilianportuguese.PrivacyTitle=Privacidade
english.PrivacyText=Two features go online by themselves. Each sends only this:
hebrew.PrivacyText=שתי תכונות מתחברות לאינטרנט מעצמן. כל אחת שולחת רק את זה:
brazilianportuguese.PrivacyText=Dois recursos acessam a internet por conta própria. Cada um envia só isto:
english.PrivacyUpdates=Check for updates once a day
hebrew.PrivacyUpdates=בדיקת עדכונים פעם ביום
brazilianportuguese.PrivacyUpdates=Verificar atualizações uma vez por dia
english.PrivacyUpdatesText=A request for the latest release, to GitHub (api.github.com).
hebrew.PrivacyUpdatesText=בקשה לגרסה האחרונה, אל GitHub (api.github.com).
brazilianportuguese.PrivacyUpdatesText=Um pedido da versão mais recente, ao GitHub (api.github.com).
english.PrivacySubtitles=Find subtitles online automatically
hebrew.PrivacySubtitles=חיפוש כתוביות ברשת באופן אוטומטי
brazilianportuguese.PrivacySubtitles=Buscar legendas on-line automaticamente
english.PrivacySubtitlesText=The film's name and a fingerprint of the file, to subtitle services (OpenSubtitles.org, Wizdom, Gestdown, YIFY); its name to IMDb, Wikidata and Cinemeta.
hebrew.PrivacySubtitlesText=שם הסרט וטביעת אצבע של הקובץ, לשירותי הכתוביות OpenSubtitles.org, Wizdom, Gestdown ו-YIFY. השם גם ל-IMDb, ל-Wikidata ול-Cinemeta.
brazilianportuguese.PrivacySubtitlesText=O nome do filme e uma impressão digital do arquivo, para serviços de legendas (OpenSubtitles.org, Wizdom, Gestdown, YIFY); o nome também para IMDb, Wikidata e Cinemeta.
english.PrivacyCardTitle=No telemetry, no ads, no account
hebrew.PrivacyCardTitle=בלי טלמטריה, בלי פרסומות, בלי חשבון
brazilianportuguese.PrivacyCardTitle=Sem telemetria, sem anúncios, sem conta
english.PrivacyCardText=Nothing else goes online unless you ask for it. Change both later in Settings.
hebrew.PrivacyCardText=שום דבר אחר לא יוצא לאינטרנט בלי שתבקשו. אפשר לשנות את שתיהן אחר כך בהגדרות.
brazilianportuguese.PrivacyCardText=Nada mais acessa a internet sem você pedir. Altere os dois depois nas Configurações.
english.LinkPrivacy=Privacy and code signing policy
hebrew.LinkPrivacy=מדיניות הפרטיות וחתימת הקוד
brazilianportuguese.LinkPrivacy=Política de privacidade e de assinatura de código
english.InstallingTitle=Installing
hebrew.InstallingTitle=מתקין
brazilianportuguese.InstallingTitle=Instalando
english.InstallingText=This takes about a minute.
hebrew.InstallingText=זה לוקח כדקה.
brazilianportuguese.InstallingText=Isso leva cerca de um minuto.
english.TipTitle=Make the player yours
hebrew.TipTitle=התאימו את הנגן לעצמכם
brazilianportuguese.TipTitle=Deixe o player do seu jeito
english.TipText=Drag buttons into and out of the control bar with ⋯ → Customize Player, and keep your layouts as profiles.
hebrew.TipText=גררו כפתורים אל סרגל השליטה וממנו דרך ⋯ ← התאמת הנגן, ושמרו את הפריסות שלכם כפרופילים.
brazilianportuguese.TipText=Arraste botões para dentro e para fora da barra de controle em ⋯ → Personalizar o player, e guarde seus layouts como perfis.
english.DoneTitle={#AppName} is ready
hebrew.DoneTitle={#AppName} מוכן
brazilianportuguese.DoneTitle=O {#AppName} está pronto
english.DoneText=Find it in the Start menu.
hebrew.DoneText=אפשר למצוא אותו בתפריט התחל.
brazilianportuguese.DoneText=Ele está no menu Iniciar.
english.DefaultTitle=Make it your default player
hebrew.DefaultTitle=הגדרה כנגן ברירת המחדל
brazilianportuguese.DefaultTitle=Torne-o seu player padrão
english.DefaultText=Windows keeps the apps you chose before. Open Default apps and choose {#AppName} for your media.
hebrew.DefaultText=Windows שומרת את האפליקציות שבחרתם בעבר. פתחו את "אפליקציות ברירת מחדל" ובחרו ב-{#AppName} עבור קובצי המדיה.
brazilianportuguese.DefaultText=O Windows mantém suas escolhas. Abra Aplicativos padrão e escolha o {#AppName} para suas mídias.
english.OpenDefaultApps=Open Default apps
hebrew.OpenDefaultApps=לאפליקציות ברירת מחדל
brazilianportuguese.OpenDefaultApps=Aplicativos padrão
english.LaunchNow=Open {#AppName} now
hebrew.LaunchNow=פתיחת {#AppName} עכשיו
brazilianportuguese.LaunchNow=Abrir o {#AppName} agora
english.DoneCredit=Enjoy the design by Juxtopposed.
hebrew.DoneCredit=תיהנו מהעיצוב של Juxtopposed.
brazilianportuguese.DoneCredit=Aproveite o design de Juxtopposed.
english.LinkSubscribe=Subscribe on YouTube
hebrew.LinkSubscribe=הרשמה לערוץ ביוטיוב
brazilianportuguese.LinkSubscribe=Inscreva-se no YouTube
english.NoticeTitle=Notice and licenses
hebrew.NoticeTitle=הודעה ורישיונות
brazilianportuguese.NoticeTitle=Aviso e licenças
english.Close=Close
hebrew.Close=סגירה
brazilianportuguese.Close=Fechar
english.BadDir=Choose a folder to install to.
hebrew.BadDir=בחרו תיקייה להתקנה.
brazilianportuguese.BadDir=Escolha uma pasta para a instalação.
; The words on the toggles, as the app writes them (SettingsToggle.qml)
english.ToggleOn=ON
hebrew.ToggleOn=פעיל
brazilianportuguese.ToggleOn=LIG
english.ToggleOff=OFF
hebrew.ToggleOff=כבוי
brazilianportuguese.ToggleOff=DESL
; Shown by Windows: the file types, their verbs, the right-click menus
english.TypeVideo=%1 video
hebrew.TypeVideo=וידאו %1
brazilianportuguese.TypeVideo=Vídeo %1
english.TypeAudio=%1 audio
hebrew.TypeAudio=שמע %1
brazilianportuguese.TypeAudio=Áudio %1
english.TypePlaylist=%1 playlist
hebrew.TypePlaylist=רשימת השמעה %1
brazilianportuguese.TypePlaylist=Playlist %1
english.VerbPlay=Play
hebrew.VerbPlay=הפעלה
brazilianportuguese.VerbPlay=Reproduzir
english.VerbEnqueue=Add to queue
hebrew.VerbEnqueue=הוספה לתור
brazilianportuguese.VerbEnqueue=Adicionar à fila
english.ContextPlay=Play with {#AppName}
hebrew.ContextPlay=הפעלה ב-{#AppName}
brazilianportuguese.ContextPlay=Reproduzir com o {#AppName}
english.ContextEnqueue=Add to {#AppName} queue
hebrew.ContextEnqueue=הוספה לתור של {#AppName}
brazilianportuguese.ContextEnqueue=Adicionar à fila do {#AppName}
english.Running={#AppName} is open. Setup will close it now and open it again when it's done.
hebrew.Running={#AppName} פתוח. ההתקנה תסגור אותו עכשיו ותפתח אותו מחדש בסיום.
brazilianportuguese.Running=O {#AppName} está aberto. O instalador vai fechá-lo agora e abri-lo de novo quando terminar.
english.StillRunning={#AppName} didn't close. Close it and click Retry.
hebrew.StillRunning={#AppName} לא נסגר. סגרו אותו ולחצו על "נסה שוב".
brazilianportuguese.StillRunning=O {#AppName} não fechou. Feche-o e clique em "Tentar novamente".
english.ShortcutComment=VLC media player with the interface designed by Juxtopposed
hebrew.ShortcutComment=נגן המדיה VLC עם הממשק שעיצבה Juxtopposed
brazilianportuguese.ShortcutComment=Reprodutor de Mídias VLC com a interface criada por Juxtopposed
; The question at the start (see the top), asked before the wizard in
; English and in the language of Windows, whatever the language Setup runs
; in: so in every language, without a prefix. One line of each, for the
; question's two columns (not wrapped: a right-to-left one is drawn right
; to left line by line).
LanguageContext=Windows is in %1.
LanguageQuestion=Install {#AppName} in %1?
LanguageYes=Yes
LanguageNo=No
LanguageContext_hebrew=ה-Windows שלך בעברית.
LanguageQuestion_hebrew=להתקין את {#AppName} בעברית?
LanguageYes_hebrew=כן
LanguageNo_hebrew=לא
LanguageContext_brazilianportuguese=Seu Windows está em português.
LanguageQuestion_brazilianportuguese=Instalar o {#AppName} em português?
LanguageYes_brazilianportuguese=Sim
LanguageNo_brazilianportuguese=Não

[Files]
; The wizard's pieces and fonts: read by the [Code], never installed
Source: "art\ui\*.png"; Flags: dontcopy noencryption
Source: "art\fonts\*.ttf"; Flags: dontcopy noencryption
; (notice.txt in English, notice.<language>.txt in the others)
Source: "notice*.txt"; Flags: dontcopy noencryption
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
  T_UPDATES = 6;
  T_SUBTITLES = 7;
  T_RTL = 8;

  { The answers of the Privacy and Layout pages, for the app's next start
    (see the top) }
  CHOICES_FILE = 'setup-choices.ini';
  { The language of the app, read by bin/winvlc.c (see the top) }
  LANGUAGE_FILE = 'language';
  UNINSTALL_KEY = 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{{#AppIdGuid}}_is1';
  { A right-to-left run inside a left-to-right line: the question is asked
    in English, before the language of Setup is set }
  RTL_EMBED = #$202B;
  RTL_POP = #$202C;

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
  { Where each copy notes its profile, for the settings of the next copy }
  PROFILES = 'Software\{#AppName}\Profiles';

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

  { An ON / OFF toggle of the settings (SettingsToggle.qml), its label, and
    the word on it in the language of Setup }
  TDesignToggle = record
    Box: TBitmapButton;
    Text: TLabel;
    Word: TLabel;
    Checked: Boolean;
  end;

  { A language of Setup (see [Languages]) }
  TLanguage = record
    Name, Code, English: String;
    Id: Integer;
    RTL, Inter: Boolean;
  end;

var
  Scale: Integer;
  FontsLoaded: Boolean;
  IsRTL: Boolean;
  Languages: array of TLanguage;
  { The language of Setup in Languages, and the one the app gets: its code
    for portable\language, '' to leave that as it is (see the top) }
  Lang: Integer;
  AppLanguage: String;
  { The width of the toggles' label, 33 in the design (see make-art.py) }
  ToggleLabel: Integer;
  TimerId: Longword;
  WasRunning: Boolean;
  RunningArgs: String;
  Relaunching: Boolean;

  OptionsPage: TWizardPage;
  LayoutPage: TWizardPage;
  PrivacyPage: TWizardPage;
  Buttons: array of TDesignButton;
  Toggles: array of TDesignToggle;
  NavCancel, NavBack, NavNext: Integer;

  { The profile the Privacy page shows, and its choices then }
  ChoicesReadFor: String;
  ProfileFresh, InitialUpdates, InitialSubtitles, PrivacyShown: Boolean;
  LayoutShown: Boolean;

  SideMark, Pill: TBitmapImage;
  SideTitle, SideSub, SideBy, SideName: TLabel;
  StepNum, StepText: array[0..4] of TLabel;
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
function ShellExecute(Wnd: HWND; Operation, FileName, Parameters, Directory: String; ShowCmd: Integer): Integer;
  external 'ShellExecuteW@shell32.dll stdcall';

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

{ Inter, or Segoe UI for a language whose letters it doesn't have (Hebrew:
  see [Languages]), as Windows draws them }
function FaceOf(Inter, Bold: Boolean): String;
begin
  if FontsLoaded and Inter then
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

function Face(Bold: Boolean): String;
begin
  Result := FaceOf(Languages[Lang].Inter, Bold);
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
    { a new caption leaves the box white until its picture is set again }
    PaintButton(I);
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

{ The label of the toggles holds ON and OFF in the language of Setup: 33
  wide in the design, wider for longer words, as the app does
  (SettingsToggle.qml), in the widths of the art (make-art.py) }
procedure MeasureToggleWords();
var
  L: TLabel;
  W: Integer;
  F: Extended;
begin
  L := TLabel.Create(WizardForm);
  L.Parent := WizardForm;
  L.Visible := False;
  StyleText(L, 12, False, CLR_FAINT, CLR_BACK);
  L.Caption := CustomMessage('ToggleOn');
  W := L.Width;
  L.Caption := CustomMessage('ToggleOff');
  if L.Width > W then
    W := L.Width;
  L.Free;
  { (in the units of the 100 % window) }
  F := W;
  W := Round(F * BASE_W / WizardForm.ClientWidth);
  if W + 8 <= 33 then
    ToggleLabel := 33
  else if W + 8 <= 39 then
    ToggleLabel := 39
  else
    ToggleLabel := 45;
end;

function ToggleWidth(): Integer;
begin
  Result := ToggleLabel + 35;
end;

{ (at the right of the content) }
function ToggleLeft(): Integer;
begin
  Result := CONTENT_L + CONTENT_W - ToggleWidth();
end;

function ToggleOf(Sender: TObject): Integer;
var
  I: Integer;
begin
  Result := -1;
  for I := 0 to GetArrayLength(Toggles) - 1 do
    if (Sender = Toggles[I].Box) or (Sender = Toggles[I].Text) or (Sender = Toggles[I].Word) then
      Result := I;
end;

{ The picture of the state (mirrored right to left), and its word where
  the design puts it: ON at 7 in its label, OFF at 4, a longer word in the
  middle }
procedure PaintToggle(I: Integer);
var
  Art, Caption: String;
  LabelX, TextX, X: Integer;
  W: TLabel;
begin
  W := Toggles[I].Word;
  if Toggles[I].Checked then
  begin
    Art := 'toggle-on-';
    Caption := CustomMessage('ToggleOn');
    LabelX := 3;
    TextX := 7;
    W.Font.Color := CLR_ACCENT;
    W.Color := CLR_ACCENT_BACK;
  end
  else
  begin
    Art := 'toggle-off-';
    Caption := CustomMessage('ToggleOff');
    LabelX := 32;
    TextX := 4;
    W.Font.Color := CLR_FAINT;
    W.Color := CLR_CARD;
  end;
  Art := Art + IntToStr(ToggleLabel);
  if IsRTL then
    Art := Art + '-rtl';
  SetBoxImage(Toggles[I].Box, Art);
  W.Caption := Caption;
  if SX(TextX) + W.Width <= SX(ToggleLabel - 3) then
    X := SX(LabelX + TextX)
  else
    X := SX(LabelX) + (SX(ToggleLabel) - W.Width) div 2;
  { right to left, mirrored once Setup has mirrored the window (it mirrors
    what is there then) }
  if IsRTL and WizardForm.ControlsFlipped then
    X := Toggles[I].Box.ClientWidth - X - W.Width;
  W.Left := X;
  W.Top := (Toggles[I].Box.ClientHeight - W.Height) div 2;
  Toggles[I].Box.Caption := Toggles[I].Text.Caption + ': ' + Caption;
end;

{ Right to left, when a page shows: Setup has mirrored the window by then }
procedure RepaintToggles();
var
  I: Integer;
begin
  for I := 0 to GetArrayLength(Toggles) - 1 do
    if Toggles[I].Box <> nil then
      PaintToggle(I);
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
  Place(Toggles[Index].Box, Body, ToggleLeft(), T + 2, ToggleWidth(), 32);
  Toggles[Index].Word := TLabel.Create(WizardForm);
  Toggles[Index].Word.Parent := Toggles[Index].Box;
  StyleText(Toggles[Index].Word, 12, False, CLR_FAINT, CLR_CARD);
  Toggles[Index].Word.Cursor := crHand;
  Toggles[Index].Word.OnClick := @ToggleClick;
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

{ ---- Privacy ------------------------------------------------------------- }

{ A switch of the command line, /NAME or -NAME }
function HasSwitch(Name: String): Boolean;
var
  I: Integer;
begin
  Result := False;
  for I := 1 to ParamCount do
    if (CompareText(ParamStr(I), '/' + Name) = 0) or (CompareText(ParamStr(I), '-' + Name) = 0) then
      Result := True;
end;

function BoolText(B: Boolean): String;
begin
  if B then
    Result := 'true'
  else
    Result := 'false';
end;

{ The profile of the copy installed: its settings, in portable\ next to
  vlc.exe (see [Dirs]) }
function ProfileFile(Name: String): String;
begin
  Result := AddBackslash(WizardDirValue) + 'portable\' + Name;
end;

{ A profile that vlc.exe leaves as it is: with settings, or a library
  (bin/winvlc.c fills one without them with the settings of the last copy) }
function ProfileHasSettings(): Boolean;
begin
  Result := FileExists(ProfileFile('vlcrc')) or FileExists(ProfileFile('vlc\vlc-qt-interface.ini')) or
            FileExists(ProfileFile('ml\ml.db'));
end;

{ An option of vlcrc as it is written there, '' for its default (VLC writes
  those as comments) }
function VlcrcValue(Name: String): String;
var
  Lines: TArrayOfString;
  I: Integer;
  L: String;
begin
  Result := '';
  if not LoadStringsFromFile(ProfileFile('vlcrc'), Lines) then
    Exit;
  for I := 0 to GetArrayLength(Lines) - 1 do
  begin
    L := Trim(Lines[I]);
    if CompareText(Copy(L, 1, Length(Name) + 1), Name + '=') = 0 then
      Result := Trim(Copy(L, Length(Name) + 2, Length(L)));
  end;
end;

{ A setting of the interface (QSettings: "true" or "false"), '' if unset }
function InterfaceValue(Section, Key: String): String;
begin
  Result := GetIniString(Section, Key, '', ProfileFile('vlc\vlc-qt-interface.ini'));
end;

{ The profile was asked already: by this page (its answers wait for the
  app's next start, or are in the settings), or by the app's first-run setup }
function ChoicesMade(): Boolean;
begin
  Result := FileExists(ProfileFile(CHOICES_FILE)) or
            (CompareText(InterfaceValue('redesign', 'privacy-asked'), 'true') = 0);
end;

{ The page shows the choices of the profile it installs to (the defaults for
  a new one); the switches of the command line turn them off }
procedure LoadChoices();
begin
  if ChoicesReadFor = ProfileFile('') then
    Exit;
  ChoicesReadFor := ProfileFile('');
  ProfileFresh := not ProfileHasSettings();
  InitialUpdates := VlcrcValue('redesign-update-check') <> '0';
  InitialSubtitles := CompareText(InterfaceValue('redesign', 'subtitle-auto-search'), 'false') <> 0;
  Toggles[T_UPDATES].Checked := InitialUpdates and not HasSwitch('NOUPDATECHECK');
  Toggles[T_SUBTITLES].Checked := InitialSubtitles and not HasSwitch('NOONLINESUBS');
  PaintToggle(T_UPDATES);
  PaintToggle(T_SUBTITLES);
  Log('Privacy: profile ' + ChoicesReadFor + ', new ' + BoolText(ProfileFresh) + ', update check ' +
      BoolText(InitialUpdates) + ', online subtitles ' + BoolText(InitialSubtitles));
end;

{ The layout was chosen: in the app's settings, or on the Layout page for
  the app's next start }
function LayoutChosen(): Boolean;
begin
  Result := (InterfaceValue('redesign', 'right-to-left') <> '') or
            (GetIniString('interface', 'right-to-left', '', ProfileFile(CHOICES_FILE)) <> '');
end;

{ After the files are in place: the answers, for the app's next start (see
  the top). Those of the page where it was shown: all of them for a new
  profile, what was changed for one with settings. Otherwise (a silent
  install, or the page skipped as asked before) what the command line says;
  a new profile is noted as asked, with the defaults. The layout: the
  answer of its page where it was shown (never asked before), otherwise
  /RIGHTTOLEFT. }
procedure SaveChoices();
var
  Updates, Subtitles, RightToLeft, WriteUpdates, WriteSubtitles, WriteLayout: Boolean;
  FileName: String;
begin
  if LayoutShown and not WizardSilent then
  begin
    RightToLeft := Checked(T_RTL);
    WriteLayout := True;
  end
  else
  begin
    RightToLeft := HasSwitch('RIGHTTOLEFT');
    WriteLayout := RightToLeft;
  end;
  if PrivacyShown and not WizardSilent then
  begin
    Updates := Checked(T_UPDATES);
    Subtitles := Checked(T_SUBTITLES);
    WriteUpdates := ProfileFresh or (Updates <> InitialUpdates);
    WriteSubtitles := ProfileFresh or (Subtitles <> InitialSubtitles);
  end
  else
  begin
    Updates := not HasSwitch('NOUPDATECHECK');
    Subtitles := not HasSwitch('NOONLINESUBS');
    WriteUpdates := not Updates;
    WriteSubtitles := not Subtitles;
    if (not WriteUpdates) and (not WriteSubtitles) and (not WriteLayout) and
       (ChoicesMade() or ProfileHasSettings()) then
    begin
      Log('Privacy: nothing to change, the profile keeps its choices');
      Exit;
    end;
  end;
  FileName := ProfileFile(CHOICES_FILE);
  if not FileExists(FileName) then
    SaveStringToFile(FileName, '; The answers of {#AppName} Setup (its Privacy and Layout pages). {#AppName}' + #13#10 +
                     '; puts them into its settings at its next start, then deletes this file.' + #13#10 +
                     '[privacy]' + #13#10, False);
  if WriteUpdates then
    SetIniString('privacy', 'update-check', BoolText(Updates), FileName);
  if WriteSubtitles then
    SetIniString('privacy', 'subtitle-search', BoolText(Subtitles), FileName);
  if WriteLayout then
    SetIniString('interface', 'right-to-left', BoolText(RightToLeft), FileName);
  Log('Privacy: ' + FileName + ' written: update check ' + BoolText(Updates) + ' (' + BoolText(WriteUpdates) +
      '), online subtitles ' + BoolText(Subtitles) + ' (' + BoolText(WriteSubtitles) + '), right to left ' +
      BoolText(RightToLeft) + ' (' + BoolText(WriteLayout) + ')');
end;

procedure PrivacyPolicyClick(Sender: TObject);
begin
  OpenUrl('{#RepoUrl}/blob/main/CODE_SIGNING_POLICY.md');
end;

{ ---- The language -------------------------------------------------------- }

{ The languages of Setup, from the list above [Languages] }
procedure LoadLanguages();
begin
  SetArrayLength(Languages, {#LangCount});
#sub EmitLanguageCode
  with Languages[{#LangI}] do
  begin
    Name := '{#LangName[LangI]}';
    Code := '{#LangCode[LangI]}';
    Id := {#LangId[LangI]};
    RTL := {#LangRTL[LangI] ? "True" : "False"};
    Inter := {#LangInter[LangI] ? "True" : "False"};
    English := '{#LangEnglish[LangI]}';
  end;
#endsub
#for {LangI = 0; LangI < LangCount; LangI++} EmitLanguageCode
end;

{ A language of Setup by its name in Setup, or by its code in the app; -1
  for one it hasn't }
function LanguageOf(Name: String): Integer;
var
  I: Integer;
begin
  Result := -1;
  for I := 0 to GetArrayLength(Languages) - 1 do
    if CompareText(Languages[I].Name, Name) = 0 then
      Result := I;
end;

function LanguageOfCode(Code: String): Integer;
var
  I: Integer;
begin
  Result := -1;
  for I := 0 to GetArrayLength(Languages) - 1 do
    if CompareText(Languages[I].Code, Code) = 0 then
      Result := I;
end;

{ The display language of Windows, or the one /TESTUILANG= pretends }
function UILanguage(): Integer;
var
  S: String;
begin
  Result := GetUILanguage;
  S := ExpandConstant('{param:TESTUILANG|}');
  if S <> '' then
  begin
    if CompareText(Copy(S, 1, 2), '0x') = 0 then
      S := '$' + Copy(S, 3, Length(S));
    Result := StrToIntDef(S, Result);
  end;
end;

{ The language of Setup that Windows is shown in: the same primary
  language, and the same sublanguage where the list gives one; -1 for
  English (the first) and for a language Setup hasn't }
function OfferedLanguage(): Integer;
var
  UI, I: Integer;
begin
  Result := -1;
  UI := UILanguage();
  for I := 1 to GetArrayLength(Languages) - 1 do
    if (Result < 0) and ((Languages[I].Id and $3FF) = (UI and $3FF)) and
       (((Languages[I].Id shr 10) = 0) or (Languages[I].Id = UI)) then
      Result := I;
end;

{ The language of the app in a folder (portable\language), '' if none }
function LanguageIn(Dir: String): String;
var
  S: AnsiString;
begin
  Result := '';
  if (Dir <> '') and LoadStringFromFile(AddBackslash(Dir) + 'portable\' + LANGUAGE_FILE, S) then
    Result := Trim(S);
end;

{ The language of the copy Setup installs over: the one installed (its
  folder in the registry), or one left where a new install goes }
function ExistingLanguage(): String;
var
  Dir: String;
begin
  if RegQueryStringValue(HKA, UNINSTALL_KEY, 'Inno Setup: App Path', Dir) and (Dir <> '') then
    Result := LanguageIn(Dir)
  else
    Result := LanguageIn(ExpandConstant('{autopf}\{#AppName}'));
end;

{ A line of the question, in the redesign's colours }
function QuestionLabel(Form: TSetupForm; Text: String; Px: Integer; Bold, Inter: Boolean;
  Color: TColor): TLabel;
begin
  Result := TLabel.Create(Form);
  Result.Parent := Form;
  Result.StyleElements := [];
  Result.Font.Name := FaceOf(Inter, Bold);
  Result.Font.Height := -ScaleY(Px);
  Result.Font.Color := Color;
  Result.Color := CLR_BACK;
  Result.Caption := Text;
end;

{ The question (see the top), before the wizard and so in Setup's English:
  two columns, English and Windows' language, each a line of context and
  the question, and Yes and No in both. A right-to-left language is drawn
  right to left line by line (embedded in the form's left to right), in the
  column at the right; a left-to-right one comes first. True for Yes. }
function AskLanguage(L: Integer): Boolean;
var
  Form: TSetupForm;
  Texts: array[0..3] of String;
  Lines: array[0..3] of TLabel;
  Widths: array[0..3] of Integer;
  YesText, NoText: String;
  Yes, No: TNewButton;
  I, Margin, Gap, English, Native, ButtonW, ButtonH, Top2, ButtonsT, W, H: Integer;
begin
  Texts[0] := FmtMessage(CustomMessage('LanguageContext'), [Languages[L].English]);
  Texts[1] := FmtMessage(CustomMessage('LanguageQuestion'), [Languages[L].English]);
  Texts[2] := CustomMessage('LanguageContext_' + Languages[L].Name);
  Texts[3] := CustomMessage('LanguageQuestion_' + Languages[L].Name);
  if Languages[L].RTL then
  begin
    Texts[2] := RTL_EMBED + Texts[2] + RTL_POP;
    Texts[3] := RTL_EMBED + Texts[3] + RTL_POP;
    YesText := CustomMessage('LanguageYes') + ' · ' + CustomMessage('LanguageYes_' + Languages[L].Name);
    NoText := CustomMessage('LanguageNo') + ' · ' + CustomMessage('LanguageNo_' + Languages[L].Name);
  end
  else
  begin
    YesText := CustomMessage('LanguageYes_' + Languages[L].Name) + ' · ' + CustomMessage('LanguageYes');
    NoText := CustomMessage('LanguageNo_' + Languages[L].Name) + ' · ' + CustomMessage('LanguageNo');
  end;
  Log('Language: Windows is in ' + Languages[L].English + ', asking: ' + Texts[1]);

  { The size of the texts first: a form's size is set when it is made }
  Form := CreateCustomForm(ScaleX(100), ScaleY(100), False, False);
  try
    for I := 0 to 3 do
    begin
      Lines[I] := QuestionLabel(Form, Texts[I], 13 + 2 * (I mod 2), (I mod 2) = 1,
                                (I < 2) or Languages[L].Inter, CLR_TEXT);
      Widths[I] := Lines[I].Width;
    end;
    ButtonW := Form.CalculateButtonWidth([YesText, NoText]);
  finally
    Form.Free();
  end;
  Margin := ScaleX(24);
  Gap := ScaleX(40);
  English := Widths[0];
  if Widths[1] > English then English := Widths[1];
  Native := Widths[2];
  if Widths[3] > Native then Native := Widths[3];
  if ButtonW < ScaleX(100) then ButtonW := ScaleX(100);
  ButtonH := ScaleY(28);
  Top2 := ScaleY(46);
  ButtonsT := Top2 + ScaleY(48);
  W := Margin + English + Gap + Native + Margin;
  if W < Margin + 2 * ButtonW + ScaleX(8) + Margin then
    W := Margin + 2 * ButtonW + ScaleX(8) + Margin;
  H := ButtonsT + ButtonH + ScaleY(20);

  Form := CreateCustomForm(W, H, False, False);
  try
    Form.Caption := '{#AppName}';
    for I := 0 to 3 do
    begin
      if (I mod 2) = 0 then
        Lines[I] := QuestionLabel(Form, Texts[I], 13, False, (I < 2) or Languages[L].Inter, CLR_TEXT2)
      else
        Lines[I] := QuestionLabel(Form, Texts[I], 15, True, (I < 2) or Languages[L].Inter, CLR_TEXT);
      if (I mod 2) = 0 then
        Lines[I].Top := ScaleY(22)
      else
        Lines[I].Top := Top2;
      if I < 2 then
      begin
        if Languages[L].RTL then
          Lines[I].Left := Margin
        else
          Lines[I].Left := Margin + Native + Gap;
      end
      else if Languages[L].RTL then
      begin
        Lines[I].AutoSize := False;
        Lines[I].Alignment := taRightJustify;
        Lines[I].SetBounds(W - Margin - Native, Lines[I].Top, Native, Lines[I].Height);
      end
      else
        Lines[I].Left := Margin;
    end;
    Yes := TNewButton.Create(Form);
    Yes.Parent := Form;
    Yes.Caption := YesText;
    Yes.SetBounds(W - Margin - 2 * ButtonW - ScaleX(8), ButtonsT, ButtonW, ButtonH);
    Yes.ModalResult := mrYes;
    Yes.Default := True;
    No := TNewButton.Create(Form);
    No.Parent := Form;
    No.Caption := NoText;
    No.SetBounds(W - Margin - ButtonW, ButtonsT, ButtonW, ButtonH);
    No.ModalResult := mrNo;
    No.Cancel := True;
    Form.ActiveControl := Yes;
    { (closed: as No, English) }
    Result := Form.ShowModal() = mrYes;
  finally
    Form.Free();
  end;
  Log('Language: answered ' + BoolText(Result));
end;

{ Setup again, with these parameters: Verb 'open', or 'runas' for
  administrator rights. Through Windows: Exec and ShellExec refuse Setup's
  own exe until the installation has started. False if it didn't start
  (refused, or the rights not given). }
function StartSetup(Wnd: HWND; Verb, Params: String): Boolean;
var
  Code: Integer;
begin
  Code := ShellExecute(Wnd, Verb, ExpandConstant('{srcexe}'), Params, ExtractFileDir(ExpandConstant('{srcexe}')),
                       SW_SHOWNORMAL);
  Result := Code > 32;
  Log(Format('Setup again (%s %s): %d', [Verb, Params, Code]));
end;

{ Setup again in that language, with what this one was given, for the
  wizard can't change its language once it runs; False if it didn't start.
  A log of this one (/LOG=, still open) gets one next to it. }
function Relaunch(Name: String): Boolean;
var
  Params, P, F: String;
  I: Integer;
begin
  Params := '';
  for I := 1 to ParamCount do
  begin
    P := ParamStr(I);
    if CompareText(Copy(P, 1, 6), '/LANG=') <> 0 then
    begin
      if CompareText(Copy(P, 1, 5), '/LOG=') = 0 then
      begin
        F := Copy(P, 6, Length(P));
        P := '/LOG=' + ChangeFileExt(F, '') + '-' + Name + ExtractFileExt(F);
      end;
      Params := Params + AddQuotes(P) + ' ';
    end;
  end;
  Params := Params + '/LANG=' + Name;
  Log('Language: Setup again in ' + Name);
  Result := StartSetup(0, 'open', Params);
end;

{ The language of Setup and of the app (see the top), at the start. False
  when Setup starts again in another language: this one ends then. }
function ChooseLanguage(): Boolean;
var
  Given, Existing: String;
  L: Integer;
begin
  Result := True;
  LoadLanguages();
  Lang := LanguageOf(ActiveLanguage);
  if Lang < 0 then
    Lang := 0;
  AppLanguage := '';
  Given := ExpandConstant('{param:LANG|}');
  if LanguageOf(Given) < 0 then
    Given := '';
  Existing := ExistingLanguage();
  if Existing <> '' then
  begin
    { Setup in the app's language, if it has it }
    L := LanguageOfCode(Existing);
    Log('Language: the app has ' + Existing + ', kept; Setup in ' + ActiveLanguage);
    if (L >= 0) and (L <> Lang) and (Given = '') and not WizardSilent then
      Result := not Relaunch(Languages[L].Name);
    Exit;
  end;
  if Given <> '' then
  begin
    AppLanguage := Languages[Lang].Code;
    Log('Language: ' + ActiveLanguage + ' (/LANG=), for the app too');
    Exit;
  end;
  L := OfferedLanguage();
  if WizardSilent or (L < 0) then
  begin
    Log(Format('Language: Windows in $%.4x, silent %s: no question, Setup in %s', [UILanguage(),
               BoolText(WizardSilent), ActiveLanguage]));
    Exit;
  end;
  if not AskLanguage(L) then
    L := 0;
  AppLanguage := Languages[L].Code;
  if L <> Lang then
    Result := not Relaunch(Languages[L].Name);
end;

{ After the files: the app's language (see the top), unless its profile
  has one }
procedure SaveLanguage();
var
  FileName: String;
begin
  if AppLanguage = '' then
    Exit;
  FileName := ProfileFile(LANGUAGE_FILE);
  if FileExists(FileName) then
    Log('Language: ' + FileName + ' kept')
  else if SaveStringToFile(FileName, AppLanguage, False) then
    Log('Language: ' + FileName + ' written: ' + AppLanguage)
  else
    Log('Language: ' + FileName + ' could not be written');
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
  for I := 0 to 4 do
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
  Names: array[0..4] of String;
  I: Integer;
begin
  Parent := WizardForm.WelcomePage;
  SideMark := NewImage(Parent, False, 'mark', 24, 24, 28, 28);
  SideTitle := NewLabel(Parent, False, 60, 24, 0, 14, True, CLR_TEXT, CLR_CARD, '{#AppName}');
  SideSub := NewLabel(Parent, False, 60, 42, 0, 11, False, CLR_FAINT, CLR_CARD, CustomMessage('Unofficial'));
  Pill := NewImage(Parent, False, 'pill', 18, 84, 144, 32);
  Names[0] := CustomMessage('StepWelcome');
  Names[1] := CustomMessage('StepOptions');
  Names[2] := CustomMessage('StepPrivacy');
  Names[3] := CustomMessage('StepInstall');
  Names[4] := CustomMessage('StepDone');
  for I := 0 to 4 do
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
  Text, FileName: String;
  I: Integer;
begin
  { in the language of Setup, English if it has none }
  FileName := 'notice.' + ActiveLanguage + '.txt';
  try
    ExtractTemporaryFile(FileName);
  except
    FileName := 'notice.txt';
    ExtractTemporaryFile(FileName);
  end;
  Text := '';
  if LoadStringsFromFile(ExpandConstant('{tmp}\') + FileName, Lines) then
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
  Params: String;
begin
  { in this language, if it was chosen or isn't English (otherwise as
    Setup starts there) }
  Params := '/ALLUSERS';
  if (AppLanguage <> '') or (Lang <> 0) then
    Params := Params + ' /LANG=' + ActiveLanguage;
  if StartSetup(WizardForm.Handle, 'runas', Params) then
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

{ The two features that go online by themselves, each with what it sends,
  and the policy (the toggles take the profile's choices when it shows) }
{ Right to left only, and once (see the top): the app's layout, mirrored
  or as designed, off to begin with as in the app }
procedure CreateLayout();
var
  P: TWinControl;
begin
  LayoutPage := CreateCustomPage(OptionsPage.ID, CustomMessage('LayoutTitle'), CustomMessage('LayoutText'));
  P := LayoutPage.Surface;
  NewToggle(T_RTL, P, True, 100, CustomMessage('LayoutRTL'), HasSwitch('RIGHTTOLEFT'));
  NewLabel(P, True, CONTENT_L, 138, CONTENT_W, 11, False, CLR_FAINT, CLR_BACK, CustomMessage('LayoutRTLText'));
  NewImage(P, True, 'card-tip', CONTENT_L, 196, CONTENT_W, 92);
  NewLabel(P, True, CONTENT_L + 16, 210, CONTENT_W - 32, 13, True, CLR_TEXT, CLR_CARD, CustomMessage('LayoutCardTitle'));
  NewLabel(P, True, CONTENT_L + 16, 232, CONTENT_W - 32, 12, False, CLR_TEXT2, CLR_CARD, CustomMessage('LayoutCardText'));
end;

{ The Layout page shows: right to left, never chosen }
function ShowLayout(): Boolean;
begin
  Result := Languages[Lang].RTL and not LayoutChosen();
end;

procedure CreatePrivacy();
var
  P: TWinControl;
begin
  PrivacyPage := CreateCustomPage(LayoutPage.ID, CustomMessage('PrivacyTitle'), CustomMessage('PrivacyText'));
  P := PrivacyPage.Surface;
  NewToggle(T_UPDATES, P, True, 100, CustomMessage('PrivacyUpdates'), True);
  NewLabel(P, True, CONTENT_L, 138, CONTENT_W, 11, False, CLR_FAINT, CLR_BACK, CustomMessage('PrivacyUpdatesText'));
  NewToggle(T_SUBTITLES, P, True, 166, CustomMessage('PrivacySubtitles'), True);
  NewLabel(P, True, CONTENT_L, 204, CONTENT_W, 11, False, CLR_FAINT, CLR_BACK, CustomMessage('PrivacySubtitlesText'));
  NewImage(P, True, 'card-tip', CONTENT_L, 254, CONTENT_W, 92);
  NewLabel(P, True, CONTENT_L + 16, 268, CONTENT_W - 32, 13, True, CLR_TEXT, CLR_CARD, CustomMessage('PrivacyCardTitle'));
  NewLabel(P, True, CONTENT_L + 16, 290, CONTENT_W - 32, 12, False, CLR_TEXT2, CLR_CARD, CustomMessage('PrivacyCardText'));
  NewLink(P, True, CONTENT_L + 16, 322, 12, CLR_CARD, CustomMessage('LinkPrivacy'), @PrivacyPolicyClick);
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
  MeasureToggleWords();
  CreateOptionToggles();
  CreateLayout();
  CreatePrivacy();
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
    ShowStep(4, WizardForm.FinishedPage);
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
      { the last page before the installation (no Ready page) when Layout
        and Privacy are skipped }
      if ChoicesMade() and not ShowLayout() then
        WizardForm.NextButton.Caption := SetupMessage(msgButtonInstall)
      else
        WizardForm.NextButton.Caption := SetupMessage(msgButtonNext);
    end
    else if CurPageID = LayoutPage.ID then
    begin
      ShowStep(1, WizardForm.InnerPage);
      SetContentText(InnerTitle, LayoutPage.Caption);
      SetContentText(InnerText, LayoutPage.Description);
      LayoutShown := True;
      if ChoicesMade() then
        WizardForm.NextButton.Caption := SetupMessage(msgButtonInstall)
      else
        WizardForm.NextButton.Caption := SetupMessage(msgButtonNext);
    end
    else if CurPageID = PrivacyPage.ID then
    begin
      ShowStep(2, WizardForm.InnerPage);
      SetContentText(InnerTitle, PrivacyPage.Caption);
      SetContentText(InnerText, PrivacyPage.Description);
      LoadChoices();
      PrivacyShown := True;
      WizardForm.NextButton.Caption := SetupMessage(msgButtonInstall);
    end
    else if CurPageID = wpInstalling then
    begin
      ShowStep(3, WizardForm.InnerPage);
      SetContentText(InnerTitle, CustomMessage('InstallingTitle'));
      SetContentText(InnerText, CustomMessage('InstallingText'));
    end
    else
    begin
      ShowStep(3, WizardForm.InnerPage);
      SetContentText(InnerTitle, WizardForm.PageNameLabel.Caption);
      SetContentText(InnerText, WizardForm.PageDescriptionLabel.Caption);
    end;
  end;
  { the focus on Setup's own Next (out of sight): Enter goes on, and no
    field or button of ours starts selected }
  if WizardForm.NextButton.Visible and WizardForm.NextButton.Enabled then
    WizardForm.ActiveControl := WizardForm.NextButton;
  if IsRTL then
    RepaintToggles();
  SyncButtons();
end;

{ Privacy: once (an update of a profile asked before goes on to install;
  the choices stay in Settings). Layout: right to left, once too. }
function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := False;
  if (PrivacyPage <> nil) and (PageID = PrivacyPage.ID) then
  begin
    Result := ChoicesMade();
    if Result then
      PrivacyShown := False;
  end
  else if (LayoutPage <> nil) and (PageID = LayoutPage.ID) then
  begin
    Result := not ShowLayout();
    if Result then
      LayoutShown := False;
  end;
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
  { (after the fonts: the question uses them) }
  Result := ChooseLanguage();
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
  RegDeleteKeyIncludingSubkeys(HKA, CAPABILITIES);
  { (not the profiles the copies noted: the settings of the next copy)
  }
  DeleteKeyIfEmpty('Software\{#AppName}');
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  Code: Integer;
begin
  if CurStep = ssPostInstall then
  begin
    SaveLanguage();
    SaveChoices();
    RegisterFileTypes();
    if not WantDesktopIcon() then
      DeleteFile(ExpandConstant('{autodesktop}\{#AppName}.lnk'));
    { Windows may check a new program for seconds the first time it runs
      (up to 10 s measured): let it check VLC and its helpers now, hidden
      and without waiting (all return at once), not when a file is opened.
      Not --version: it opens a console and waits for a key }
    Exec(AppExe(), '--prewarm', '', SW_HIDE, ewNoWait, Code);
    Exec(ExpandConstant('{app}\vlc-preparser.exe'), '--help', '', SW_HIDE, ewNoWait, Code);
    Exec(ExpandConstant('{app}\vlc-waveform.exe'), '', '', SW_HIDE, ewNoWait, Code);
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
    { The program goes with its settings, library and history (an update
      keeps them: it installs over the program, never uninstalls it) }
    if DirExists(ExpandConstant('{app}\portable')) then
      DelTree(ExpandConstant('{app}\portable'), True, True, True);
    { no copy starts with these settings any more }
    RegDeleteValue(HKCU, PROFILES, ExpandConstant('{app}\portable'));
    RegDeleteKeyIfEmpty(HKCU, PROFILES);
    RegDeleteKeyIfEmpty(HKCU, 'Software\{#AppName}');
    RemoveDir(ExpandConstant('{app}'));
  end;
end;
