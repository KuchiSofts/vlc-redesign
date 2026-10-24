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
#dim LangName[32]
#dim LangIsl[32]
#dim LangCode[32]
#dim LangId[32]
#dim LangRTL[32]
#dim LangInter[32]
#dim LangEnglish[32]
#define AddLanguage(str Name, str Isl, str Code, int Id, int RTL, int Inter, str English) \
  LangName[LangCount] = Name, LangIsl[LangCount] = Isl, LangCode[LangCount] = Code, \
  LangId[LangCount] = Id, LangRTL[LangCount] = RTL, LangInter[LangCount] = Inter, \
  LangEnglish[LangCount] = English, ++LangCount
#expr AddLanguage("english", "compiler:Default.isl", "en", 0x0009, 0, 1, "English")
#expr AddLanguage("hebrew", "compiler:Languages\Hebrew.isl", "he", 0x000D, 1, 0, "Hebrew")
#expr AddLanguage("brazilianportuguese", "compiler:Languages\BrazilianPortuguese.isl", "pt_BR", 0x0416, 0, 1, "Portuguese")
#expr AddLanguage("dutch", "compiler:Languages\Dutch.isl", "nl", 0x0013, 0, 1, "Dutch")
#expr AddLanguage("german", "compiler:Languages\German.isl", "de", 0x0007, 0, 1, "German")
#expr AddLanguage("czech", "compiler:Languages\Czech.isl", "cs", 0x0005, 0, 1, "Czech")
#expr AddLanguage("swedish", "compiler:Languages\Swedish.isl", "sv", 0x001D, 0, 1, "Swedish")
#expr AddLanguage("portuguese", "compiler:Languages\Portuguese.isl", "pt_PT", 0x0816, 0, 1, "Portuguese")
#expr AddLanguage("french", "compiler:Languages\French.isl", "fr", 0x000C, 0, 1, "French")
#expr AddLanguage("korean", "compiler:Languages\Korean.isl", "ko", 0x0012, 0, 0, "Korean")
#expr AddLanguage("japanese", "compiler:Languages\Japanese.isl", "ja", 0x0011, 0, 0, "Japanese")
#expr AddLanguage("italian", "compiler:Languages\Italian.isl", "it", 0x0010, 0, 1, "Italian")
#expr AddLanguage("spanish", "compiler:Languages\Spanish.isl", "es", 0x000A, 0, 1, "Spanish")
#expr AddLanguage("polish", "compiler:Languages\Polish.isl", "pl", 0x0015, 0, 1, "Polish")
#expr AddLanguage("ukrainian", "compiler:Languages\Ukrainian.isl", "uk", 0x0022, 0, 1, "Ukrainian")
#expr AddLanguage("russian", "compiler:Languages\Russian.isl", "ru", 0x0019, 0, 1, "Russian")
#expr AddLanguage("arabic", "compiler:Languages\Arabic.isl", "ar", 0x0001, 1, 0, "Arabic")
#expr AddLanguage("turkish", "compiler:Languages\Turkish.isl", "tr", 0x001F, 0, 1, "Turkish")

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
dutch.StepWelcome=Welkom
german.StepWelcome=Willkommen
czech.StepWelcome=Vítejte
swedish.StepWelcome=Välkommen
portuguese.StepWelcome=Boas-vindas
french.StepWelcome=Bienvenue
korean.StepWelcome=환영
japanese.StepWelcome=ようこそ
italian.StepWelcome=Benvenuto
spanish.StepWelcome=Bienvenida
polish.StepWelcome=Powitanie
ukrainian.StepWelcome=Вітання
russian.StepWelcome=Приветствие
arabic.StepWelcome=مرحبًا
turkish.StepWelcome=Hoş geldiniz
english.StepOptions=Options
hebrew.StepOptions=אפשרויות
brazilianportuguese.StepOptions=Opções
dutch.StepOptions=Opties
german.StepOptions=Optionen
czech.StepOptions=Možnosti
swedish.StepOptions=Alternativ
portuguese.StepOptions=Opções
french.StepOptions=Options
korean.StepOptions=옵션
japanese.StepOptions=オプション
italian.StepOptions=Opzioni
spanish.StepOptions=Opciones
polish.StepOptions=Opcje
ukrainian.StepOptions=Параметри
russian.StepOptions=Параметры
arabic.StepOptions=الخيارات
turkish.StepOptions=Seçenekler
english.StepPrivacy=Privacy
hebrew.StepPrivacy=פרטיות
brazilianportuguese.StepPrivacy=Privacidade
dutch.StepPrivacy=Privacy
german.StepPrivacy=Datenschutz
czech.StepPrivacy=Soukromí
swedish.StepPrivacy=Sekretess
portuguese.StepPrivacy=Privacidade
french.StepPrivacy=Confidentialité
korean.StepPrivacy=개인정보
japanese.StepPrivacy=プライバシー
italian.StepPrivacy=Privacy
spanish.StepPrivacy=Privacidad
polish.StepPrivacy=Prywatność
ukrainian.StepPrivacy=Приватність
russian.StepPrivacy=Приватность
arabic.StepPrivacy=الخصوصية
turkish.StepPrivacy=Gizlilik
english.StepInstall=Install
hebrew.StepInstall=התקנה
brazilianportuguese.StepInstall=Instalação
dutch.StepInstall=Installeren
german.StepInstall=Installation
czech.StepInstall=Instalace
swedish.StepInstall=Installation
portuguese.StepInstall=Instalação
french.StepInstall=Installation
korean.StepInstall=설치
japanese.StepInstall=インストール
italian.StepInstall=Installazione
spanish.StepInstall=Instalación
polish.StepInstall=Instalacja
ukrainian.StepInstall=Встановлення
russian.StepInstall=Установка
arabic.StepInstall=التثبيت
turkish.StepInstall=Kurulum
english.StepDone=Done
hebrew.StepDone=סיום
brazilianportuguese.StepDone=Concluído
dutch.StepDone=Klaar
german.StepDone=Fertig
czech.StepDone=Hotovo
swedish.StepDone=Klart
portuguese.StepDone=Concluído
french.StepDone=Terminé
korean.StepDone=완료
japanese.StepDone=完了
italian.StepDone=Fine
spanish.StepDone=Listo
polish.StepDone=Gotowe
ukrainian.StepDone=Готово
russian.StepDone=Готово
arabic.StepDone=تم
turkish.StepDone=Bitti
english.Unofficial=Unofficial build
hebrew.Unofficial=גרסה לא רשמית
brazilianportuguese.Unofficial=Não oficial
dutch.Unofficial=Onofficiële build
german.Unofficial=Inoffizieller Build
czech.Unofficial=Neoficiální sestavení
swedish.Unofficial=Inofficiell version
portuguese.Unofficial=Não oficial
french.Unofficial=Version non officielle
korean.Unofficial=비공식 빌드
japanese.Unofficial=非公式ビルド
italian.Unofficial=Build non ufficiale
spanish.Unofficial=Versión no oficial
polish.Unofficial=Nieoficjalna wersja
ukrainian.Unofficial=Неофіційна збірка
russian.Unofficial=Неофициальная
arabic.Unofficial=إصدار غير رسمي
turkish.Unofficial=Resmî olmayan derleme
english.DesignedBy=Interface designed by
hebrew.DesignedBy=הממשק עוצב בידי
brazilianportuguese.DesignedBy=Interface criada por
dutch.DesignedBy=Interface ontworpen door
german.DesignedBy=Oberfläche gestaltet von
czech.DesignedBy=Rozhraní navrhla
swedish.DesignedBy=Gränssnitt designat av
portuguese.DesignedBy=Interface desenhada por
french.DesignedBy=Interface conçue par
korean.DesignedBy=인터페이스 디자인:
japanese.DesignedBy=インターフェースデザイン
italian.DesignedBy=Interfaccia progettata da
spanish.DesignedBy=Interfaz diseñada por
polish.DesignedBy=Projekt interfejsu
ukrainian.DesignedBy=Дизайн інтерфейсу
russian.DesignedBy=Дизайн интерфейса:
arabic.DesignedBy=الواجهة من تصميم
turkish.DesignedBy=Arayüzü tasarlayan
english.WelcomeTitle=Welcome to {#AppName}
hebrew.WelcomeTitle=ברוכים הבאים ל-{#AppName}
brazilianportuguese.WelcomeTitle=Boas-vindas ao {#AppName}
dutch.WelcomeTitle=Welkom bij {#AppName}
german.WelcomeTitle=Willkommen bei {#AppName}
czech.WelcomeTitle=Vítejte ve {#AppName}
swedish.WelcomeTitle=Välkommen till {#AppName}
portuguese.WelcomeTitle=Bem-vindo ao {#AppName}
french.WelcomeTitle=Bienvenue dans {#AppName}
korean.WelcomeTitle={#AppName}에 오신 것을 환영합니다
japanese.WelcomeTitle={#AppName}へようこそ
italian.WelcomeTitle=Benvenuto in {#AppName}
spanish.WelcomeTitle=Bienvenido a {#AppName}
polish.WelcomeTitle=Witaj w {#AppName}
ukrainian.WelcomeTitle=Ласкаво просимо до {#AppName}
russian.WelcomeTitle=Добро пожаловать в {#AppName}
arabic.WelcomeTitle=مرحبًا بك في {#AppName}
turkish.WelcomeTitle={#AppName}'a hoş geldiniz
english.WelcomeText=The real VLC 4 engine with the interface designed by Juxtopposed. Setup takes about a minute.
hebrew.WelcomeText=מנוע VLC 4 האמיתי עם הממשק שעיצבה Juxtopposed. ההתקנה נמשכת כדקה.
brazilianportuguese.WelcomeText=O verdadeiro motor do VLC 4, com a interface criada por Juxtopposed. A instalação leva cerca de um minuto.
dutch.WelcomeText=De echte VLC 4-engine met de interface ontworpen door Juxtopposed. De installatie duurt ongeveer een minuut.
german.WelcomeText=Die echte VLC-4-Engine mit der von Juxtopposed gestalteten Oberfläche. Die Installation dauert etwa eine Minute.
czech.WelcomeText=Skutečné jádro VLC 4 s rozhraním, které navrhla Juxtopposed. Instalace trvá zhruba minutu.
swedish.WelcomeText=Den riktiga VLC 4-motorn med gränssnittet som Juxtopposed har designat. Installationen tar ungefär en minut.
portuguese.WelcomeText=O verdadeiro motor do VLC 4 com a interface desenhada por Juxtopposed. A instalação demora cerca de um minuto.
french.WelcomeText=Le vrai moteur de VLC 4 avec l'interface conçue par Juxtopposed. L'installation prend environ une minute.
korean.WelcomeText=Juxtopposed가 디자인한 인터페이스에 진짜 VLC 4 엔진을 담았습니다. 설치는 1분 정도 걸립니다.
japanese.WelcomeText=Juxtopposedがデザインしたインターフェースを備えた、本物のVLC 4エンジン。セットアップは約1分で完了します。
italian.WelcomeText=Il vero motore di VLC 4 con l'interfaccia progettata da Juxtopposed. L'installazione richiede circa un minuto.
spanish.WelcomeText=El auténtico motor de VLC 4 con la interfaz diseñada por Juxtopposed. La instalación tarda alrededor de un minuto.
polish.WelcomeText=Prawdziwy silnik VLC 4 z interfejsem zaprojektowanym przez Juxtopposed. Instalacja trwa około minuty.
ukrainian.WelcomeText=Справжній рушій VLC 4 з інтерфейсом від Juxtopposed. Встановлення триває близько хвилини.
russian.WelcomeText=Настоящий движок VLC 4 с интерфейсом от Juxtopposed. Установка займёт около минуты.
arabic.WelcomeText=محرك VLC 4 الحقيقي مع الواجهة التي صممتها Juxtopposed. يستغرق الإعداد نحو دقيقة.
turkish.WelcomeText=Juxtopposed'un tasarladığı arayüzle gerçek VLC 4 motoru. Kurulum yaklaşık bir dakika sürer.
english.CreditTitle=Interface designed by Juxtopposed
hebrew.CreditTitle=את הממשק עיצבה Juxtopposed
brazilianportuguese.CreditTitle=Interface criada por Juxtopposed
dutch.CreditTitle=Interface ontworpen door Juxtopposed
german.CreditTitle=Oberfläche gestaltet von Juxtopposed
czech.CreditTitle=Rozhraní navrhla Juxtopposed
swedish.CreditTitle=Gränssnitt designat av Juxtopposed
portuguese.CreditTitle=Interface desenhada por Juxtopposed
french.CreditTitle=Interface conçue par Juxtopposed
korean.CreditTitle=인터페이스 디자인: Juxtopposed
japanese.CreditTitle=インターフェースデザイン: Juxtopposed
italian.CreditTitle=Interfaccia progettata da Juxtopposed
spanish.CreditTitle=Interfaz diseñada por Juxtopposed
polish.CreditTitle=Projekt interfejsu: Juxtopposed
ukrainian.CreditTitle=Дизайн інтерфейсу — Juxtopposed
russian.CreditTitle=Дизайн интерфейса — Juxtopposed
arabic.CreditTitle=الواجهة من تصميم Juxtopposed
turkish.CreditTitle=Arayüzü tasarlayan: Juxtopposed
english.CreditText=Every screen comes from the "VLC Redesign (Community)" Figma file, licensed under CC BY 4.0. Juxtopposed does not endorse this project.
hebrew.CreditText=כל מסך מגיע מקובץ ה-Figma "VLC Redesign (Community)", ברישיון CC BY 4.0. Juxtopposed אינה מעורבת בפרויקט הזה ואינה ממליצה עליו.
brazilianportuguese.CreditText=Cada tela vem do arquivo do Figma "VLC Redesign (Community)", sob a licença CC BY 4.0. Juxtopposed não endossa este projeto.
dutch.CreditText=Elk scherm komt uit het Figma-bestand "VLC Redesign (Community)", gelicentieerd onder CC BY 4.0. Juxtopposed onderschrijft dit project niet.
german.CreditText=Jeder Bildschirm stammt aus der Figma-Datei „VLC Redesign (Community)“, lizenziert unter CC BY 4.0. Juxtopposed unterstützt dieses Projekt nicht.
czech.CreditText=Všechny obrazovky pocházejí ze souboru Figma „VLC Redesign (Community)“ s licencí CC BY 4.0. Juxtopposed tento projekt nijak nezaštiťuje.
swedish.CreditText=Varje skärm kommer från Figma-filen ”VLC Redesign (Community)”, licensierad under CC BY 4.0. Juxtopposed står inte bakom det här projektet.
portuguese.CreditText=Todos os ecrãs vêm do ficheiro Figma "VLC Redesign (Community)", licenciado sob CC BY 4.0. Juxtopposed não apoia este projeto.
french.CreditText=Chaque écran provient du fichier Figma « VLC Redesign (Community) », sous licence CC BY 4.0. Juxtopposed n'approuve pas ce projet.
korean.CreditText=모든 화면은 CC BY 4.0 라이선스의 "VLC Redesign (Community)" Figma 파일에서 가져왔습니다. Juxtopposed는 이 프로젝트를 보증하지 않습니다.
japanese.CreditText=すべての画面は、CC BY 4.0でライセンスされたFigmaファイル「VLC Redesign (Community)」に基づいています。Juxtopposedはこのプロジェクトを承認していません。
italian.CreditText=Ogni schermata proviene dal file Figma "VLC Redesign (Community)", con licenza CC BY 4.0. Juxtopposed non approva questo progetto.
spanish.CreditText=Todas las pantallas provienen del archivo de Figma «VLC Redesign (Community)», con licencia CC BY 4.0. Juxtopposed no respalda este proyecto.
polish.CreditText=Każdy ekran pochodzi z pliku Figma "VLC Redesign (Community)" na licencji CC BY 4.0. Juxtopposed nie popiera tego projektu.
ukrainian.CreditText=Усі екрани взято з файлу Figma «VLC Redesign (Community)» за ліцензією CC BY 4.0. Juxtopposed не схвалює цей проєкт.
russian.CreditText=Все экраны взяты из файла Figma «VLC Redesign (Community)» под лицензией CC BY 4.0. Juxtopposed не одобряет этот проект.
arabic.CreditText=كل الشاشات مأخوذة من ملف Figma "VLC Redesign (Community)"‎ المرخّص بموجب CC BY 4.0. لا تؤيد Juxtopposed هذا المشروع.
turkish.CreditText=Tüm ekranlar, CC BY 4.0 lisanslı "VLC Redesign (Community)" Figma dosyasından alınmıştır. Juxtopposed bu projeyi onaylamaz.
english.LinkVideo=Watch the video
hebrew.LinkVideo=לצפייה בסרטון
brazilianportuguese.LinkVideo=Assistir ao vídeo
dutch.LinkVideo=Bekijk de video
german.LinkVideo=Video ansehen
czech.LinkVideo=Podívat se na video
swedish.LinkVideo=Se videon
portuguese.LinkVideo=Ver o vídeo
french.LinkVideo=Voir la vidéo
korean.LinkVideo=영상 보기
japanese.LinkVideo=動画を見る
italian.LinkVideo=Guarda il video
spanish.LinkVideo=Ver el vídeo
polish.LinkVideo=Obejrzyj film
ukrainian.LinkVideo=Дивитися відео
russian.LinkVideo=Смотреть видео
arabic.LinkVideo=شاهد الفيديو
turkish.LinkVideo=Videoyu izleyin
english.LinkChannel=YouTube channel
hebrew.LinkChannel=ערוץ היוטיוב
brazilianportuguese.LinkChannel=Canal no YouTube
dutch.LinkChannel=YouTube-kanaal
german.LinkChannel=YouTube-Kanal
czech.LinkChannel=Kanál YouTube
swedish.LinkChannel=YouTube-kanal
portuguese.LinkChannel=Canal do YouTube
french.LinkChannel=Chaîne YouTube
korean.LinkChannel=YouTube 채널
japanese.LinkChannel=YouTubeチャンネル
italian.LinkChannel=Canale YouTube
spanish.LinkChannel=Canal de YouTube
polish.LinkChannel=Kanał YouTube
ukrainian.LinkChannel=Канал YouTube
russian.LinkChannel=Канал на YouTube
arabic.LinkChannel=قناة YouTube
turkish.LinkChannel=YouTube kanalı
english.LinkFigma=Figma file
hebrew.LinkFigma=קובץ ה-Figma
brazilianportuguese.LinkFigma=Arquivo do Figma
dutch.LinkFigma=Figma-bestand
german.LinkFigma=Figma-Datei
czech.LinkFigma=Soubor Figma
swedish.LinkFigma=Figma-fil
portuguese.LinkFigma=Ficheiro Figma
french.LinkFigma=Fichier Figma
korean.LinkFigma=Figma 파일
japanese.LinkFigma=Figmaファイル
italian.LinkFigma=File Figma
spanish.LinkFigma=Archivo de Figma
polish.LinkFigma=Plik Figma
ukrainian.LinkFigma=Файл Figma
russian.LinkFigma=Файл Figma
arabic.LinkFigma=ملف Figma
turkish.LinkFigma=Figma dosyası
english.WelcomeNotice=An unofficial build, not made or endorsed by VideoLAN. VLC and the cone logo are trademarks of VideoLAN. It installs next to an official VLC without touching it.
hebrew.WelcomeNotice=גרסה לא רשמית, שאינה של VideoLAN ואינה מאושרת על ידה. VLC ולוגו הקונוס הם סימנים מסחריים של VideoLAN. היא מותקנת לצד VLC רשמי בלי לגעת בו.
brazilianportuguese.WelcomeNotice=Uma versão não oficial, que não é feita nem endossada pela VideoLAN. VLC e o logotipo do cone são marcas da VideoLAN. Ela é instalada ao lado de um VLC oficial, sem mexer nele.
dutch.WelcomeNotice=Een onofficiële build, niet gemaakt of goedgekeurd door VideoLAN. VLC en het kegellogo zijn handelsmerken van VideoLAN. Wordt naast een officiële VLC geïnstalleerd zonder die aan te raken.
german.WelcomeNotice=Ein inoffizieller Build, weder von VideoLAN erstellt noch unterstützt. VLC und das Kegel-Logo sind Marken von VideoLAN. Er wird neben einem offiziellen VLC installiert, ohne es zu verändern.
czech.WelcomeNotice=Neoficiální sestavení, které nevytvořila ani neschválila organizace VideoLAN. VLC a logo kužele jsou ochranné známky VideoLAN. Instaluje se vedle oficiálního VLC, aniž by se ho dotklo.
swedish.WelcomeNotice=En inofficiell version, inte gjord eller godkänd av VideoLAN. VLC och konlogotypen är varumärken som tillhör VideoLAN. Den installeras bredvid en officiell VLC utan att röra den.
portuguese.WelcomeNotice=Uma compilação não oficial, não feita nem apoiada pela VideoLAN. VLC e o logótipo do cone são marcas da VideoLAN. Instala-se ao lado de um VLC oficial sem lhe tocar.
french.WelcomeNotice=Version non officielle, ni créée ni approuvée par VideoLAN. VLC et le logo du cône sont des marques de VideoLAN. Elle s'installe à côté d'un VLC officiel sans y toucher.
korean.WelcomeNotice=VideoLAN이 만들거나 보증하지 않은 비공식 빌드입니다. VLC 및 원뿔 로고는 VideoLAN의 상표입니다. 공식 VLC를 건드리지 않고 그 옆에 설치됩니다.
japanese.WelcomeNotice=VideoLANが作成・承認したものではない非公式ビルドです。VLCとコーンのロゴはVideoLANの商標です。公式のVLCには手を加えず、その隣にインストールされます。
italian.WelcomeNotice=Una build non ufficiale, non realizzata né approvata da VideoLAN. VLC e il logo del cono sono marchi di VideoLAN. Si installa accanto a un VLC ufficiale senza modificarlo.
spanish.WelcomeNotice=Una versión no oficial, no creada ni respaldada por VideoLAN. VLC y el logotipo del cono son marcas comerciales de VideoLAN. Se instala junto a un VLC oficial sin modificarlo.
polish.WelcomeNotice=Nieoficjalna wersja, niestworzona ani niepopierana przez VideoLAN. VLC i logo pachołka są znakami towarowymi VideoLAN. Instaluje się obok oficjalnego VLC, nie naruszając go.
ukrainian.WelcomeNotice=Неофіційна збірка, яку не створювала й не схвалювала VideoLAN. VLC і логотип-конус є торговельними марками VideoLAN. Встановлюється поруч з офіційним VLC, не зачіпаючи його.
russian.WelcomeNotice=Неофициальная сборка, не созданная и не одобренная VideoLAN. VLC и логотип с конусом — товарные знаки VideoLAN. Устанавливается рядом с официальным VLC, не затрагивая его.
arabic.WelcomeNotice=إصدار غير رسمي، لم تصنعه VideoLAN ولا تؤيده. ‏VLC وشعار المخروط علامتان تجاريتان لـ VideoLAN. يُثبَّت بجانب VLC الرسمي دون المساس به.
turkish.WelcomeNotice=VideoLAN tarafından yapılmamış ya da onaylanmamış, resmî olmayan bir derlemedir. VLC ve koni logosu VideoLAN'ın ticari markalarıdır. Resmî VLC'ye dokunmadan onun yanına kurulur.
english.LinkNotice=Notice and licenses
hebrew.LinkNotice=הודעה ורישיונות
brazilianportuguese.LinkNotice=Aviso e licenças
dutch.LinkNotice=Kennisgeving en licenties
german.LinkNotice=Hinweis und Lizenzen
czech.LinkNotice=Upozornění a licence
swedish.LinkNotice=Information och licenser
portuguese.LinkNotice=Aviso e licenças
french.LinkNotice=Avis et licences
korean.LinkNotice=고지 및 라이선스
japanese.LinkNotice=注意事項とライセンス
italian.LinkNotice=Note e licenze
spanish.LinkNotice=Aviso y licencias
polish.LinkNotice=Informacje i licencje
ukrainian.LinkNotice=Повідомлення й ліцензії
russian.LinkNotice=Уведомление и лицензии
arabic.LinkNotice=الإشعار والتراخيص
turkish.LinkNotice=Bildirim ve lisanslar
english.LinkSource=Source code
hebrew.LinkSource=קוד מקור
brazilianportuguese.LinkSource=Código-fonte
dutch.LinkSource=Broncode
german.LinkSource=Quellcode
czech.LinkSource=Zdrojový kód
swedish.LinkSource=Källkod
portuguese.LinkSource=Código-fonte
french.LinkSource=Code source
korean.LinkSource=소스 코드
japanese.LinkSource=ソースコード
italian.LinkSource=Codice sorgente
spanish.LinkSource=Código fuente
polish.LinkSource=Kod źródłowy
ukrainian.LinkSource=Вихідний код
russian.LinkSource=Исходный код
arabic.LinkSource=الشفرة المصدرية
turkish.LinkSource=Kaynak kodu
english.VersionLine=Version %1 · free software under the GNU GPL v2 or later
hebrew.VersionLine=גרסה %1 · תוכנה חופשית ברישיון GNU GPL גרסה 2 ומעלה
brazilianportuguese.VersionLine=Versão %1 · software livre sob a GNU GPL v2 ou posterior
dutch.VersionLine=Versie %1 · vrije software onder de GNU GPL v2 of later
german.VersionLine=Version %1 · freie Software unter der GNU GPL v2 oder neuer
czech.VersionLine=Verze %1 · svobodný software pod licencí GNU GPL v2 nebo novější
swedish.VersionLine=Version %1 · fri programvara under GNU GPL v2 eller senare
portuguese.VersionLine=Versão %1 · software livre sob a GNU GPL v2 ou posterior
french.VersionLine=Version %1 · logiciel libre sous GNU GPL v2 ou ultérieure
korean.VersionLine=버전 %1 · GNU GPL v2 이상에 따른 자유 소프트웨어
japanese.VersionLine=バージョン %1 · GNU GPL v2以降のフリーソフトウェア
italian.VersionLine=Versione %1 · software libero con licenza GNU GPL v2 o successiva
spanish.VersionLine=Versión %1 · software libre bajo la GNU GPL v2 o posterior
polish.VersionLine=Wersja %1 · wolne oprogramowanie na licencji GNU GPL v2 lub nowszej
ukrainian.VersionLine=Версія %1 · вільне ПЗ за ліцензією GNU GPL v2 або новішою
russian.VersionLine=Версия %1 · свободное ПО под лицензией GNU GPL v2 или более поздней
arabic.VersionLine=الإصدار %1 · برنامج حر بموجب GNU GPL v2 أو أحدث
turkish.VersionLine=Sürüm %1 · GNU GPL v2 ya da sonrası kapsamında özgür yazılım
english.OptionsTitle=Options
hebrew.OptionsTitle=אפשרויות
brazilianportuguese.OptionsTitle=Opções
dutch.OptionsTitle=Opties
german.OptionsTitle=Optionen
czech.OptionsTitle=Možnosti
swedish.OptionsTitle=Alternativ
portuguese.OptionsTitle=Opções
french.OptionsTitle=Options
korean.OptionsTitle=옵션
japanese.OptionsTitle=オプション
italian.OptionsTitle=Opzioni
spanish.OptionsTitle=Opciones
polish.OptionsTitle=Opcje
ukrainian.OptionsTitle=Параметри
russian.OptionsTitle=Параметры
arabic.OptionsTitle=الخيارات
turkish.OptionsTitle=Seçenekler
english.OptionsText=Where it goes, and what it opens.
hebrew.OptionsText=לאן להתקין, ומה לפתוח איתה.
brazilianportuguese.OptionsText=Onde instalar e o que abrir com ele.
dutch.OptionsText=Waar het komt, en wat het opent.
german.OptionsText=Wohin es installiert wird und was es öffnet.
czech.OptionsText=Kam se nainstaluje a co bude otevírat.
swedish.OptionsText=Var den installeras och vad den öppnar.
portuguese.OptionsText=Onde fica e o que abre.
french.OptionsText=Où l'installer, et ce qu'il ouvre.
korean.OptionsText=설치할 위치와 이 앱으로 열 파일.
japanese.OptionsText=インストール先と、開くファイルの種類。
italian.OptionsText=Dove installarlo e cosa aprire con esso.
spanish.OptionsText=Dónde se instala y qué abre.
polish.OptionsText=Gdzie zainstalować i co otwierać.
ukrainian.OptionsText=Куди встановити й що ним відкривати.
russian.OptionsText=Куда установить и что открывать.
arabic.OptionsText=مكان التثبيت، وما يفتحه.
turkish.OptionsText=Nereye kurulacağı ve neleri açacağı.
english.InstallTo=Install to
hebrew.InstallTo=התקנה אל
brazilianportuguese.InstallTo=Instalar em
dutch.InstallTo=Installeren in
german.InstallTo=Installieren nach
czech.InstallTo=Instalovat do
swedish.InstallTo=Installera i
portuguese.InstallTo=Instalar em
french.InstallTo=Installer dans
korean.InstallTo=설치 위치
japanese.InstallTo=インストール先
italian.InstallTo=Installa in
spanish.InstallTo=Instalar en
polish.InstallTo=Zainstaluj w
ukrainian.InstallTo=Встановити в
russian.InstallTo=Папка установки
arabic.InstallTo=التثبيت في
turkish.InstallTo=Kurulum yeri
english.Browse=Browse
hebrew.Browse=עיון
brazilianportuguese.Browse=Procurar
dutch.Browse=Bladeren
german.Browse=Durchsuchen
czech.Browse=Procházet
swedish.Browse=Bläddra
portuguese.Browse=Procurar
french.Browse=Parcourir
korean.Browse=찾아보기
japanese.Browse=参照
italian.Browse=Sfoglia
spanish.Browse=Examinar
polish.Browse=Przeglądaj
ukrainian.Browse=Огляд
russian.Browse=Обзор
arabic.Browse=استعراض
turkish.Browse=Gözat
english.ForYou=For you only, without administrator rights.
hebrew.ForYou=עבורך בלבד, בלי הרשאות מנהל.
brazilianportuguese.ForYou=Só para você, sem direitos de administrador.
dutch.ForYou=Alleen voor u, zonder beheerdersrechten.
german.ForYou=Nur für Sie, ohne Administratorrechte.
czech.ForYou=Jen pro vás, bez práv správce.
swedish.ForYou=Bara för dig, utan administratörsbehörighet.
portuguese.ForYou=Só para si, sem direitos de administrador.
french.ForYou=Pour vous uniquement, sans droits d'administrateur.
korean.ForYou=현재 사용자만, 관리자 권한 없이 설치합니다.
japanese.ForYou=現在のユーザーのみ。管理者権限は不要です。
italian.ForYou=Solo per te, senza diritti di amministratore.
spanish.ForYou=Solo para usted, sin permisos de administrador.
polish.ForYou=Tylko dla Ciebie, bez uprawnień administratora.
ukrainian.ForYou=Лише для вас, без прав адміністратора.
russian.ForYou=Только для вас, без прав администратора.
arabic.ForYou=لك وحدك، دون صلاحيات المسؤول.
turkish.ForYou=Yalnızca sizin için, yönetici izinleri olmadan.
english.ForEveryone=Install for everyone
hebrew.ForEveryone=התקנה לכל המשתמשים
brazilianportuguese.ForEveryone=Instalar para todos
dutch.ForEveryone=Voor iedereen installeren
german.ForEveryone=Für alle installieren
czech.ForEveryone=Instalovat pro všechny
swedish.ForEveryone=Installera för alla
portuguese.ForEveryone=Instalar para todos
french.ForEveryone=Installer pour tous
korean.ForEveryone=모든 사용자용으로 설치
japanese.ForEveryone=すべてのユーザーにインストール
italian.ForEveryone=Installa per tutti
spanish.ForEveryone=Instalar para todos
polish.ForEveryone=Zainstaluj dla wszystkich
ukrainian.ForEveryone=Встановити для всіх
russian.ForEveryone=Установить для всех
arabic.ForEveryone=التثبيت لكل المستخدمين
turkish.ForEveryone=Herkes için kur
english.ForAll=For everyone on this PC.
hebrew.ForAll=לכל המשתמשים במחשב.
brazilianportuguese.ForAll=Para todos neste computador.
dutch.ForAll=Voor iedereen op deze pc.
german.ForAll=Für alle auf diesem PC.
czech.ForAll=Pro všechny na tomto počítači.
swedish.ForAll=För alla på den här datorn.
portuguese.ForAll=Para todos neste PC.
french.ForAll=Pour tous les utilisateurs de ce PC.
korean.ForAll=이 PC의 모든 사용자용입니다.
japanese.ForAll=このPCのすべてのユーザーが対象です。
italian.ForAll=Per tutti su questo PC.
spanish.ForAll=Para todos en este equipo.
polish.ForAll=Dla wszystkich na tym komputerze.
ukrainian.ForAll=Для всіх користувачів цього ПК.
russian.ForAll=Для всех пользователей этого ПК.
arabic.ForAll=لكل المستخدمين على هذا الكمبيوتر.
turkish.ForAll=Bu bilgisayardaki herkes için.
english.UpdatingHere=Updates the copy in this folder.
hebrew.UpdatingHere=מעדכן את העותק שבתיקייה הזו.
brazilianportuguese.UpdatingHere=Atualiza a cópia que está nesta pasta.
dutch.UpdatingHere=Werkt de kopie in deze map bij.
german.UpdatingHere=Aktualisiert die Kopie in diesem Ordner.
czech.UpdatingHere=Aktualizuje kopii v této složce.
swedish.UpdatingHere=Uppdaterar kopian i den här mappen.
portuguese.UpdatingHere=Atualiza a cópia nesta pasta.
french.UpdatingHere=Met à jour la copie de ce dossier.
korean.UpdatingHere=이 폴더에 있는 사본을 업데이트합니다.
japanese.UpdatingHere=このフォルダー内のコピーを更新します。
italian.UpdatingHere=Aggiorna la copia in questa cartella.
spanish.UpdatingHere=Actualiza la copia de esta carpeta.
polish.UpdatingHere=Aktualizuje kopię w tym folderze.
ukrainian.UpdatingHere=Оновлює копію в цій теці.
russian.UpdatingHere=Обновляет копию в этой папке.
arabic.UpdatingHere=يحدّث النسخة الموجودة في هذا المجلد.
turkish.UpdatingHere=Bu klasördeki kopyayı günceller.
english.OpenVideo=Open videos with {#AppName}
hebrew.OpenVideo=פתיחת סרטונים ב-{#AppName}
brazilianportuguese.OpenVideo=Abrir vídeos com o {#AppName}
dutch.OpenVideo=Video's openen met {#AppName}
german.OpenVideo=Videos mit {#AppName} öffnen
czech.OpenVideo=Otevírat videa ve {#AppName}
swedish.OpenVideo=Öppna videor med {#AppName}
portuguese.OpenVideo=Abrir vídeos com o {#AppName}
french.OpenVideo=Ouvrir les vidéos avec {#AppName}
korean.OpenVideo={#AppName}으로 비디오 열기
japanese.OpenVideo={#AppName}でビデオを開く
italian.OpenVideo=Apri i video con {#AppName}
spanish.OpenVideo=Abrir vídeos con {#AppName}
polish.OpenVideo=Otwieraj wideo w {#AppName}
ukrainian.OpenVideo=Відкривати відео в {#AppName}
russian.OpenVideo=Открывать видео в {#AppName}
arabic.OpenVideo=فتح الفيديو باستخدام {#AppName}
turkish.OpenVideo=Videolar {#AppName} ile açılsın
english.OpenAudio=Open music with {#AppName}
hebrew.OpenAudio=פתיחת מוזיקה ב-{#AppName}
brazilianportuguese.OpenAudio=Abrir músicas com o {#AppName}
dutch.OpenAudio=Muziek openen met {#AppName}
german.OpenAudio=Musik mit {#AppName} öffnen
czech.OpenAudio=Otevírat hudbu ve {#AppName}
swedish.OpenAudio=Öppna musik med {#AppName}
portuguese.OpenAudio=Abrir música com o {#AppName}
french.OpenAudio=Ouvrir la musique avec {#AppName}
korean.OpenAudio={#AppName}으로 음악 열기
japanese.OpenAudio={#AppName}で音楽を開く
italian.OpenAudio=Apri la musica con {#AppName}
spanish.OpenAudio=Abrir música con {#AppName}
polish.OpenAudio=Otwieraj muzykę w {#AppName}
ukrainian.OpenAudio=Відкривати музику в {#AppName}
russian.OpenAudio=Открывать музыку в {#AppName}
arabic.OpenAudio=فتح الموسيقى باستخدام {#AppName}
turkish.OpenAudio=Müzikler {#AppName} ile açılsın
english.OpenPlaylists=Open playlists with {#AppName}
hebrew.OpenPlaylists=פתיחת רשימות השמעה ב-{#AppName}
brazilianportuguese.OpenPlaylists=Abrir playlists com o {#AppName}
dutch.OpenPlaylists=Afspeellijsten openen met {#AppName}
german.OpenPlaylists=Wiedergabelisten mit {#AppName} öffnen
czech.OpenPlaylists=Otevírat seznamy skladeb ve {#AppName}
swedish.OpenPlaylists=Öppna spellistor med {#AppName}
portuguese.OpenPlaylists=Abrir listas de reprodução com o {#AppName}
french.OpenPlaylists=Ouvrir les listes de lecture avec {#AppName}
korean.OpenPlaylists={#AppName}으로 재생목록 열기
japanese.OpenPlaylists={#AppName}でプレイリストを開く
italian.OpenPlaylists=Apri le scalette con {#AppName}
spanish.OpenPlaylists=Abrir listas de reproducción con {#AppName}
polish.OpenPlaylists=Otwieraj listy odtwarzania w {#AppName}
ukrainian.OpenPlaylists=Відкривати списки відтворення в {#AppName}
russian.OpenPlaylists=Открывать плейлисты в {#AppName}
arabic.OpenPlaylists=فتح قوائم التشغيل باستخدام {#AppName}
turkish.OpenPlaylists=Oynatma listeleri {#AppName} ile açılsın
english.ContextMenu=Play with {#AppName} in right-click menus
hebrew.ContextMenu="הפעלה ב-{#AppName}" בתפריט הקליק הימני
brazilianportuguese.ContextMenu={#AppName} no menu do clique direito
dutch.ContextMenu=Afspelen met {#AppName} in contextmenu's
german.ContextMenu=„Mit {#AppName} wiedergeben“ im Kontextmenü
czech.ContextMenu=Přehrát pomocí {#AppName} v místních nabídkách
swedish.ContextMenu=Spela upp med {#AppName} i högerklicksmenyer
portuguese.ContextMenu=Reproduzir com o {#AppName} no menu de contexto
french.ContextMenu=Lire avec {#AppName} dans les menus contextuels
korean.ContextMenu=오른쪽 클릭 메뉴에 {#AppName}으로 재생 추가
japanese.ContextMenu=右クリックメニューに「{#AppName}で再生」を追加
italian.ContextMenu=Riproduci con {#AppName} nel menu del tasto destro
spanish.ContextMenu=Reproducir con {#AppName} en el menú contextual
polish.ContextMenu=„Odtwórz w {#AppName}” w menu kontekstowym
ukrainian.ContextMenu=«Відтворити в {#AppName}» у контекстних меню
russian.ContextMenu=«Воспроизвести в {#AppName}» в контекстном меню
arabic.ContextMenu=«التشغيل باستخدام {#AppName}» في قائمة النقر الأيمن
turkish.ContextMenu=Sağ tık menülerinde {#AppName} ile oynat
english.DesktopIcon=Desktop shortcut
hebrew.DesktopIcon=קיצור דרך בשולחן העבודה
brazilianportuguese.DesktopIcon=Atalho na área de trabalho
dutch.DesktopIcon=Bureaubladsnelkoppeling
german.DesktopIcon=Desktopverknüpfung
czech.DesktopIcon=Zástupce na ploše
swedish.DesktopIcon=Skrivbordsgenväg
portuguese.DesktopIcon=Atalho no ambiente de trabalho
french.DesktopIcon=Raccourci sur le bureau
korean.DesktopIcon=바탕 화면 바로 가기
japanese.DesktopIcon=デスクトップのショートカット
italian.DesktopIcon=Collegamento sul desktop
spanish.DesktopIcon=Icono en el escritorio
polish.DesktopIcon=Skrót na pulpicie
ukrainian.DesktopIcon=Ярлик на робочому столі
russian.DesktopIcon=Ярлык на рабочем столе
arabic.DesktopIcon=اختصار على سطح المكتب
turkish.DesktopIcon=Masaüstü kısayolu
english.LayoutTitle=Layout
hebrew.LayoutTitle=פריסה
brazilianportuguese.LayoutTitle=Layout
dutch.LayoutTitle=Indeling
german.LayoutTitle=Layout
czech.LayoutTitle=Rozvržení
swedish.LayoutTitle=Layout
portuguese.LayoutTitle=Esquema
french.LayoutTitle=Disposition
korean.LayoutTitle=레이아웃
japanese.LayoutTitle=レイアウト
italian.LayoutTitle=Layout
spanish.LayoutTitle=Diseño
polish.LayoutTitle=Układ
ukrainian.LayoutTitle=Компонування
russian.LayoutTitle=Макет
arabic.LayoutTitle=التخطيط
turkish.LayoutTitle=Yerleşim
english.LayoutText=Left to right as designed, or mirrored.
hebrew.LayoutText=משמאל לימין כמו בעיצוב, או בתמונת ראי.
brazilianportuguese.LayoutText=Da esquerda para a direita, como no design, ou espelhado.
dutch.LayoutText=Van links naar rechts zoals ontworpen, of gespiegeld.
german.LayoutText=Von links nach rechts wie im Design oder gespiegelt.
czech.LayoutText=Zleva doprava podle návrhu, nebo zrcadlově.
swedish.LayoutText=Vänster till höger som i designen, eller spegelvänd.
portuguese.LayoutText=Da esquerda para a direita, como no design, ou espelhado.
french.LayoutText=De gauche à droite comme dans le design, ou en miroir.
korean.LayoutText=디자인대로 왼쪽에서 오른쪽으로, 또는 좌우 반전.
japanese.LayoutText=デザイン通りの左から右、または反転。
italian.LayoutText=Da sinistra a destra come nel design, o speculare.
spanish.LayoutText=De izquierda a derecha como en el diseño, o en espejo.
polish.LayoutText=Od lewej do prawej jak w projekcie lub odwrotnie.
ukrainian.LayoutText=Зліва направо, як у дизайні, або дзеркально.
russian.LayoutText=Слева направо, как в дизайне, или зеркально.
arabic.LayoutText=من اليسار إلى اليمين كما في التصميم، أو معكوسًا.
turkish.LayoutText=Tasarımdaki gibi soldan sağa ya da yansıtılmış.
english.LayoutRTL=Right-to-left layout
hebrew.LayoutRTL=פריסה מימין לשמאל
brazilianportuguese.LayoutRTL=Layout da direita para a esquerda
dutch.LayoutRTL=Indeling van rechts naar links
german.LayoutRTL=Layout von rechts nach links
czech.LayoutRTL=Rozvržení zprava doleva
swedish.LayoutRTL=Layout från höger till vänster
portuguese.LayoutRTL=Esquema da direita para a esquerda
french.LayoutRTL=Disposition de droite à gauche
korean.LayoutRTL=오른쪽에서 왼쪽 레이아웃
japanese.LayoutRTL=右から左のレイアウト
italian.LayoutRTL=Layout da destra a sinistra
spanish.LayoutRTL=Diseño de derecha a izquierda
polish.LayoutRTL=Układ od prawej do lewej
ukrainian.LayoutRTL=Компонування справа наліво
russian.LayoutRTL=Макет справа налево
arabic.LayoutRTL=تخطيط من اليمين إلى اليسار
turkish.LayoutRTL=Sağdan sola yerleşim
english.LayoutRTLText=The sidebar on the right and the controls in reverse order, for reading from right to left. Off: the design's layout, from left to right.
hebrew.LayoutRTLText=סרגל הצד מימין והפקדים בסדר הפוך, לקריאה מימין לשמאל. כבוי: הפריסה של העיצוב, משמאל לימין.
brazilianportuguese.LayoutRTLText=A barra lateral à direita e os controles em ordem inversa, para ler da direita para a esquerda. Desligado: o layout do design, da esquerda para a direita.
dutch.LayoutRTLText=De zijbalk rechts en de bediening in omgekeerde volgorde, om van rechts naar links te lezen. Uit: de indeling van het ontwerp, van links naar rechts.
german.LayoutRTLText=Die Seitenleiste rechts und die Steuerelemente in umgekehrter Reihenfolge, zum Lesen von rechts nach links. Aus: das Layout des Designs, von links nach rechts.
czech.LayoutRTLText=Postranní panel vpravo a ovládací prvky v obráceném pořadí, pro čtení zprava doleva. Vypnuto: rozvržení podle návrhu, zleva doprava.
swedish.LayoutRTLText=Sidofältet till höger och kontrollerna i omvänd ordning, för läsning från höger till vänster. Av: designens layout, från vänster till höger.
portuguese.LayoutRTLText=A barra lateral à direita e os controlos por ordem inversa, para ler da direita para a esquerda. Desligado: o esquema do design, da esquerda para a direita.
french.LayoutRTLText=La barre latérale à droite et les contrôles dans l'ordre inverse, pour lire de droite à gauche. Désactivé : la disposition du design, de gauche à droite.
korean.LayoutRTLText=사이드바를 오른쪽에 두고 컨트롤 순서를 반대로 하여 오른쪽에서 왼쪽으로 읽습니다. 끔: 디자인의 레이아웃(왼쪽에서 오른쪽).
japanese.LayoutRTLText=サイドバーを右側に、コントロールを逆順に配置し、右から左に読めるようにします。オフ: デザイン通りの左から右のレイアウト。
italian.LayoutRTLText=La barra laterale a destra e i controlli in ordine inverso, per leggere da destra a sinistra. Disattivato: il layout del design, da sinistra a destra.
spanish.LayoutRTLText=La barra lateral a la derecha y los controles en orden inverso, para leer de derecha a izquierda. Desactivado: el diseño original, de izquierda a derecha.
polish.LayoutRTLText=Pasek boczny po prawej i sterowanie w odwrotnej kolejności, do czytania od prawej do lewej. Wyłączone: układ z projektu, od lewej do prawej.
ukrainian.LayoutRTLText=Бічна панель праворуч, а елементи керування у зворотному порядку — для читання справа наліво. Вимкнено: компонування дизайну, зліва направо.
russian.LayoutRTLText=Боковая панель справа, элементы управления в обратном порядке — для чтения справа налево. Выкл.: макет дизайна, слева направо.
arabic.LayoutRTLText=الشريط الجانبي على اليمين وعناصر التحكم بترتيب معكوس، للقراءة من اليمين إلى اليسار. عند الإيقاف: تخطيط التصميم، من اليسار إلى اليمين.
turkish.LayoutRTLText=Sağdan sola okuma için kenar çubuğu sağda, denetimler ters sırada. Kapalı: tasarımın soldan sağa yerleşimi.
english.LayoutCardTitle=Change it any time
hebrew.LayoutCardTitle=אפשר לשנות בכל עת
brazilianportuguese.LayoutCardTitle=Dá para mudar quando quiser
dutch.LayoutCardTitle=Altijd te wijzigen
german.LayoutCardTitle=Jederzeit änderbar
czech.LayoutCardTitle=Můžete to kdykoli změnit
swedish.LayoutCardTitle=Ändra när du vill
portuguese.LayoutCardTitle=Pode mudar quando quiser
french.LayoutCardTitle=Modifiable à tout moment
korean.LayoutCardTitle=언제든지 변경 가능
japanese.LayoutCardTitle=いつでも変更できます
italian.LayoutCardTitle=Puoi cambiarlo quando vuoi
spanish.LayoutCardTitle=Cámbielo cuando quiera
polish.LayoutCardTitle=Można to zmienić
ukrainian.LayoutCardTitle=Змінюйте будь-коли
russian.LayoutCardTitle=Можно изменить в любой момент
arabic.LayoutCardTitle=يمكنك تغييره في أي وقت
turkish.LayoutCardTitle=İstediğiniz zaman değiştirin
english.LayoutCardText=Settings → Appearance: Layout direction. The language of the app is there too, in Interface language.
hebrew.LayoutCardText=בהגדרות ← מראה: כיוון הפריסה. גם שפת האפליקציה נמצאת שם, בשפת הממשק.
brazilianportuguese.LayoutCardText=Configurações → Aparência: direção do layout. O idioma do app também fica lá, em idioma da interface.
dutch.LayoutCardText=Instellingen → Uiterlijk: Indelingsrichting. De taal van de app staat daar ook, bij Interfacetaal.
german.LayoutCardText=Einstellungen → Aussehen: Layoutrichtung. Dort finden Sie auch die Sprache der App, unter „Sprache der Oberfläche“.
czech.LayoutCardText=Nastavení → Vzhled: Směr rozvržení. Najdete tam i jazyk aplikace, v položce Jazyk rozhraní.
swedish.LayoutCardText=Inställningar → Utseende: Layoutriktning. Appens språk finns där också, under Gränssnittsspråk.
portuguese.LayoutCardText=Definições → Aparência: Direção do esquema. O idioma da aplicação também está lá, em Idioma da interface.
french.LayoutCardText=Paramètres → Apparence : Sens de la disposition. La langue de l'application s'y trouve aussi, dans Langue de l'interface.
korean.LayoutCardText=설정 → 모양: 레이아웃 방향. 앱 언어도 같은 곳의 인터페이스 언어에 있습니다.
japanese.LayoutCardText=設定 → 外観: レイアウトの方向。アプリの言語も同じ場所の「インターフェースの言語」で変更できます。
italian.LayoutCardText=Impostazioni → Aspetto: Direzione del layout. Lì c'è anche la lingua dell'app, in Lingua dell'interfaccia.
spanish.LayoutCardText=Preferencias → Apariencia: Dirección del diseño. El idioma de la aplicación también está ahí, en Idioma de la interfaz.
polish.LayoutCardText=Ustawienia → Wygląd: Kierunek układu. Jest tam też język aplikacji, w opcji Język interfejsu.
ukrainian.LayoutCardText=Параметри → Зовнішній вигляд: Напрямок компонування. Там же й мова програми — у пункті «Мова інтерфейсу».
russian.LayoutCardText=Настройки → Внешний вид: Направление интерфейса. Там же и язык приложения — в пункте «Язык интерфейса».
arabic.LayoutCardText=الإعدادات ← المظهر: اتجاه التخطيط. وتجد لغة التطبيق هناك أيضًا، في لغة الواجهة.
turkish.LayoutCardText=Ayarlar → Görünüm: Yerleşim yönü. Uygulamanın dili de oradadır, Arayüz dili bölümünde.
english.PrivacyTitle=Privacy
hebrew.PrivacyTitle=פרטיות
brazilianportuguese.PrivacyTitle=Privacidade
dutch.PrivacyTitle=Privacy
german.PrivacyTitle=Datenschutz
czech.PrivacyTitle=Soukromí
swedish.PrivacyTitle=Sekretess
portuguese.PrivacyTitle=Privacidade
french.PrivacyTitle=Confidentialité
korean.PrivacyTitle=개인정보
japanese.PrivacyTitle=プライバシー
italian.PrivacyTitle=Privacy
spanish.PrivacyTitle=Privacidad
polish.PrivacyTitle=Prywatność
ukrainian.PrivacyTitle=Приватність
russian.PrivacyTitle=Конфиденциальность
arabic.PrivacyTitle=الخصوصية
turkish.PrivacyTitle=Gizlilik
english.PrivacyText=Two features go online by themselves. Each sends only this:
hebrew.PrivacyText=שתי תכונות מתחברות לאינטרנט מעצמן. כל אחת שולחת רק את זה:
brazilianportuguese.PrivacyText=Dois recursos acessam a internet por conta própria. Cada um envia só isto:
dutch.PrivacyText=Twee functies gaan zelf online. Elk verzendt alleen dit:
german.PrivacyText=Zwei Funktionen gehen von selbst online. Jede sendet nur Folgendes:
czech.PrivacyText=Dvě funkce se připojují k internetu samy. Každá odesílá pouze toto:
swedish.PrivacyText=Två funktioner går online själva. Var och en skickar bara detta:
portuguese.PrivacyText=Duas funcionalidades ligam-se à Internet sozinhas. Cada uma envia apenas isto:
french.PrivacyText=Deux fonctions se connectent d'elles-mêmes à Internet. Chacune n'envoie que ceci :
korean.PrivacyText=두 기능이 스스로 인터넷에 연결합니다. 각각 다음 정보만 보냅니다:
japanese.PrivacyText=2つの機能が自動的にインターネットに接続します。それぞれが送信するのは次の情報だけです:
italian.PrivacyText=Due funzioni si connettono a Internet da sole. Ognuna invia solo questo:
spanish.PrivacyText=Dos funciones se conectan a internet por sí solas. Cada una envía solo esto:
polish.PrivacyText=Dwie funkcje łączą się z internetem same. Każda wysyła tylko to:
ukrainian.PrivacyText=Дві функції самі виходять в Інтернет. Кожна надсилає лише це:
russian.PrivacyText=Две функции выходят в Интернет сами. Каждая отправляет только это:
arabic.PrivacyText=ميزتان تتصلان بالإنترنت من تلقاء نفسيهما، ولا ترسل كل منهما إلا ما يلي:
turkish.PrivacyText=İki özellik kendiliğinden internete bağlanır. Her biri yalnızca şunu gönderir:
english.PrivacyUpdates=Check for updates once a day
hebrew.PrivacyUpdates=בדיקת עדכונים פעם ביום
brazilianportuguese.PrivacyUpdates=Verificar atualizações uma vez por dia
dutch.PrivacyUpdates=Dagelijks controleren op updates
german.PrivacyUpdates=Einmal täglich nach Aktualisierungen suchen
czech.PrivacyUpdates=Kontrolovat aktualizace jednou denně
swedish.PrivacyUpdates=Sök efter uppdateringar en gång om dagen
portuguese.PrivacyUpdates=Procurar atualizações uma vez por dia
french.PrivacyUpdates=Rechercher des mises à jour une fois par jour
korean.PrivacyUpdates=하루에 한 번 업데이트 확인
japanese.PrivacyUpdates=1日1回アップデートを確認
italian.PrivacyUpdates=Controlla aggiornamenti una volta al giorno
spanish.PrivacyUpdates=Buscar actualizaciones una vez al día
polish.PrivacyUpdates=Sprawdzaj aktualizacje raz dziennie
ukrainian.PrivacyUpdates=Перевіряти оновлення раз на день
russian.PrivacyUpdates=Проверять обновления раз в день
arabic.PrivacyUpdates=التحقق من وجود تحديثات مرة يوميًا
turkish.PrivacyUpdates=Güncellemeleri günde bir kez denetle
english.PrivacyUpdatesText=A request for the latest release, to GitHub (api.github.com).
hebrew.PrivacyUpdatesText=בקשה לגרסה האחרונה, אל GitHub (api.github.com).
brazilianportuguese.PrivacyUpdatesText=Um pedido da versão mais recente, ao GitHub (api.github.com).
dutch.PrivacyUpdatesText=Een verzoek om de nieuwste release, naar GitHub (api.github.com).
german.PrivacyUpdatesText=Eine Anfrage nach der neuesten Version an GitHub (api.github.com).
czech.PrivacyUpdatesText=Dotaz na nejnovější vydání, odeslaný na GitHub (api.github.com).
swedish.PrivacyUpdatesText=En begäran om den senaste versionen, till GitHub (api.github.com).
portuguese.PrivacyUpdatesText=Um pedido da versão mais recente ao GitHub (api.github.com).
french.PrivacyUpdatesText=Une demande de la dernière version, à GitHub (api.github.com).
korean.PrivacyUpdatesText=GitHub(api.github.com)에 최신 릴리스를 요청합니다.
japanese.PrivacyUpdatesText=最新リリースの問い合わせを GitHub (api.github.com) に送信。
italian.PrivacyUpdatesText=Una richiesta dell'ultima versione, a GitHub (api.github.com).
spanish.PrivacyUpdatesText=Una solicitud de la última versión a GitHub (api.github.com).
polish.PrivacyUpdatesText=Zapytanie o najnowsze wydanie, do GitHub (api.github.com).
ukrainian.PrivacyUpdatesText=Запит останнього випуску до GitHub (api.github.com).
russian.PrivacyUpdatesText=Запрос последнего выпуска к GitHub (api.github.com).
arabic.PrivacyUpdatesText=طلب لأحدث إصدار، إلى GitHub (api.github.com)‎.
turkish.PrivacyUpdatesText=GitHub'a (api.github.com) en son sürüm için bir istek.
english.PrivacySubtitles=Find subtitles online automatically
hebrew.PrivacySubtitles=חיפוש כתוביות ברשת באופן אוטומטי
brazilianportuguese.PrivacySubtitles=Buscar legendas on-line automaticamente
dutch.PrivacySubtitles=Automatisch online ondertitels zoeken
german.PrivacySubtitles=Untertitel automatisch online suchen
czech.PrivacySubtitles=Automaticky hledat titulky online
swedish.PrivacySubtitles=Hitta undertexter online automatiskt
portuguese.PrivacySubtitles=Procurar legendas online automaticamente
french.PrivacySubtitles=Trouver automatiquement des sous-titres en ligne
korean.PrivacySubtitles=온라인에서 자막 자동으로 찾기
japanese.PrivacySubtitles=オンラインで字幕を自動的に検索
italian.PrivacySubtitles=Cerca sottotitoli online automaticamente
spanish.PrivacySubtitles=Buscar subtítulos en línea automáticamente
polish.PrivacySubtitles=Automatycznie szukaj napisów w internecie
ukrainian.PrivacySubtitles=Автоматично шукати субтитри в Інтернеті
russian.PrivacySubtitles=Автоматически искать субтитры в Интернете
arabic.PrivacySubtitles=البحث عن الترجمات عبر الإنترنت تلقائيًا
turkish.PrivacySubtitles=Altyazıları çevrim içi otomatik bul
english.PrivacySubtitlesText=The film's name and a fingerprint of the file, to subtitle services (OpenSubtitles.org, Wizdom, Gestdown, YIFY); its name to IMDb, Wikidata and Cinemeta.
hebrew.PrivacySubtitlesText=שם הסרט וטביעת אצבע של הקובץ, לשירותי הכתוביות OpenSubtitles.org, Wizdom, Gestdown ו-YIFY. השם גם ל-IMDb, ל-Wikidata ול-Cinemeta.
brazilianportuguese.PrivacySubtitlesText=O nome do filme e uma impressão digital do arquivo, para serviços de legendas (OpenSubtitles.org, Wizdom, Gestdown, YIFY); o nome também para IMDb, Wikidata e Cinemeta.
dutch.PrivacySubtitlesText=De naam van de film en een vingerafdruk van het bestand, naar ondertiteldiensten (OpenSubtitles.org, Wizdom, Gestdown, YIFY); de naam naar IMDb, Wikidata en Cinemeta.
german.PrivacySubtitlesText=Der Name des Films und ein Fingerabdruck der Datei an Untertiteldienste (OpenSubtitles.org, Wizdom, Gestdown, YIFY); sein Name an IMDb, Wikidata und Cinemeta.
czech.PrivacySubtitlesText=Název filmu a otisk souboru titulkovým službám (OpenSubtitles.org, Wizdom, Gestdown, YIFY); název také do IMDb, Wikidata a Cinemeta.
swedish.PrivacySubtitlesText=Filmens namn och ett fingeravtryck av filen, till undertexttjänster (OpenSubtitles.org, Wizdom, Gestdown, YIFY); namnet till IMDb, Wikidata och Cinemeta.
portuguese.PrivacySubtitlesText=O nome do filme e uma impressão digital do ficheiro aos serviços de legendas (OpenSubtitles.org, Wizdom, Gestdown, YIFY); o nome ao IMDb, Wikidata e Cinemeta.
french.PrivacySubtitlesText=Le nom du film et une empreinte du fichier, aux services de sous-titres (OpenSubtitles.org, Wizdom, Gestdown, YIFY) ; son nom à IMDb, Wikidata et Cinemeta.
korean.PrivacySubtitlesText=자막 서비스(OpenSubtitles.org, Wizdom, Gestdown, YIFY)에는 영화 이름과 파일 지문을, IMDb, Wikidata, Cinemeta에는 이름을 보냅니다.
japanese.PrivacySubtitlesText=映画の名前とファイルのフィンガープリントを字幕サービス (OpenSubtitles.org、Wizdom、Gestdown、YIFY) に、名前をIMDb、Wikidata、Cinemetaに送信。
italian.PrivacySubtitlesText=Il nome del film e un'impronta del file, ai servizi di sottotitoli (OpenSubtitles.org, Wizdom, Gestdown, YIFY); il nome a IMDb, Wikidata e Cinemeta.
spanish.PrivacySubtitlesText=El nombre de la película y una huella del archivo, a los servicios de subtítulos (OpenSubtitles.org, Wizdom, Gestdown, YIFY); su nombre, a IMDb, Wikidata y Cinemeta.
polish.PrivacySubtitlesText=Nazwa filmu i odcisk pliku do serwisów z napisami (OpenSubtitles.org, Wizdom, Gestdown, YIFY); nazwa także do IMDb, Wikidata i Cinemeta.
ukrainian.PrivacySubtitlesText=Назва фільму й відбиток файлу — сервісам субтитрів (OpenSubtitles.org, Wizdom, Gestdown, YIFY); назва — IMDb, Wikidata і Cinemeta.
russian.PrivacySubtitlesText=Название фильма и отпечаток файла — сервисам субтитров (OpenSubtitles.org, Wizdom, Gestdown, YIFY); название — в IMDb, Wikidata и Cinemeta.
arabic.PrivacySubtitlesText=اسم الفيلم وبصمة الملف، إلى خدمات الترجمة (OpenSubtitles.org وWizdom وGestdown وYIFY)؛ واسمه إلى IMDb وWikidata وCinemeta.
turkish.PrivacySubtitlesText=Filmin adı ve dosyanın parmak izi altyazı hizmetlerine (OpenSubtitles.org, Wizdom, Gestdown, YIFY); adı IMDb, Wikidata ve Cinemeta'ya.
english.PrivacyCardTitle=No telemetry, no ads, no account
hebrew.PrivacyCardTitle=בלי טלמטריה, בלי פרסומות, בלי חשבון
brazilianportuguese.PrivacyCardTitle=Sem telemetria, sem anúncios, sem conta
dutch.PrivacyCardTitle=Geen telemetrie, geen reclame, geen account
german.PrivacyCardTitle=Keine Telemetrie, keine Werbung, kein Konto
czech.PrivacyCardTitle=Žádná telemetrie, žádné reklamy, žádný účet
swedish.PrivacyCardTitle=Ingen telemetri, ingen reklam, inget konto
portuguese.PrivacyCardTitle=Sem telemetria, sem anúncios, sem conta
french.PrivacyCardTitle=Aucune télémétrie, aucune publicité, aucun compte
korean.PrivacyCardTitle=원격 분석, 광고, 계정 없음
japanese.PrivacyCardTitle=テレメトリー、広告、アカウントなし
italian.PrivacyCardTitle=Niente telemetria, pubblicità o account
spanish.PrivacyCardTitle=Sin telemetría, sin anuncios, sin cuenta
polish.PrivacyCardTitle=Bez telemetrii, reklam i konta
ukrainian.PrivacyCardTitle=Жодної телеметрії, реклами чи облікового запису
russian.PrivacyCardTitle=Без телеметрии, рекламы и учётной записи
arabic.PrivacyCardTitle=لا قياس عن بُعد، ولا إعلانات، ولا حساب
turkish.PrivacyCardTitle=Telemetri yok, reklam yok, hesap yok
english.PrivacyCardText=Nothing else goes online unless you ask for it. Change both later in Settings.
hebrew.PrivacyCardText=שום דבר אחר לא יוצא לאינטרנט בלי שתבקשו. אפשר לשנות את שתיהן אחר כך בהגדרות.
brazilianportuguese.PrivacyCardText=Nada mais acessa a internet sem você pedir. Altere os dois depois nas Configurações.
dutch.PrivacyCardText=Niets anders gaat online tenzij u erom vraagt. Wijzig beide later in Instellingen.
german.PrivacyCardText=Sonst geht nichts online, außer Sie verlangen es. Beides lässt sich später in den Einstellungen ändern.
czech.PrivacyCardText=Nic dalšího se k internetu nepřipojí, pokud o to nepožádáte. Obojí můžete později změnit v Nastavení.
swedish.PrivacyCardText=Inget annat går ut på internet om du inte ber om det. Ändra båda senare i Inställningar.
portuguese.PrivacyCardText=Mais nada se liga à Internet sem o pedir. Altere ambas mais tarde nas Definições.
french.PrivacyCardText=Rien d'autre ne se connecte sans que vous le demandiez. Modifiez les deux plus tard dans les Paramètres.
korean.PrivacyCardText=요청하지 않으면 그 밖의 어떤 것도 인터넷에 연결하지 않습니다. 두 기능 모두 나중에 설정에서 바꿀 수 있습니다.
japanese.PrivacyCardText=求めない限り、ほかには何もインターネットに送信されません。どちらも後で設定から変更できます。
italian.PrivacyCardText=Nient'altro va online se non lo chiedi tu. Puoi cambiare entrambe in seguito nelle Impostazioni.
spanish.PrivacyCardText=Nada más se conecta a internet a menos que usted lo pida. Puede cambiar ambas después en Preferencias.
polish.PrivacyCardText=Nic innego nie łączy się z internetem bez Twojej zgody. Obie zmienisz później w Ustawieniach.
ukrainian.PrivacyCardText=Ніщо інше не виходить в Інтернет без вашого запиту. Обидві функції можна змінити пізніше в параметрах.
russian.PrivacyCardText=Больше ничего не выходит в Интернет без вашего запроса. Обе функции можно изменить позже в настройках.
arabic.PrivacyCardText=لا شيء آخر يتصل بالإنترنت ما لم تطلب ذلك. يمكنك تغيير الخيارين لاحقًا في الإعدادات.
turkish.PrivacyCardText=Siz istemedikçe başka hiçbir şey internete bağlanmaz. İkisini de daha sonra Ayarlar'dan değiştirebilirsiniz.
english.LinkPrivacy=Privacy and code signing policy
hebrew.LinkPrivacy=מדיניות הפרטיות וחתימת הקוד
brazilianportuguese.LinkPrivacy=Política de privacidade e de assinatura de código
dutch.LinkPrivacy=Privacy- en codeondertekeningsbeleid
german.LinkPrivacy=Datenschutz- und Codesignaturrichtlinie
czech.LinkPrivacy=Ochrana soukromí a podepisování kódu
swedish.LinkPrivacy=Policy för integritet och kodsignering
portuguese.LinkPrivacy=Política de privacidade e de assinatura de código
french.LinkPrivacy=Politique de confidentialité et de signature du code
korean.LinkPrivacy=개인정보 및 코드 서명 정책
japanese.LinkPrivacy=プライバシーとコード署名のポリシー
italian.LinkPrivacy=Privacy e firma del codice
spanish.LinkPrivacy=Política de privacidad y de firma de código
polish.LinkPrivacy=Zasady prywatności i podpisywania kodu
ukrainian.LinkPrivacy=Політика конфіденційності та підписування коду
russian.LinkPrivacy=Политика конфиденциальности и подписи кода
arabic.LinkPrivacy=سياسة الخصوصية وتوقيع الشفرة
turkish.LinkPrivacy=Gizlilik ve kod imzalama ilkesi
english.InstallingTitle=Installing
hebrew.InstallingTitle=מתקין
brazilianportuguese.InstallingTitle=Instalando
dutch.InstallingTitle=Installeren
german.InstallingTitle=Installation läuft
czech.InstallingTitle=Instalace
swedish.InstallingTitle=Installerar
portuguese.InstallingTitle=A instalar
french.InstallingTitle=Installation
korean.InstallingTitle=설치 중
japanese.InstallingTitle=インストールしています
italian.InstallingTitle=Installazione
spanish.InstallingTitle=Instalando
polish.InstallingTitle=Instalowanie
ukrainian.InstallingTitle=Встановлення
russian.InstallingTitle=Установка
arabic.InstallingTitle=جارٍ التثبيت
turkish.InstallingTitle=Kuruluyor
english.InstallingText=This takes about a minute.
hebrew.InstallingText=זה לוקח כדקה.
brazilianportuguese.InstallingText=Isso leva cerca de um minuto.
dutch.InstallingText=Dit duurt ongeveer een minuut.
german.InstallingText=Das dauert etwa eine Minute.
czech.InstallingText=Potrvá to zhruba minutu.
swedish.InstallingText=Det tar ungefär en minut.
portuguese.InstallingText=Isto demora cerca de um minuto.
french.InstallingText=Cela prend environ une minute.
korean.InstallingText=1분 정도 걸립니다.
japanese.InstallingText=1分ほどかかります。
italian.InstallingText=Richiede circa un minuto.
spanish.InstallingText=Esto tarda alrededor de un minuto.
polish.InstallingText=To potrwa około minuty.
ukrainian.InstallingText=Це триває близько хвилини.
russian.InstallingText=Это займёт около минуты.
arabic.InstallingText=يستغرق هذا نحو دقيقة.
turkish.InstallingText=Bu işlem yaklaşık bir dakika sürer.
english.TipTitle=Make the player yours
hebrew.TipTitle=התאימו את הנגן לעצמכם
brazilianportuguese.TipTitle=Deixe o player do seu jeito
dutch.TipTitle=Maak de speler persoonlijk
german.TipTitle=Gestalten Sie Ihren Player
czech.TipTitle=Přizpůsobte si přehrávač
swedish.TipTitle=Gör spelaren till din
portuguese.TipTitle=Personalize o leitor
french.TipTitle=Personnalisez votre lecteur
korean.TipTitle=나만의 플레이어 만들기
japanese.TipTitle=プレイヤーを自分好みに
italian.TipTitle=Personalizza il lettore
spanish.TipTitle=Haga suyo el reproductor
polish.TipTitle=Dopasuj odtwarzacz
ukrainian.TipTitle=Налаштуйте програвач під себе
russian.TipTitle=Настройте плеер под себя
arabic.TipTitle=اجعل المشغل على ذوقك
turkish.TipTitle=Oynatıcıyı kişiselleştirin
english.TipText=Drag buttons into and out of the control bar with ⋯ → Customize Player, and keep your layouts as profiles.
hebrew.TipText=גררו כפתורים אל סרגל השליטה וממנו דרך ⋯ ← התאמת הנגן, ושמרו את הפריסות שלכם כפרופילים.
brazilianportuguese.TipText=Arraste botões para dentro e para fora da barra de controle em ⋯ → Personalizar o player, e guarde seus layouts como perfis.
dutch.TipText=Sleep knoppen in en uit de bedieningsbalk via ⋯ → Speler aanpassen, en bewaar uw indelingen als profielen.
german.TipText=Ziehen Sie Schaltflächen mit ⋯ → Player anpassen in die Steuerleiste hinein und heraus, und speichern Sie Ihre Layouts als Profile.
czech.TipText=Přetahujte tlačítka na ovládací panel a z něj přes ⋯ → Přizpůsobit přehrávač a uchovejte svá rozvržení jako profily.
swedish.TipText=Dra knappar till och från kontrollfältet med ⋯ → Anpassa spelaren, och spara dina layouter som profiler.
portuguese.TipText=Arraste botões para dentro e para fora da barra de controlo com ⋯ → Personalizar o leitor, e guarde os seus esquemas como perfis.
french.TipText=Faites glisser des boutons dans la barre de contrôle et hors de celle-ci avec ⋯ → Personnaliser le lecteur, et gardez vos dispositions en profils.
korean.TipText=⋯ → 플레이어 사용자 지정에서 버튼을 컨트롤 막대로 끌어 넣거나 빼고, 레이아웃을 프로필로 저장하세요.
japanese.TipText=⋯ → 「プレイヤーをカスタマイズ」で、コントロールバーにボタンをドラッグして出し入れでき、レイアウトをプロファイルとして保存できます。
italian.TipText=Trascina i pulsanti dentro e fuori dalla barra dei controlli con ⋯ → Personalizza lettore, e salva i tuoi layout come profili.
spanish.TipText=Arrastre botones dentro y fuera de la barra de controles con ⋯ → Personalizar reproductor, y guarde sus diseños como perfiles.
polish.TipText=Przeciągaj przyciski na pasek sterowania i z niego w ⋯ → Dostosuj odtwarzacz, a swoje układy zapisuj jako profile.
ukrainian.TipText=Перетягуйте кнопки на панель керування й з неї через ⋯ → Налаштувати програвач і зберігайте свої компонування як профілі.
russian.TipText=Перетаскивайте кнопки на панель управления и с неё через ⋯ → Настроить плеер и сохраняйте свои макеты как профили.
arabic.TipText=اسحب الأزرار إلى شريط التحكم ومنه عبر ⋯ ← تخصيص المشغل، واحفظ تخطيطاتك كملفات تعريف.
turkish.TipText=⋯ → Oynatıcıyı Özelleştir ile düğmeleri denetim çubuğuna sürükleyip çıkarın ve yerleşimlerinizi profil olarak saklayın.
english.DoneTitle={#AppName} is ready
hebrew.DoneTitle={#AppName} מוכן
brazilianportuguese.DoneTitle=O {#AppName} está pronto
dutch.DoneTitle={#AppName} is klaar
german.DoneTitle={#AppName} ist bereit
czech.DoneTitle={#AppName} je připraven
swedish.DoneTitle={#AppName} är klar
portuguese.DoneTitle=O {#AppName} está pronto
french.DoneTitle={#AppName} est prêt
korean.DoneTitle={#AppName} 준비 완료
japanese.DoneTitle={#AppName}の準備ができました
italian.DoneTitle={#AppName} è pronto
spanish.DoneTitle={#AppName} está listo
polish.DoneTitle={#AppName} jest gotowy
ukrainian.DoneTitle={#AppName} готовий
russian.DoneTitle={#AppName} готов
arabic.DoneTitle=أصبح {#AppName} جاهزًا
turkish.DoneTitle={#AppName} hazır
english.DoneText=Find it in the Start menu.
hebrew.DoneText=אפשר למצוא אותו בתפריט התחל.
brazilianportuguese.DoneText=Ele está no menu Iniciar.
dutch.DoneText=U vindt het in het menu Start.
german.DoneText=Sie finden es im Startmenü.
czech.DoneText=Najdete ho v nabídce Start.
swedish.DoneText=Du hittar den på Start-menyn.
portuguese.DoneText=Encontra-o no menu Iniciar.
french.DoneText=Retrouvez-le dans le menu Démarrer.
korean.DoneText=시작 메뉴에서 찾을 수 있습니다.
japanese.DoneText=スタートメニューから起動できます。
italian.DoneText=Lo trovi nel menu Start.
spanish.DoneText=Lo encontrará en el menú Inicio.
polish.DoneText=Znajdziesz go w menu Start.
ukrainian.DoneText=Його можна знайти в меню «Пуск».
russian.DoneText=Он есть в меню «Пуск».
arabic.DoneText=ستجده في قائمة ابدأ.
turkish.DoneText=Başlat menüsünde bulabilirsiniz.
english.DefaultTitle=Make it your default player
hebrew.DefaultTitle=הגדרה כנגן ברירת המחדל
brazilianportuguese.DefaultTitle=Torne-o seu player padrão
dutch.DefaultTitle=Maak het uw standaardspeler
german.DefaultTitle=Als Standardplayer festlegen
czech.DefaultTitle=Nastavte jako výchozí přehrávač
swedish.DefaultTitle=Gör den till din standardspelare
portuguese.DefaultTitle=Torne-o o seu leitor predefinido
french.DefaultTitle=Faites-en votre lecteur par défaut
korean.DefaultTitle=기본 플레이어로 설정
japanese.DefaultTitle=既定のプレイヤーに設定
italian.DefaultTitle=Rendilo il lettore predefinito
spanish.DefaultTitle=Úselo como reproductor predeterminado
polish.DefaultTitle=Ustaw go jako domyślny odtwarzacz
ukrainian.DefaultTitle=Зробіть його типовим програвачем
russian.DefaultTitle=Сделайте его плеером по умолчанию
arabic.DefaultTitle=اجعله مشغلك الافتراضي
turkish.DefaultTitle=Varsayılan oynatıcı olarak ayarlayın
english.DefaultText=Windows keeps the apps you chose before. Open Default apps and choose {#AppName} for your media.
hebrew.DefaultText=Windows שומרת את האפליקציות שבחרתם בעבר. פתחו את "אפליקציות ברירת מחדל" ובחרו ב-{#AppName} עבור קובצי המדיה.
brazilianportuguese.DefaultText=O Windows mantém suas escolhas. Abra Aplicativos padrão e escolha o {#AppName} para suas mídias.
dutch.DefaultText=Windows behoudt de apps die u eerder koos. Open Standaard-apps en kies {#AppName} voor uw media.
german.DefaultText=Windows behält die zuvor gewählten Apps. Öffnen Sie „Standard-Apps“ und wählen Sie {#AppName} für Ihre Medien.
czech.DefaultText=Windows si pamatuje dříve zvolené aplikace. Otevřete Výchozí aplikace a pro svá média zvolte {#AppName}.
swedish.DefaultText=Windows behåller apparna du valde tidigare. Öppna Standardappar och välj {#AppName} för dina media.
portuguese.DefaultText=O Windows mantém as aplicações que escolheu antes. Abra Aplicações predefinidas e escolha o {#AppName} para os seus ficheiros multimédia.
french.DefaultText=Windows conserve les applications choisies auparavant. Ouvrez Applications par défaut et choisissez {#AppName} pour vos médias.
korean.DefaultText=Windows는 이전에 선택한 앱을 유지합니다. 기본 앱을 열고 미디어에 {#AppName}을 선택하세요.
japanese.DefaultText=Windowsは以前に選んだアプリを維持します。「既定のアプリ」を開き、メディアに{#AppName}を選択してください。
italian.DefaultText=Windows mantiene le app che avevi scelto prima. Apri App predefinite e scegli {#AppName} per i tuoi media.
spanish.DefaultText=Windows mantiene sus elecciones anteriores. Abra Aplicaciones predeterminadas y elija {#AppName} para sus medios.
polish.DefaultText=Windows zachowuje wcześniejsze wybory. Otwórz Aplikacje domyślne i wybierz {#AppName} dla multimediów.
ukrainian.DefaultText=Windows зберігає програми, вибрані раніше. Відкрийте «Програми за замовчуванням» і виберіть {#AppName} для своїх медіа.
russian.DefaultText=Windows сохраняет выбранные ранее приложения. Откройте «Приложения по умолчанию» и выберите {#AppName} для своих медиафайлов.
arabic.DefaultText=يحتفظ Windows بالتطبيقات التي اخترتها من قبل. افتح «التطبيقات الافتراضية» واختر {#AppName} لوسائطك.
turkish.DefaultText=Windows önceden seçtiğiniz uygulamaları korur. Varsayılan uygulamalar ayarını açın ve ortamlarınız için {#AppName} uygulamasını seçin.
english.OpenDefaultApps=Open Default apps
hebrew.OpenDefaultApps=לאפליקציות ברירת מחדל
brazilianportuguese.OpenDefaultApps=Aplicativos padrão
dutch.OpenDefaultApps=Standaard-apps openen
german.OpenDefaultApps=Standard-Apps öffnen
czech.OpenDefaultApps=Otevřít Výchozí aplikace
swedish.OpenDefaultApps=Öppna Standardappar
portuguese.OpenDefaultApps=Aplicações predefinidas
french.OpenDefaultApps=Applications par défaut
korean.OpenDefaultApps=기본 앱 열기
japanese.OpenDefaultApps=既定のアプリを開く
italian.OpenDefaultApps=Apri App predefinite
spanish.OpenDefaultApps=Aplicaciones predeterminadas
polish.OpenDefaultApps=Aplikacje domyślne
ukrainian.OpenDefaultApps=Програми за замовчуванням
russian.OpenDefaultApps=Приложения по умолчанию
arabic.OpenDefaultApps=التطبيقات الافتراضية
turkish.OpenDefaultApps=Varsayılan uygulamaları aç
english.LaunchNow=Open {#AppName} now
hebrew.LaunchNow=פתיחת {#AppName} עכשיו
brazilianportuguese.LaunchNow=Abrir o {#AppName} agora
dutch.LaunchNow={#AppName} nu openen
german.LaunchNow={#AppName} jetzt öffnen
czech.LaunchNow=Spustit {#AppName} nyní
swedish.LaunchNow=Öppna {#AppName} nu
portuguese.LaunchNow=Abrir o {#AppName} agora
french.LaunchNow=Ouvrir {#AppName} maintenant
korean.LaunchNow=지금 {#AppName} 열기
japanese.LaunchNow=今すぐ{#AppName}を開く
italian.LaunchNow=Apri {#AppName} ora
spanish.LaunchNow=Abrir {#AppName} ahora
polish.LaunchNow=Otwórz {#AppName} teraz
ukrainian.LaunchNow=Відкрити {#AppName} зараз
russian.LaunchNow=Запустить {#AppName}
arabic.LaunchNow=فتح {#AppName} الآن
turkish.LaunchNow={#AppName} şimdi açılsın
english.DoneCredit=Enjoy the design by Juxtopposed.
hebrew.DoneCredit=תיהנו מהעיצוב של Juxtopposed.
brazilianportuguese.DoneCredit=Aproveite o design de Juxtopposed.
dutch.DoneCredit=Geniet van het ontwerp van Juxtopposed.
german.DoneCredit=Viel Freude mit dem Design von Juxtopposed.
czech.DoneCredit=Užijte si design od Juxtopposed.
swedish.DoneCredit=Njut av designen av Juxtopposed.
portuguese.DoneCredit=Desfrute do design de Juxtopposed.
french.DoneCredit=Profitez du design de Juxtopposed.
korean.DoneCredit=Juxtopposed의 디자인을 즐겨 보세요.
japanese.DoneCredit=Juxtopposedのデザインをお楽しみください。
italian.DoneCredit=Goditi il design di Juxtopposed.
spanish.DoneCredit=Disfrute del diseño de Juxtopposed.
polish.DoneCredit=Ciesz się projektem Juxtopposed.
ukrainian.DoneCredit=Насолоджуйтеся дизайном від Juxtopposed.
russian.DoneCredit=Наслаждайтесь дизайном от Juxtopposed.
arabic.DoneCredit=استمتع بتصميم Juxtopposed.
turkish.DoneCredit=Juxtopposed'un tasarımının keyfini çıkarın.
english.LinkSubscribe=Subscribe on YouTube
hebrew.LinkSubscribe=הרשמה לערוץ ביוטיוב
brazilianportuguese.LinkSubscribe=Inscreva-se no YouTube
dutch.LinkSubscribe=Abonneren op YouTube
german.LinkSubscribe=Auf YouTube abonnieren
czech.LinkSubscribe=Odebírat na YouTube
swedish.LinkSubscribe=Prenumerera på YouTube
portuguese.LinkSubscribe=Subscrever no YouTube
french.LinkSubscribe=S'abonner sur YouTube
korean.LinkSubscribe=YouTube에서 구독
japanese.LinkSubscribe=YouTubeでチャンネル登録
italian.LinkSubscribe=Iscriviti su YouTube
spanish.LinkSubscribe=Suscribirse en YouTube
polish.LinkSubscribe=Subskrybuj na YouTube
ukrainian.LinkSubscribe=Підписатися на YouTube
russian.LinkSubscribe=Подписаться на YouTube
arabic.LinkSubscribe=اشترك على YouTube
turkish.LinkSubscribe=YouTube'da abone olun
english.NoticeTitle=Notice and licenses
hebrew.NoticeTitle=הודעה ורישיונות
brazilianportuguese.NoticeTitle=Aviso e licenças
dutch.NoticeTitle=Kennisgeving en licenties
german.NoticeTitle=Hinweis und Lizenzen
czech.NoticeTitle=Upozornění a licence
swedish.NoticeTitle=Information och licenser
portuguese.NoticeTitle=Aviso e licenças
french.NoticeTitle=Avis et licences
korean.NoticeTitle=고지 및 라이선스
japanese.NoticeTitle=注意事項とライセンス
italian.NoticeTitle=Note e licenze
spanish.NoticeTitle=Aviso y licencias
polish.NoticeTitle=Informacje i licencje
ukrainian.NoticeTitle=Повідомлення й ліцензії
russian.NoticeTitle=Уведомление и лицензии
arabic.NoticeTitle=الإشعار والتراخيص
turkish.NoticeTitle=Bildirim ve lisanslar
english.Close=Close
hebrew.Close=סגירה
brazilianportuguese.Close=Fechar
dutch.Close=Sluiten
german.Close=Schließen
czech.Close=Zavřít
swedish.Close=Stäng
portuguese.Close=Fechar
french.Close=Fermer
korean.Close=닫기
japanese.Close=閉じる
italian.Close=Chiudi
spanish.Close=Cerrar
polish.Close=Zamknij
ukrainian.Close=Закрити
russian.Close=Закрыть
arabic.Close=إغلاق
turkish.Close=Kapat
english.BadDir=Choose a folder to install to.
hebrew.BadDir=בחרו תיקייה להתקנה.
brazilianportuguese.BadDir=Escolha uma pasta para a instalação.
dutch.BadDir=Kies een map om in te installeren.
german.BadDir=Wählen Sie einen Installationsordner.
czech.BadDir=Vyberte složku pro instalaci.
swedish.BadDir=Välj en mapp att installera i.
portuguese.BadDir=Escolha uma pasta para a instalação.
french.BadDir=Choisissez un dossier d'installation.
korean.BadDir=설치할 폴더를 선택하세요.
japanese.BadDir=インストール先のフォルダーを選択してください。
italian.BadDir=Scegli una cartella in cui installare.
spanish.BadDir=Elija una carpeta para la instalación.
polish.BadDir=Wybierz folder instalacji.
ukrainian.BadDir=Виберіть теку для встановлення.
russian.BadDir=Выберите папку для установки.
arabic.BadDir=اختر مجلدًا للتثبيت فيه.
turkish.BadDir=Kurulum için bir klasör seçin.
; The words on the toggles, as the app writes them (SettingsToggle.qml)
english.ToggleOn=ON
hebrew.ToggleOn=פעיל
brazilianportuguese.ToggleOn=LIG
dutch.ToggleOn=AAN
german.ToggleOn=EIN
czech.ToggleOn=ZAP
swedish.ToggleOn=PÅ
portuguese.ToggleOn=SIM
french.ToggleOn=OUI
korean.ToggleOn=켬
japanese.ToggleOn=オン
italian.ToggleOn=SÌ
spanish.ToggleOn=SÍ
polish.ToggleOn=WŁ.
ukrainian.ToggleOn=УВІМК.
russian.ToggleOn=ВКЛ
arabic.ToggleOn=تشغيل
turkish.ToggleOn=AÇIK
english.ToggleOff=OFF
hebrew.ToggleOff=כבוי
brazilianportuguese.ToggleOff=DESL
dutch.ToggleOff=UIT
german.ToggleOff=AUS
czech.ToggleOff=VYP
swedish.ToggleOff=AV
portuguese.ToggleOff=NÃO
french.ToggleOff=NON
korean.ToggleOff=끔
japanese.ToggleOff=オフ
italian.ToggleOff=NO
spanish.ToggleOff=NO
polish.ToggleOff=WYŁ.
ukrainian.ToggleOff=ВИМК.
russian.ToggleOff=ВЫКЛ
arabic.ToggleOff=إيقاف
turkish.ToggleOff=KAPALI
; Shown by Windows: the file types, their verbs, the right-click menus
english.TypeVideo=%1 video
hebrew.TypeVideo=וידאו %1
brazilianportuguese.TypeVideo=Vídeo %1
dutch.TypeVideo=%1-video
german.TypeVideo=%1-Video
czech.TypeVideo=Video %1
swedish.TypeVideo=%1-video
portuguese.TypeVideo=Vídeo %1
french.TypeVideo=Vidéo %1
korean.TypeVideo=%1 비디오
japanese.TypeVideo=%1 ビデオ
italian.TypeVideo=Video %1
spanish.TypeVideo=Vídeo %1
polish.TypeVideo=Plik wideo %1
ukrainian.TypeVideo=Відео %1
russian.TypeVideo=Видео %1
arabic.TypeVideo=فيديو %1
turkish.TypeVideo=%1 video dosyası
english.TypeAudio=%1 audio
hebrew.TypeAudio=שמע %1
brazilianportuguese.TypeAudio=Áudio %1
dutch.TypeAudio=%1-audio
german.TypeAudio=%1-Audio
czech.TypeAudio=Zvuk %1
swedish.TypeAudio=%1-ljud
portuguese.TypeAudio=Áudio %1
french.TypeAudio=Audio %1
korean.TypeAudio=%1 오디오
japanese.TypeAudio=%1 オーディオ
italian.TypeAudio=Audio %1
spanish.TypeAudio=Audio %1
polish.TypeAudio=Plik audio %1
ukrainian.TypeAudio=Аудіо %1
russian.TypeAudio=Аудио %1
arabic.TypeAudio=صوت %1
turkish.TypeAudio=%1 ses dosyası
english.TypePlaylist=%1 playlist
hebrew.TypePlaylist=רשימת השמעה %1
brazilianportuguese.TypePlaylist=Playlist %1
dutch.TypePlaylist=%1-afspeellijst
german.TypePlaylist=%1-Wiedergabeliste
czech.TypePlaylist=Seznam skladeb %1
swedish.TypePlaylist=%1-spellista
portuguese.TypePlaylist=Lista de reprodução %1
french.TypePlaylist=Liste de lecture %1
korean.TypePlaylist=%1 재생목록
japanese.TypePlaylist=%1 プレイリスト
italian.TypePlaylist=Scaletta %1
spanish.TypePlaylist=Lista de reproducción %1
polish.TypePlaylist=Lista odtwarzania %1
ukrainian.TypePlaylist=Список відтворення %1
russian.TypePlaylist=Плейлист %1
arabic.TypePlaylist=قائمة تشغيل %1
turkish.TypePlaylist=%1 oynatma listesi
english.VerbPlay=Play
hebrew.VerbPlay=הפעלה
brazilianportuguese.VerbPlay=Reproduzir
dutch.VerbPlay=Afspelen
german.VerbPlay=Wiedergeben
czech.VerbPlay=Přehrát
swedish.VerbPlay=Spela upp
portuguese.VerbPlay=Reproduzir
french.VerbPlay=Lire
korean.VerbPlay=재생
japanese.VerbPlay=再生
italian.VerbPlay=Riproduci
spanish.VerbPlay=Reproducir
polish.VerbPlay=Odtwórz
ukrainian.VerbPlay=Відтворити
russian.VerbPlay=Воспроизвести
arabic.VerbPlay=تشغيل
turkish.VerbPlay=Oynat
english.VerbEnqueue=Add to queue
hebrew.VerbEnqueue=הוספה לתור
brazilianportuguese.VerbEnqueue=Adicionar à fila
dutch.VerbEnqueue=In wachtrij zetten
german.VerbEnqueue=Zur Warteschlange hinzufügen
czech.VerbEnqueue=Přidat do fronty
swedish.VerbEnqueue=Lägg till i kön
portuguese.VerbEnqueue=Adicionar à fila
french.VerbEnqueue=Ajouter à la file
korean.VerbEnqueue=대기열에 추가
japanese.VerbEnqueue=キューに追加
italian.VerbEnqueue=Aggiungi alla coda
spanish.VerbEnqueue=Añadir a la cola
polish.VerbEnqueue=Dodaj do kolejki
ukrainian.VerbEnqueue=Додати до черги
russian.VerbEnqueue=Добавить в очередь
arabic.VerbEnqueue=إضافة إلى قائمة الانتظار
turkish.VerbEnqueue=Kuyruğa ekle
english.ContextPlay=Play with {#AppName}
hebrew.ContextPlay=הפעלה ב-{#AppName}
brazilianportuguese.ContextPlay=Reproduzir com o {#AppName}
dutch.ContextPlay=Afspelen met {#AppName}
german.ContextPlay=Mit {#AppName} wiedergeben
czech.ContextPlay=Přehrát pomocí {#AppName}
swedish.ContextPlay=Spela upp med {#AppName}
portuguese.ContextPlay=Reproduzir com o {#AppName}
french.ContextPlay=Lire avec {#AppName}
korean.ContextPlay={#AppName}으로 재생
japanese.ContextPlay={#AppName}で再生
italian.ContextPlay=Riproduci con {#AppName}
spanish.ContextPlay=Reproducir con {#AppName}
polish.ContextPlay=Odtwórz w {#AppName}
ukrainian.ContextPlay=Відтворити в {#AppName}
russian.ContextPlay=Воспроизвести в {#AppName}
arabic.ContextPlay=التشغيل باستخدام {#AppName}
turkish.ContextPlay={#AppName} ile oynat
english.ContextEnqueue=Add to {#AppName} queue
hebrew.ContextEnqueue=הוספה לתור של {#AppName}
brazilianportuguese.ContextEnqueue=Adicionar à fila do {#AppName}
dutch.ContextEnqueue=In wachtrij van {#AppName} zetten
german.ContextEnqueue=Zur Warteschlange von {#AppName} hinzufügen
czech.ContextEnqueue=Přidat do fronty {#AppName}
swedish.ContextEnqueue=Lägg till i kön i {#AppName}
portuguese.ContextEnqueue=Adicionar à fila do {#AppName}
french.ContextEnqueue=Ajouter à la file de {#AppName}
korean.ContextEnqueue={#AppName} 대기열에 추가
japanese.ContextEnqueue={#AppName}のキューに追加
italian.ContextEnqueue=Aggiungi alla coda di {#AppName}
spanish.ContextEnqueue=Añadir a la cola de {#AppName}
polish.ContextEnqueue=Dodaj do kolejki {#AppName}
ukrainian.ContextEnqueue=Додати до черги {#AppName}
russian.ContextEnqueue=Добавить в очередь {#AppName}
arabic.ContextEnqueue=إضافة إلى قائمة انتظار {#AppName}
turkish.ContextEnqueue={#AppName} kuyruğuna ekle
english.Running={#AppName} is open. Setup will close it now and open it again when it's done.
hebrew.Running={#AppName} פתוח. ההתקנה תסגור אותו עכשיו ותפתח אותו מחדש בסיום.
brazilianportuguese.Running=O {#AppName} está aberto. O instalador vai fechá-lo agora e abri-lo de novo quando terminar.
dutch.Running={#AppName} is geopend. Setup sluit het nu en opent het opnieuw als het klaar is.
german.Running={#AppName} ist geöffnet. Das Setup schließt es jetzt und öffnet es nach Abschluss wieder.
czech.Running={#AppName} je otevřen. Instalační program ho nyní zavře a po dokončení znovu otevře.
swedish.Running={#AppName} är öppen. Installationen stänger den nu och öppnar den igen efteråt.
portuguese.Running=O {#AppName} está aberto. A instalação vai fechá-lo agora e abri-lo novamente quando terminar.
french.Running={#AppName} est ouvert. L'installation va le fermer maintenant et le rouvrir une fois terminée.
korean.Running={#AppName}이 열려 있습니다. 설치 프로그램이 지금 닫고, 완료되면 다시 엽니다.
japanese.Running={#AppName}が開いています。セットアップは今すぐ閉じ、完了後にもう一度開きます。
italian.Running={#AppName} è aperto. L'installazione lo chiuderà ora e lo riaprirà al termine.
spanish.Running={#AppName} está abierto. El instalador lo cerrará ahora y lo volverá a abrir al terminar.
polish.Running={#AppName} jest otwarty. Instalator zamknie go teraz i otworzy ponownie po zakończeniu.
ukrainian.Running={#AppName} відкрито. Інсталятор закриє його зараз і знову відкриє після завершення.
russian.Running={#AppName} открыт. Установщик сейчас закроет его и снова откроет по завершении.
arabic.Running=تطبيق {#AppName} مفتوح. سيغلقه برنامج الإعداد الآن ويعيد فتحه عند الانتهاء.
turkish.Running={#AppName} açık. Kurulum onu şimdi kapatacak ve bittiğinde yeniden açacak.
english.StillRunning={#AppName} didn't close. Close it and click Retry.
hebrew.StillRunning={#AppName} לא נסגר. סגרו אותו ולחצו על "נסה שוב".
brazilianportuguese.StillRunning=O {#AppName} não fechou. Feche-o e clique em "Tentar novamente".
dutch.StillRunning={#AppName} is niet gesloten. Sluit het en klik op "Opnieuw proberen".
german.StillRunning={#AppName} wurde nicht geschlossen. Schließen Sie es und klicken Sie auf „Wiederholen“.
czech.StillRunning={#AppName} se nezavřel. Zavřete ho a klikněte na Opakovat.
swedish.StillRunning={#AppName} stängdes inte. Stäng den och klicka på Försök igen.
portuguese.StillRunning=O {#AppName} não fechou. Feche-o e clique em Tentar novamente.
french.StillRunning={#AppName} ne s'est pas fermé. Fermez-le et cliquez sur Réessayer.
korean.StillRunning={#AppName}이 닫히지 않았습니다. 닫은 다음 "다시 시도"를 클릭하세요.
japanese.StillRunning={#AppName}が終了しませんでした。閉じてから「再試行」をクリックしてください。
italian.StillRunning={#AppName} non si è chiuso. Chiudilo e fai clic su Riprova.
spanish.StillRunning={#AppName} no se cerró. Ciérrelo y haga clic en Reintentar.
polish.StillRunning={#AppName} nadal działa. Zamknij go i kliknij Ponów próbę.
ukrainian.StillRunning={#AppName} не закрився. Закрийте його й натисніть «Спробувати знову».
russian.StillRunning={#AppName} не закрылся. Закройте его и нажмите «Попробовать снова».
arabic.StillRunning=لم يُغلق {#AppName}. أغلقه وانقر «إعادة المحاولة».
turkish.StillRunning={#AppName} kapanmadı. Kapatın ve Yeniden Dene'ye tıklayın.
english.ShortcutComment=VLC media player with the interface designed by Juxtopposed
hebrew.ShortcutComment=נגן המדיה VLC עם הממשק שעיצבה Juxtopposed
brazilianportuguese.ShortcutComment=Reprodutor de Mídias VLC com a interface criada por Juxtopposed
dutch.ShortcutComment=VLC media player met de interface ontworpen door Juxtopposed
german.ShortcutComment=VLC media player mit der von Juxtopposed gestalteten Oberfläche
czech.ShortcutComment=VLC media player s rozhraním, které navrhla Juxtopposed
swedish.ShortcutComment=VLC media player med gränssnittet som Juxtopposed har designat
portuguese.ShortcutComment=VLC media player com a interface desenhada por Juxtopposed
french.ShortcutComment=VLC media player avec l'interface conçue par Juxtopposed
korean.ShortcutComment=Juxtopposed가 디자인한 인터페이스의 VLC 미디어 재생기
japanese.ShortcutComment=Juxtopposedがデザインしたインターフェースを備えたVLCメディアプレイヤー
italian.ShortcutComment=Lettore multimediale VLC con l'interfaccia progettata da Juxtopposed
spanish.ShortcutComment=Reproductor multimedia VLC con la interfaz diseñada por Juxtopposed
polish.ShortcutComment=VLC media player z interfejsem zaprojektowanym przez Juxtopposed
ukrainian.ShortcutComment=Медіапрогравач VLC з інтерфейсом від Juxtopposed
russian.ShortcutComment=Медиапроигрыватель VLC с интерфейсом от Juxtopposed
arabic.ShortcutComment=مشغل الوسائط VLC مع الواجهة التي صممتها Juxtopposed
turkish.ShortcutComment=Juxtopposed'un tasarladığı arayüze sahip VLC media player
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
LanguageContext_dutch=Uw Windows is in het Nederlands.
LanguageQuestion_dutch={#AppName} in het Nederlands installeren?
LanguageYes_dutch=Ja
LanguageNo_dutch=Nee
LanguageContext_german=Ihr Windows ist auf Deutsch.
LanguageQuestion_german={#AppName} auf Deutsch installieren?
LanguageYes_german=Ja
LanguageNo_german=Nein
LanguageContext_czech=Váš systém Windows je v češtině.
LanguageQuestion_czech=Nainstalovat {#AppName} v češtině?
LanguageYes_czech=Ano
LanguageNo_czech=Ne
LanguageContext_swedish=Ditt Windows är på svenska.
LanguageQuestion_swedish=Installera {#AppName} på svenska?
LanguageYes_swedish=Ja
LanguageNo_swedish=Nej
LanguageContext_portuguese=O seu Windows está em português.
LanguageQuestion_portuguese=Instalar o {#AppName} em português?
LanguageYes_portuguese=Sim
LanguageNo_portuguese=Não
LanguageContext_french=Votre Windows est en français.
LanguageQuestion_french=Installer {#AppName} en français ?
LanguageYes_french=Oui
LanguageNo_french=Non
LanguageContext_korean=Windows가 한국어로 설정되어 있습니다.
LanguageQuestion_korean={#AppName}을 한국어로 설치할까요?
LanguageYes_korean=예
LanguageNo_korean=아니요
LanguageContext_japanese=お使いのWindowsは日本語です。
LanguageQuestion_japanese={#AppName}を日本語でインストールしますか?
LanguageYes_japanese=はい
LanguageNo_japanese=いいえ
LanguageContext_italian=Il tuo Windows è in italiano.
LanguageQuestion_italian=Installare {#AppName} in italiano?
LanguageYes_italian=Sì
LanguageNo_italian=No
LanguageContext_spanish=Su Windows está en español.
LanguageQuestion_spanish=¿Instalar {#AppName} en español?
LanguageYes_spanish=Sí
LanguageNo_spanish=No
LanguageContext_polish=Twój Windows jest po polsku.
LanguageQuestion_polish=Zainstalować {#AppName} po polsku?
LanguageYes_polish=Tak
LanguageNo_polish=Nie
LanguageContext_ukrainian=Мова Windows — українська.
LanguageQuestion_ukrainian=Встановити {#AppName} українською?
LanguageYes_ukrainian=Так
LanguageNo_ukrainian=Ні
LanguageContext_russian=Язык вашей Windows — русский.
LanguageQuestion_russian=Установить {#AppName} на русском языке?
LanguageYes_russian=Да
LanguageNo_russian=Нет
LanguageContext_arabic=نظام Windows لديك باللغة العربية.
LanguageQuestion_arabic=هل تريد تثبيت {#AppName} باللغة العربية؟
LanguageYes_arabic=نعم
LanguageNo_arabic=لا
LanguageContext_turkish=Windows'unuz Türkçe.
LanguageQuestion_turkish={#AppName} Türkçe kurulsun mu?
LanguageYes_turkish=Evet
LanguageNo_turkish=Hayır

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
