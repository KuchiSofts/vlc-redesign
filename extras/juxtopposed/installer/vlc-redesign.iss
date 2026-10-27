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
; The answers go to {app}\portable\setup-choices.ini (installed for everyone:
; to %APPDATA%\VLC Redesign of the user who runs Setup, as every
; {app}\portable\ below), which VLC Redesign puts
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
; right to left. The Languages page (a new install, or a profile without a
; language) has two lists: the app's interface language, Setup's to begin
; with, written to {app}\portable\language, and the default subtitle
; language, the interface's until chosen apart, for setup-choices.ini
; ([subtitles] language); for a right-to-left interface language, the app's
; right-to-left layout too, on to begin with as the app lays out the
; language, for setup-choices.ini ([interface]) as the Privacy page's
; answers. An update runs in the language of the
; install it updates (UsePreviousLanguage), and so does the uninstaller.
; A silent install never asks: the app in English (or as it was), unless
; told otherwise:
;   /LANG=hebrew        Setup in Hebrew, and the app too (a copy that has a
;                       language keeps it); the names are those of
;                       [Languages]: english, hebrew, brazilianportuguese
;   /RIGHTTOLEFT        the app's right-to-left layout
;   /LEFTTORIGHT        the app's left-to-right layout (without either, the
;                       direction of the app's language)
;   /TESTUILANG=0x040D  for tests: the Windows display language Setup sees,
;                       to ask its question on any Windows
;   /UILANG=he          the app's interface language (a code of its
;                       languages: he, pt_BR, en_GB...), without the page
;   /SUBLANG=en         the default subtitle language (the same codes)

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
#expr AddLanguage("catalan", "compiler:Languages\Catalan.isl", "ca", 0x0003, 0, 1, "Catalan")
#expr AddLanguage("danish", "compiler:Languages\Danish.isl", "da", 0x0006, 0, 1, "Danish")
#expr AddLanguage("finnish", "compiler:Languages\Finnish.isl", "fi", 0x000B, 0, 1, "Finnish")
#expr AddLanguage("hungarian", "compiler:Languages\Hungarian.isl", "hu", 0x000E, 0, 1, "Hungarian")
#expr AddLanguage("bulgarian", "compiler:Languages\Bulgarian.isl", "bg", 0x0002, 0, 1, "Bulgarian")
#expr AddLanguage("slovak", "compiler:Languages\Slovak.isl", "sk", 0x001B, 0, 1, "Slovak")
#expr AddLanguage("slovenian", "compiler:Languages\Slovenian.isl", "sl", 0x0024, 0, 1, "Slovenian")
#expr AddLanguage("norwegian", "compiler:Languages\Norwegian.isl", "nb", 0x0014, 0, 1, "Norwegian")
#expr AddLanguage("thai", "compiler:Languages\Thai.isl", "th", 0x001E, 0, 0, "Thai")
#expr AddLanguage("tamil", "compiler:Languages\Tamil.isl", "ta", 0x0049, 0, 0, "Tamil")
#expr AddLanguage("armenian", "compiler:Languages\Armenian.isl", "hy", 0x002B, 0, 0, "Armenian")

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
catalan.StepWelcome=Benvinguda
danish.StepWelcome=Velkommen
finnish.StepWelcome=Tervetuloa
hungarian.StepWelcome=Üdvözlés
bulgarian.StepWelcome=Добре дошли
slovak.StepWelcome=Vitajte
slovenian.StepWelcome=Dobrodošli
norwegian.StepWelcome=Velkommen
thai.StepWelcome=ยินดีต้อนรับ
tamil.StepWelcome=வரவேற்பு
armenian.StepWelcome=Բարի գալուստ
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
catalan.StepOptions=Opcions
danish.StepOptions=Indstillinger
finnish.StepOptions=Valinnat
hungarian.StepOptions=Beállítások
bulgarian.StepOptions=Опции
slovak.StepOptions=Voľby
slovenian.StepOptions=Možnosti
norwegian.StepOptions=Valg
thai.StepOptions=ตัวเลือก
tamil.StepOptions=விருப்பங்கள்
armenian.StepOptions=Ընտրանքներ
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
catalan.StepPrivacy=Privadesa
danish.StepPrivacy=Privatliv
finnish.StepPrivacy=Tietosuoja
hungarian.StepPrivacy=Adatvédelem
bulgarian.StepPrivacy=Поверителност
slovak.StepPrivacy=Súkromie
slovenian.StepPrivacy=Zasebnost
norwegian.StepPrivacy=Personvern
thai.StepPrivacy=ความเป็นส่วนตัว
tamil.StepPrivacy=தனியுரிமை
armenian.StepPrivacy=Գաղտնիություն
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
catalan.StepInstall=Instal·lació
danish.StepInstall=Installation
finnish.StepInstall=Asennus
hungarian.StepInstall=Telepítés
bulgarian.StepInstall=Инсталиране
slovak.StepInstall=Inštalácia
slovenian.StepInstall=Namestitev
norwegian.StepInstall=Installasjon
thai.StepInstall=ติดตั้ง
tamil.StepInstall=நிறுவல்
armenian.StepInstall=Տեղադրում
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
catalan.StepDone=Fet
danish.StepDone=Færdig
finnish.StepDone=Valmis
hungarian.StepDone=Kész
bulgarian.StepDone=Готово
slovak.StepDone=Hotovo
slovenian.StepDone=Končano
norwegian.StepDone=Ferdig
thai.StepDone=เสร็จสิ้น
tamil.StepDone=முடிந்தது
armenian.StepDone=Պատրաստ է
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
catalan.Unofficial=Compilació no oficial
danish.Unofficial=Uofficiel version
finnish.Unofficial=Epävirallinen koontiversio
hungarian.Unofficial=Nem hivatalos kiadás
bulgarian.Unofficial=Неофициална компилация
slovak.Unofficial=Neoficiálne zostavenie
slovenian.Unofficial=Neuradna različica
norwegian.Unofficial=Uoffisiell utgave
thai.Unofficial=รุ่นที่ไม่เป็นทางการ
tamil.Unofficial=அதிகாரப்பூர்வமற்ற உருவாக்கம்
armenian.Unofficial=Ոչ պաշտոնական տարբերակ
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
catalan.DesignedBy=Interfície dissenyada per
danish.DesignedBy=Brugerflade designet af
finnish.DesignedBy=Käyttöliittymän suunnitteli
hungarian.DesignedBy=A kezelőfelület tervezője
bulgarian.DesignedBy=Интерфейс, създаден от
slovak.DesignedBy=Dizajn rozhrania
slovenian.DesignedBy=Vmesnik je oblikovala
norwegian.DesignedBy=Grensesnitt designet av
thai.DesignedBy=ออกแบบส่วนติดต่อโดย
tamil.DesignedBy=இடைமுக வடிவமைப்பு
armenian.DesignedBy=Ինտերֆեյսի դիզայնը՝
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
catalan.WelcomeTitle=Us donem la benvinguda al {#AppName}
danish.WelcomeTitle=Velkommen til {#AppName}
finnish.WelcomeTitle=Tervetuloa {#AppName} -sovellukseen
hungarian.WelcomeTitle=Üdvözli a {#AppName}
bulgarian.WelcomeTitle=Добре дошли във {#AppName}
slovak.WelcomeTitle=Vitajte vo {#AppName}
slovenian.WelcomeTitle=Dobrodošli v {#AppName}
norwegian.WelcomeTitle=Velkommen til {#AppName}
thai.WelcomeTitle=ยินดีต้อนรับสู่ {#AppName}
tamil.WelcomeTitle={#AppName}-க்கு வரவேற்கிறோம்
armenian.WelcomeTitle=Բարի գալուստ {#AppName}
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
catalan.WelcomeText=El motor autèntic del VLC 4 amb la interfície dissenyada per Juxtopposed. La instal·lació triga aproximadament un minut.
danish.WelcomeText=Den ægte VLC 4-motor med brugerfladen designet af Juxtopposed. Installationen tager omkring et minut.
finnish.WelcomeText=Aito VLC 4 -moottori Juxtopposedin suunnittelemalla käyttöliittymällä. Asennus kestää noin minuutin.
hungarian.WelcomeText=Az igazi VLC 4 motor a Juxtopposed által tervezett kezelőfelülettel. A telepítés körülbelül egy percig tart.
bulgarian.WelcomeText=Истинският двигател на VLC 4 с интерфейса, създаден от Juxtopposed. Инсталирането отнема около минута.
slovak.WelcomeText=Skutočné jadro VLC 4 s rozhraním od Juxtopposed. Inštalácia trvá asi minútu.
slovenian.WelcomeText=Pravi pogon VLC 4 z vmesnikom, ki ga je oblikovala Juxtopposed. Namestitev traja približno minuto.
norwegian.WelcomeText=Den ekte VLC 4-motoren med grensesnittet designet av Juxtopposed. Installasjonen tar omtrent ett minutt.
thai.WelcomeText=เอนจิน VLC 4 ตัวจริง พร้อมส่วนติดต่อที่ออกแบบโดย Juxtopposed การติดตั้งใช้เวลาประมาณหนึ่งนาที
tamil.WelcomeText=Juxtopposed வடிவமைத்த இடைமுகத்துடன் உண்மையான VLC 4 இயந்திரம். நிறுவலுக்குச் சுமார் ஒரு நிமிடம் ஆகும்.
armenian.WelcomeText=Իսկական VLC 4 շարժիչը՝ Juxtopposed-ի ստեղծած ինտերֆեյսով։ Տեղադրումը տևում է մոտ մեկ րոպե։
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
catalan.CreditTitle=Interfície dissenyada per Juxtopposed
danish.CreditTitle=Brugerflade designet af Juxtopposed
finnish.CreditTitle=Käyttöliittymän suunnitteli Juxtopposed
hungarian.CreditTitle=A kezelőfelületet Juxtopposed tervezte
bulgarian.CreditTitle=Интерфейс, създаден от Juxtopposed
slovak.CreditTitle=Dizajn rozhrania: Juxtopposed
slovenian.CreditTitle=Vmesnik je oblikovala Juxtopposed
norwegian.CreditTitle=Grensesnitt designet av Juxtopposed
thai.CreditTitle=ออกแบบส่วนติดต่อโดย Juxtopposed
tamil.CreditTitle=Juxtopposed வடிவமைத்த இடைமுகம்
armenian.CreditTitle=Ինտերֆեյսի դիզայնը՝ Juxtopposed
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
catalan.CreditText=Totes les pantalles provenen del fitxer de Figma «VLC Redesign (Community)», amb llicència CC BY 4.0. Juxtopposed no avala aquest projecte.
danish.CreditText=Alle skærmbilleder stammer fra Figma-filen "VLC Redesign (Community)", licenseret under CC BY 4.0. Juxtopposed står ikke bag dette projekt.
finnish.CreditText=Jokainen näkymä on peräisin Figma-tiedostosta ”VLC Redesign (Community)”, jonka lisenssi on CC BY 4.0. Juxtopposed ei tue tätä projektia.
hungarian.CreditText=Minden képernyő a CC BY 4.0 licencű „VLC Redesign (Community)” Figma-fájlból származik. Juxtopposed nem támogatja ezt a projektet.
bulgarian.CreditText=Всеки екран идва от файла „VLC Redesign (Community)“ във Figma, лицензиран под CC BY 4.0. Juxtopposed не одобрява този проект.
slovak.CreditText=Všetky obrazovky pochádzajú zo súboru Figma „VLC Redesign (Community)“ s licenciou CC BY 4.0. Juxtopposed tento projekt nijako nepodporuje.
slovenian.CreditText=Vsak zaslon izhaja iz datoteke Figma »VLC Redesign (Community)«, ki je na voljo pod licenco CC BY 4.0. Juxtopposed tega projekta ne podpira.
norwegian.CreditText=Hver skjerm kommer fra Figma-filen «VLC Redesign (Community)», lisensiert under CC BY 4.0. Juxtopposed står ikke bak dette prosjektet.
thai.CreditText=ทุกหน้าจอมาจากแฟ้ม Figma "VLC Redesign (Community)" ซึ่งใช้สัญญาอนุญาต CC BY 4.0 ทั้งนี้ Juxtopposed ไม่ได้รับรองโครงการนี้
tamil.CreditText=ஒவ்வொரு திரையும் CC BY 4.0 உரிமத்தின் கீழுள்ள "VLC Redesign (Community)" Figma கோப்பிலிருந்து வருகிறது. Juxtopposed இந்தத் திட்டத்தை அங்கீகரிக்கவில்லை.
armenian.CreditText=Յուրաքանչյուր էկրան վերցված է «VLC Redesign (Community)» Figma ֆայլից, որը լիցենզավորված է CC BY 4.0-ով։ Juxtopposed-ը հավանություն չի տվել այս նախագծին։
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
catalan.LinkVideo=Mireu el vídeo
danish.LinkVideo=Se videoen
finnish.LinkVideo=Katso video
hungarian.LinkVideo=Videó megtekintése
bulgarian.LinkVideo=Гледайте видеото
slovak.LinkVideo=Pozrieť si video
slovenian.LinkVideo=Oglejte si video
norwegian.LinkVideo=Se videoen
thai.LinkVideo=ดูวีดิทัศน์
tamil.LinkVideo=நிகழ்படத்தைப் பாருங்கள்
armenian.LinkVideo=Դիտել վիդեոն
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
catalan.LinkChannel=Canal de YouTube
danish.LinkChannel=YouTube-kanal
finnish.LinkChannel=YouTube-kanava
hungarian.LinkChannel=YouTube-csatorna
bulgarian.LinkChannel=Канал в YouTube
slovak.LinkChannel=Kanál YouTube
slovenian.LinkChannel=Kanal YouTube
norwegian.LinkChannel=YouTube-kanal
thai.LinkChannel=ช่อง YouTube
tamil.LinkChannel=YouTube சேனல்
armenian.LinkChannel=YouTube ալիք
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
catalan.LinkFigma=Fitxer de Figma
danish.LinkFigma=Figma-fil
finnish.LinkFigma=Figma-tiedosto
hungarian.LinkFigma=Figma-fájl
bulgarian.LinkFigma=Файл във Figma
slovak.LinkFigma=Súbor Figma
slovenian.LinkFigma=Datoteka Figma
norwegian.LinkFigma=Figma-fil
thai.LinkFigma=แฟ้ม Figma
tamil.LinkFigma=Figma கோப்பு
armenian.LinkFigma=Figma ֆայլ
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
catalan.WelcomeNotice=Una compilació no oficial, no creada ni avalada per VideoLAN. VLC i el logotip del con són marques comercials de VideoLAN. S'instal·la al costat d'un VLC oficial sense modificar-lo.
danish.WelcomeNotice=En uofficiel version, hverken lavet eller godkendt af VideoLAN. VLC og kegle-logoet er varemærker tilhørende VideoLAN. Den installeres ved siden af en officiel VLC uden at røre den.
finnish.WelcomeNotice=Epävirallinen koontiversio, jota VideoLAN ei ole tehnyt eikä hyväksynyt. VLC ja kartiologo ovat VideoLANin tavaramerkkejä. Se asennetaan virallisen VLC:n rinnalle koskematta siihen.
hungarian.WelcomeNotice=Nem hivatalos kiadás, amelyet nem a VideoLAN készített, és nem is támogat. A VLC és a kúp logó a VideoLAN védjegyei. Egy hivatalos VLC mellé települ anélkül, hogy azt módosítaná.
bulgarian.WelcomeNotice=Неофициална компилация, която не е създадена или одобрена от VideoLAN. VLC и логото с конуса са търговски марки на VideoLAN. Инсталира се до официалния VLC, без да го засяга.
slovak.WelcomeNotice=Neoficiálne zostavenie, ktoré nevytvorila ani neschválila organizácia VideoLAN. VLC a logo kužeľa sú ochranné známky VideoLAN. Inštaluje sa vedľa oficiálneho VLC bez toho, aby ho ovplyvnilo.
slovenian.WelcomeNotice=Neuradna različica, ki je ni izdelal ali odobril VideoLAN. VLC in logotip stožca sta blagovni znamki VideoLAN. Namesti se poleg uradnega VLC in ga ne spremeni.
norwegian.WelcomeNotice=En uoffisiell utgave, ikke laget eller godkjent av VideoLAN. VLC og kjeglelogoen er varemerker for VideoLAN. Den installeres ved siden av en offisiell VLC uten å røre den.
thai.WelcomeNotice=รุ่นที่ไม่เป็นทางการ ซึ่ง VideoLAN ไม่ได้สร้างหรือรับรอง VLC และโลโก้กรวยเป็นเครื่องหมายการค้าของ VideoLAN ติดตั้งไว้ข้าง VLC ทางการโดยไม่แตะต้อง VLC ตัวนั้น
tamil.WelcomeNotice=அதிகாரப்பூர்வமற்ற உருவாக்கம்; VideoLAN உருவாக்கியதோ அங்கீகரித்ததோ அல்ல. VLC மற்றும் கூம்புச் சின்னம் VideoLAN-இன் வர்த்தகமுத்திரைகள். இது அதிகாரப்பூர்வ VLC-ஐத் தொடாமல் அதன் அருகில் நிறுவப்படும்.
armenian.WelcomeNotice=Ոչ պաշտոնական տարբերակ, որը ստեղծված կամ հավանության արժանացած չէ VideoLAN-ի կողմից։ VLC-ն և կոնի լոգոն VideoLAN-ի ապրանքային նշաններն են։ Այն տեղադրվում է պաշտոնական VLC-ի կողքին՝ առանց այն փոփոխելու։
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
catalan.LinkNotice=Avís i llicències
danish.LinkNotice=Oplysninger og licenser
finnish.LinkNotice=Tiedot ja lisenssit
hungarian.LinkNotice=Tájékoztató és licencek
bulgarian.LinkNotice=Бележки и лицензи
slovak.LinkNotice=Upozornenie a licencie
slovenian.LinkNotice=Obvestilo in licence
norwegian.LinkNotice=Merknad og lisenser
thai.LinkNotice=ประกาศและสัญญาอนุญาต
tamil.LinkNotice=அறிவிப்பும் உரிமங்களும்
armenian.LinkNotice=Ծանուցում և լիցենզիաներ
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
catalan.LinkSource=Codi font
danish.LinkSource=Kildekode
finnish.LinkSource=Lähdekoodi
hungarian.LinkSource=Forráskód
bulgarian.LinkSource=Изходен код
slovak.LinkSource=Zdrojový kód
slovenian.LinkSource=Izvorna koda
norwegian.LinkSource=Kildekode
thai.LinkSource=ซอร์สโค้ด
tamil.LinkSource=மூலக் குறியீடு
armenian.LinkSource=Ելակետային կոդ
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
catalan.VersionLine=Versió %1 · programari lliure sota la GNU GPL v2 o posterior
danish.VersionLine=Version %1 · fri software under GNU GPL v2 eller senere
finnish.VersionLine=Versio %1 · vapaa ohjelmisto GNU GPL v2:n tai uudemman alaisena
hungarian.VersionLine=Verzió: %1 · szabad szoftver a GNU GPL v2 vagy újabb licenc alatt
bulgarian.VersionLine=Версия %1 · свободен софтуер под GNU GPL v2 или по-нова версия
slovak.VersionLine=Verzia %1 · slobodný softvér pod licenciou GNU GPL v2 alebo novšou
slovenian.VersionLine=Različica %1 · prosto programje pod licenco GNU GPL v2 ali novejšo
norwegian.VersionLine=Versjon %1 · fri programvare under GNU GPL v2 eller senere
thai.VersionLine=รุ่น %1 · ซอฟต์แวร์เสรีภายใต้ GNU GPL v2 หรือใหม่กว่า
tamil.VersionLine=பதிப்பு %1 · GNU GPL v2 அல்லது பிந்தைய உரிமத்தின் கீழ் இலவச மென்பொருள்
armenian.VersionLine=Տարբերակ %1 · ազատ ծրագրակազմ GNU GPL v2 կամ ավելի նոր լիցենզիայով
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
catalan.OptionsTitle=Opcions
danish.OptionsTitle=Indstillinger
finnish.OptionsTitle=Valinnat
hungarian.OptionsTitle=Beállítások
bulgarian.OptionsTitle=Опции
slovak.OptionsTitle=Voľby
slovenian.OptionsTitle=Možnosti
norwegian.OptionsTitle=Valg
thai.OptionsTitle=ตัวเลือก
tamil.OptionsTitle=விருப்பங்கள்
armenian.OptionsTitle=Ընտրանքներ
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
catalan.OptionsText=On s'instal·la i què obre.
danish.OptionsText=Hvor den installeres, og hvad den åbner.
finnish.OptionsText=Minne se asennetaan ja mitä se avaa.
hungarian.OptionsText=Hová települjön, és mit nyisson meg.
bulgarian.OptionsText=Къде да се инсталира и какво да отваря.
slovak.OptionsText=Kam sa nainštaluje a čo bude otvárať.
slovenian.OptionsText=Kam se namesti in kaj odpira.
norwegian.OptionsText=Hvor den installeres, og hva den åpner.
thai.OptionsText=ติดตั้งที่ใด และให้เปิดอะไร
tamil.OptionsText=எங்கே நிறுவப்படும், எதைத் திறக்கும்.
armenian.OptionsText=Որտեղ տեղադրել և ինչ բացել։
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
catalan.InstallTo=Instal·la a
danish.InstallTo=Installér i
finnish.InstallTo=Asennuskohde
hungarian.InstallTo=Telepítés helye
bulgarian.InstallTo=Инсталиране в
slovak.InstallTo=Inštalovať do
slovenian.InstallTo=Namesti v
norwegian.InstallTo=Installer i
thai.InstallTo=ติดตั้งไปที่
tamil.InstallTo=நிறுவும் இடம்
armenian.InstallTo=Տեղադրման պանակ
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
catalan.Browse=Navega
danish.Browse=Gennemse
finnish.Browse=Selaa
hungarian.Browse=Tallózás
bulgarian.Browse=Преглед
slovak.Browse=Prehľadávať
slovenian.Browse=Prebrskaj
norwegian.Browse=Bla gjennom
thai.Browse=เรียกดู
tamil.Browse=உலாவு
armenian.Browse=Ընտրել
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
catalan.ForYou=Només per a vós, sense drets d'administrador.
danish.ForYou=Kun for dig, uden administratorrettigheder.
finnish.ForYou=Vain sinulle, ilman järjestelmänvalvojan oikeuksia.
hungarian.ForYou=Csak az Ön számára, rendszergazdai jogok nélkül.
bulgarian.ForYou=Само за вас, без администраторски права.
slovak.ForYou=Iba pre vás, bez práv správcu.
slovenian.ForYou=Samo za vas, brez skrbniških pravic.
norwegian.ForYou=Bare for deg, uten administratorrettigheter.
thai.ForYou=สำหรับคุณเท่านั้น ไม่ต้องใช้สิทธิ์ผู้ดูแลระบบ
tamil.ForYou=உங்களுக்கு மட்டும், நிர்வாகி உரிமைகள் இல்லாமல்.
armenian.ForYou=Միայն ձեզ համար, առանց ադմինիստրատորի իրավունքների։
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
catalan.ForEveryone=Instal·la per a tothom
danish.ForEveryone=Installér for alle
finnish.ForEveryone=Asenna kaikille
hungarian.ForEveryone=Telepítés mindenkinek
bulgarian.ForEveryone=Инсталиране за всички
slovak.ForEveryone=Inštalovať pre všetkých
slovenian.ForEveryone=Namesti za vse
norwegian.ForEveryone=Installer for alle
thai.ForEveryone=ติดตั้งสำหรับทุกคน
tamil.ForEveryone=அனைவருக்கும் நிறுவு
armenian.ForEveryone=Տեղադրել բոլորի համար
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
catalan.ForAll=Per a tothom en aquest ordinador.
danish.ForAll=For alle på denne pc.
finnish.ForAll=Kaikille tämän tietokoneen käyttäjille.
hungarian.ForAll=A számítógép minden felhasználója számára.
bulgarian.ForAll=За всички на този компютър.
slovak.ForAll=Pre všetkých na tomto počítači.
slovenian.ForAll=Za vse uporabnike tega računalnika.
norwegian.ForAll=For alle på denne PC-en.
thai.ForAll=สำหรับทุกคนในพีซีเครื่องนี้
tamil.ForAll=இந்தக் கணினியிலுள்ள அனைவருக்கும்.
armenian.ForAll=Այս համակարգչի բոլոր օգտատերերի համար։
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
catalan.UpdatingHere=Actualitza la còpia d'aquesta carpeta.
danish.UpdatingHere=Opdaterer kopien i denne mappe.
finnish.UpdatingHere=Päivittää tässä kansiossa olevan kopion.
hungarian.UpdatingHere=Frissíti az ebben a mappában lévő példányt.
bulgarian.UpdatingHere=Обновява копието в тази папка.
slovak.UpdatingHere=Aktualizuje kópiu v tomto priečinku.
slovenian.UpdatingHere=Posodobi kopijo v tej mapi.
norwegian.UpdatingHere=Oppdaterer kopien i denne mappen.
thai.UpdatingHere=ปรับปรุงสำเนาในโฟลเดอร์นี้
tamil.UpdatingHere=இந்தக் கோப்புறையிலுள்ள நகலைப் புதுப்பிக்கிறது.
armenian.UpdatingHere=Թարմացնում է այս պանակի պատճենը։
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
catalan.OpenVideo=Obre els vídeos amb el {#AppName}
danish.OpenVideo=Åbn videoer med {#AppName}
finnish.OpenVideo=Avaa videot sovelluksella {#AppName}
hungarian.OpenVideo=Videók megnyitása a {#AppName} programmal
bulgarian.OpenVideo=Отваряне на видеа с {#AppName}
slovak.OpenVideo=Otvárať videá v {#AppName}
slovenian.OpenVideo=Odpiraj videe z {#AppName}
norwegian.OpenVideo=Åpne videoer med {#AppName}
thai.OpenVideo=เปิดวีดิทัศน์ด้วย {#AppName}
tamil.OpenVideo=நிகழ்படங்களை {#AppName} மூலம் திற
armenian.OpenVideo=Բացել վիդեոները {#AppName}-ով
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
catalan.OpenAudio=Obre la música amb el {#AppName}
danish.OpenAudio=Åbn musik med {#AppName}
finnish.OpenAudio=Avaa musiikki sovelluksella {#AppName}
hungarian.OpenAudio=Zenék megnyitása a {#AppName} programmal
bulgarian.OpenAudio=Отваряне на музика с {#AppName}
slovak.OpenAudio=Otvárať hudbu v {#AppName}
slovenian.OpenAudio=Odpiraj glasbo z {#AppName}
norwegian.OpenAudio=Åpne musikk med {#AppName}
thai.OpenAudio=เปิดเพลงด้วย {#AppName}
tamil.OpenAudio=இசையை {#AppName} மூலம் திற
armenian.OpenAudio=Բացել երաժշտությունը {#AppName}-ով
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
catalan.OpenPlaylists=Obre les llistes de reproducció amb el {#AppName}
danish.OpenPlaylists=Åbn spillelister med {#AppName}
finnish.OpenPlaylists=Avaa soittolistat sovelluksella {#AppName}
hungarian.OpenPlaylists=Lejátszólisták megnyitása a {#AppName} programmal
bulgarian.OpenPlaylists=Отваряне на плейлисти с {#AppName}
slovak.OpenPlaylists=Otvárať playlisty v {#AppName}
slovenian.OpenPlaylists=Odpiraj sezname predvajanja z {#AppName}
norwegian.OpenPlaylists=Åpne spillelister med {#AppName}
thai.OpenPlaylists=เปิดผังเล่นด้วย {#AppName}
tamil.OpenPlaylists=தொகுப்பு பட்டியல்களை {#AppName} மூலம் திற
armenian.OpenPlaylists=Բացել երգացանկերը {#AppName}-ով
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
catalan.ContextMenu=Reprodueix amb el {#AppName} als menús contextuals
danish.ContextMenu=Afspil med {#AppName} i højrekliksmenuer
finnish.ContextMenu=Toista sovelluksella {#AppName} pikavalikoissa
hungarian.ContextMenu=„Lejátszás a {#AppName} programmal” a helyi menükben
bulgarian.ContextMenu=„Възпроизвеждане с {#AppName}“ в контекстните менюта
slovak.ContextMenu=Prehrať pomocou {#AppName} v kontextových menu
slovenian.ContextMenu=Predvajaj z {#AppName} v priročnih menijih
norwegian.ContextMenu=Spill av med {#AppName} i høyreklikkmenyer
thai.ContextMenu=เล่นด้วย {#AppName} ในเมนูคลิกขวา
tamil.ContextMenu=வலது-சொடுக்கு மெனுக்களில் {#AppName} மூலம் இயக்கு
armenian.ContextMenu=«Նվագարկել {#AppName}-ով» աջ սեղմման մենյուներում
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
catalan.DesktopIcon=Drecera a l'escriptori
danish.DesktopIcon=Genvej på skrivebordet
finnish.DesktopIcon=Työpöydän pikakuvake
hungarian.DesktopIcon=Asztali parancsikon
bulgarian.DesktopIcon=Пряк път на работния плот
slovak.DesktopIcon=Odkaz na pracovnej ploche
slovenian.DesktopIcon=Bližnjica na namizju
norwegian.DesktopIcon=Snarvei på skrivebordet
thai.DesktopIcon=ทางลัดบนเดสก์ท็อป
tamil.DesktopIcon=மேசைத்தளக் குறுக்குவழி
armenian.DesktopIcon=Աշխատասեղանի դյուրանցում
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
catalan.LayoutTitle=Disposició
danish.LayoutTitle=Layout
finnish.LayoutTitle=Asettelu
hungarian.LayoutTitle=Elrendezés
bulgarian.LayoutTitle=Оформление
slovak.LayoutTitle=Rozloženie
slovenian.LayoutTitle=Postavitev
norwegian.LayoutTitle=Oppsett
thai.LayoutTitle=การจัดวาง
tamil.LayoutTitle=தளவமைப்பு
armenian.LayoutTitle=Դասավորություն
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
catalan.LayoutText=D'esquerra a dreta, com en el disseny, o en mirall.
danish.LayoutText=Venstre mod højre som i designet, eller spejlvendt.
finnish.LayoutText=Vasemmalta oikealle kuten suunniteltu, tai peilattuna.
hungarian.LayoutText=Balról jobbra, ahogy tervezték, vagy tükrözve.
bulgarian.LayoutText=Отляво надясно, както е в дизайна, или огледално.
slovak.LayoutText=Zľava doprava podľa návrhu, alebo zrkadlovo.
slovenian.LayoutText=Od leve proti desni, kot je oblikovano, ali zrcalno.
norwegian.LayoutText=Venstre mot høyre som i designet, eller speilvendt.
thai.LayoutText=ซ้ายไปขวาตามที่ออกแบบ หรือกลับด้าน
tamil.LayoutText=வடிவமைத்தபடி இடமிருந்து வலம், அல்லது பிரதிபலித்தபடி.
armenian.LayoutText=Ձախից աջ՝ ինչպես դիզայնում, կամ հայելային։
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
catalan.LayoutRTL=Disposició de dreta a esquerra
danish.LayoutRTL=Layout fra højre mod venstre
finnish.LayoutRTL=Asettelu oikealta vasemmalle
hungarian.LayoutRTL=Jobbról balra haladó elrendezés
bulgarian.LayoutRTL=Оформление отдясно наляво
slovak.LayoutRTL=Rozloženie sprava doľava
slovenian.LayoutRTL=Postavitev od desne proti levi
norwegian.LayoutRTL=Oppsett fra høyre mot venstre
thai.LayoutRTL=การจัดวางขวาไปซ้าย
tamil.LayoutRTL=வலமிருந்து இடம் தளவமைப்பு
armenian.LayoutRTL=Աջից ձախ դասավորություն
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
catalan.LayoutRTLText=La barra lateral a la dreta i els controls en ordre invers, per llegir de dreta a esquerra. Desactivat: la disposició del disseny, d'esquerra a dreta.
danish.LayoutRTLText=Sidepanelet til højre og kontrollerne i omvendt rækkefølge, til læsning fra højre mod venstre. Fra: designets layout, fra venstre mod højre.
finnish.LayoutRTLText=Sivupalkki oikealla ja ohjaimet käänteisessä järjestyksessä oikealta vasemmalle lukemista varten. Pois: suunnittelun asettelu vasemmalta oikealle.
hungarian.LayoutRTLText=Az oldalsáv jobbra kerül, a vezérlők fordított sorrendbe, jobbról balra olvasáshoz. Kikapcsolva: a terv eredeti, balról jobbra haladó elrendezése.
bulgarian.LayoutRTLText=Страничната лента вдясно и контролите в обратен ред, за четене отдясно наляво. Изключено: оформлението на дизайна, отляво надясно.
slovak.LayoutRTLText=Bočný panel vpravo a ovládacie prvky v opačnom poradí, na čítanie sprava doľava. Vypnuté: rozloženie podľa návrhu, zľava doprava.
slovenian.LayoutRTLText=Stranska vrstica na desni in kontrolniki v obratnem vrstnem redu, za branje od desne proti levi. Izklopljeno: oblikovana postavitev, od leve proti desni.
norwegian.LayoutRTLText=Sidepanelet til høyre og kontrollene i omvendt rekkefølge, for lesing fra høyre mot venstre. Av: designets oppsett, fra venstre mot høyre.
thai.LayoutRTLText=แถบด้านข้างอยู่ทางขวาและตัวควบคุมเรียงกลับลำดับ สำหรับการอ่านจากขวาไปซ้าย ปิด: การจัดวางตามที่ออกแบบ จากซ้ายไปขวา
tamil.LayoutRTLText=வலமிருந்து இடமாகப் படிப்பதற்காக, பக்கப்பட்டி வலதுபுறமும் கட்டுப்பாடுகள் தலைகீழ் வரிசையிலும். முடக்கம்: வடிவமைப்பின் தளவமைப்பு, இடமிருந்து வலம்.
armenian.LayoutRTLText=Կողային վահանակը աջ կողմում, կառավարման տարրերը՝ հակառակ հերթականությամբ, աջից ձախ կարդալու համար։ Անջատված՝ դիզայնի դասավորությունը՝ ձախից աջ։
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
catalan.LayoutCardTitle=Canvieu-ho quan vulgueu
danish.LayoutCardTitle=Skift når som helst
finnish.LayoutCardTitle=Muuta milloin tahansa
hungarian.LayoutCardTitle=Bármikor módosítható
bulgarian.LayoutCardTitle=Променете го по всяко време
slovak.LayoutCardTitle=Môžete to kedykoľvek zmeniť
slovenian.LayoutCardTitle=Spremenite lahko kadar koli
norwegian.LayoutCardTitle=Endre det når som helst
thai.LayoutCardTitle=เปลี่ยนได้ทุกเมื่อ
tamil.LayoutCardTitle=எப்போது வேண்டுமானாலும் மாற்றலாம்
armenian.LayoutCardTitle=Կարող եք փոխել ցանկացած պահի
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
catalan.LayoutCardText=Paràmetres → Aparença: Direcció de la disposició. L'idioma de l'aplicació també hi és, a Idioma de la interfície.
danish.LayoutCardText=Indstillinger → Udseende: Layoutretning. Appens sprog findes også der, under Brugerfladesprog.
finnish.LayoutCardText=Asetukset → Ulkoasu: Asettelun suunta. Sovelluksen kieli on samassa paikassa, kohdassa Käyttöliittymän kieli.
hungarian.LayoutCardText=Beállítások → Megjelenés: Elrendezés iránya. Az alkalmazás nyelve is itt található, a Kezelőfelület nyelve pontban.
bulgarian.LayoutCardText=Настройки → Външен вид: Посока на оформлението. Там е и езикът на приложението, в „Език на интерфейса“.
slovak.LayoutCardText=Nastavenia → Vzhľad: Smer rozloženia. Nájdete tam aj jazyk aplikácie, v položke Jazyk rozhrania.
slovenian.LayoutCardText=Nastavitve → Videz: Smer postavitve. Tam je tudi jezik programa, pod Jezik vmesnika.
norwegian.LayoutCardText=Innstillinger → Utseende: Oppsettsretning. Språket i appen finner du der også, under Grensesnittspråk.
thai.LayoutCardText=ค่าตั้ง → รูปลักษณ์: ทิศทางการจัดวาง ภาษาของโปรแกรมก็อยู่ที่นั่นด้วย ในภาษาของส่วนติดต่อ
tamil.LayoutCardText=அமைப்புகள் → தோற்றம்: தளவமைப்புத் திசை. பயன்பாட்டின் மொழியும் அங்கேயே, இடைமுக மொழியில் உள்ளது.
armenian.LayoutCardText=Կարգավորումներ → Արտաքին տեսք՝ Դասավորության ուղղություն։ Հավելվածի լեզուն նույնպես այնտեղ է՝ Ինտերֆեյսի լեզու կետում։
english.LanguagesTitle=Languages
hebrew.LanguagesTitle=שפות
brazilianportuguese.LanguagesTitle=Idiomas
dutch.LanguagesTitle=Talen
german.LanguagesTitle=Sprachen
czech.LanguagesTitle=Jazyky
swedish.LanguagesTitle=Språk
portuguese.LanguagesTitle=Idiomas
french.LanguagesTitle=Langues
korean.LanguagesTitle=언어
japanese.LanguagesTitle=言語
italian.LanguagesTitle=Lingue
spanish.LanguagesTitle=Idiomas
polish.LanguagesTitle=Języki
ukrainian.LanguagesTitle=Мови
russian.LanguagesTitle=Языки
arabic.LanguagesTitle=اللغات
turkish.LanguagesTitle=Diller
catalan.LanguagesTitle=Idiomes
danish.LanguagesTitle=Sprog
finnish.LanguagesTitle=Kielet
hungarian.LanguagesTitle=Nyelvek
bulgarian.LanguagesTitle=Езици
slovak.LanguagesTitle=Jazyky
slovenian.LanguagesTitle=Jeziki
norwegian.LanguagesTitle=Språk
thai.LanguagesTitle=ภาษา
tamil.LanguagesTitle=மொழிகள்
armenian.LanguagesTitle=Լեզուներ
english.LanguagesText=The language of the app and of your subtitles.
hebrew.LanguagesText=השפה של הנגן ושל הכתוביות שלך.
brazilianportuguese.LanguagesText=O idioma do app e das suas legendas.
dutch.LanguagesText=De taal van de app en van je ondertitels.
german.LanguagesText=Die Sprache der App und deiner Untertitel.
czech.LanguagesText=Jazyk aplikace a vašich titulků.
swedish.LanguagesText=Språket för appen och för dina undertexter.
portuguese.LanguagesText=O idioma da aplicação e das suas legendas.
french.LanguagesText=La langue de l'application et de vos sous-titres.
korean.LanguagesText=앱과 자막의 언어입니다.
japanese.LanguagesText=アプリと字幕の言語です。
italian.LanguagesText=La lingua dell'app e dei tuoi sottotitoli.
spanish.LanguagesText=El idioma de la aplicación y de tus subtítulos.
polish.LanguagesText=Język aplikacji i Twoich napisów.
ukrainian.LanguagesText=Мова застосунку та ваших субтитрів.
russian.LanguagesText=Язык приложения и ваших субтитров.
arabic.LanguagesText=لغة التطبيق ولغة ترجماتك.
turkish.LanguagesText=Uygulamanın ve altyazılarınızın dili.
catalan.LanguagesText=L'idioma de l'aplicació i dels vostres subtítols.
danish.LanguagesText=Sproget i appen og i dine undertekster.
finnish.LanguagesText=Sovelluksen ja tekstitystesi kieli.
hungarian.LanguagesText=Az alkalmazás és a feliratok nyelve.
bulgarian.LanguagesText=Езикът на приложението и на субтитрите ви.
slovak.LanguagesText=Jazyk aplikácie a vašich titulkov.
slovenian.LanguagesText=Jezik aplikacije in vaših podnapisov.
norwegian.LanguagesText=Språket i appen og i undertekstene dine.
thai.LanguagesText=ภาษาของแอปและคำบรรยายของคุณ
tamil.LanguagesText=பயன்பாட்டின் மற்றும் உங்கள் வசனங்களின் மொழி.
armenian.LanguagesText=Հավելվածի և ձեր ենթագրերի լեզուն։
english.LanguagesInterface=Interface language
hebrew.LanguagesInterface=שפת הממשק
brazilianportuguese.LanguagesInterface=Idioma da interface
dutch.LanguagesInterface=Taal van de interface
german.LanguagesInterface=Sprache der Oberfläche
czech.LanguagesInterface=Jazyk rozhraní
swedish.LanguagesInterface=Gränssnittets språk
portuguese.LanguagesInterface=Idioma da interface
french.LanguagesInterface=Langue de l'interface
korean.LanguagesInterface=인터페이스 언어
japanese.LanguagesInterface=インターフェースの言語
italian.LanguagesInterface=Lingua dell'interfaccia
spanish.LanguagesInterface=Idioma de la interfaz
polish.LanguagesInterface=Język interfejsu
ukrainian.LanguagesInterface=Мова інтерфейсу
russian.LanguagesInterface=Язык интерфейса
arabic.LanguagesInterface=لغة الواجهة
turkish.LanguagesInterface=Arayüz dili
catalan.LanguagesInterface=Idioma de la interfície
danish.LanguagesInterface=Grænsefladens sprog
finnish.LanguagesInterface=Käyttöliittymän kieli
hungarian.LanguagesInterface=A felület nyelve
bulgarian.LanguagesInterface=Език на интерфейса
slovak.LanguagesInterface=Jazyk rozhrania
slovenian.LanguagesInterface=Jezik vmesnika
norwegian.LanguagesInterface=Grensesnittspråk
thai.LanguagesInterface=ภาษาของอินเทอร์เฟซ
tamil.LanguagesInterface=இடைமுக மொழி
armenian.LanguagesInterface=Միջերեսի լեզուն
english.LanguagesSubtitles=Subtitle language
hebrew.LanguagesSubtitles=שפת הכתוביות
brazilianportuguese.LanguagesSubtitles=Idioma das legendas
dutch.LanguagesSubtitles=Taal van de ondertitels
german.LanguagesSubtitles=Sprache der Untertitel
czech.LanguagesSubtitles=Jazyk titulků
swedish.LanguagesSubtitles=Undertexternas språk
portuguese.LanguagesSubtitles=Idioma das legendas
french.LanguagesSubtitles=Langue des sous-titres
korean.LanguagesSubtitles=자막 언어
japanese.LanguagesSubtitles=字幕の言語
italian.LanguagesSubtitles=Lingua dei sottotitoli
spanish.LanguagesSubtitles=Idioma de los subtítulos
polish.LanguagesSubtitles=Język napisów
ukrainian.LanguagesSubtitles=Мова субтитрів
russian.LanguagesSubtitles=Язык субтитров
arabic.LanguagesSubtitles=لغة الترجمة
turkish.LanguagesSubtitles=Altyazı dili
catalan.LanguagesSubtitles=Idioma dels subtítols
danish.LanguagesSubtitles=Underteksternes sprog
finnish.LanguagesSubtitles=Tekstityksen kieli
hungarian.LanguagesSubtitles=A feliratok nyelve
bulgarian.LanguagesSubtitles=Език на субтитрите
slovak.LanguagesSubtitles=Jazyk titulkov
slovenian.LanguagesSubtitles=Jezik podnapisov
norwegian.LanguagesSubtitles=Undertekstspråk
thai.LanguagesSubtitles=ภาษาคำบรรยาย
tamil.LanguagesSubtitles=வசன மொழி
armenian.LanguagesSubtitles=Ենթագրերի լեզուն
english.LanguagesSubtitlesText=Subtitles in this language show by themselves: from the video, a file next to it, or online. Change both any time in Settings.
hebrew.LanguagesSubtitlesText=כתוביות בשפה זו מוצגות מעצמן: מהסרטון, מקובץ לידו או מהאינטרנט. אפשר לשנות את שתיהן בכל עת בהגדרות.
brazilianportuguese.LanguagesSubtitlesText=Legendas neste idioma aparecem sozinhas: do vídeo, de um arquivo ao lado dele ou da internet. Mude os dois quando quiser em Configurações.
dutch.LanguagesSubtitlesText=Ondertitels in deze taal verschijnen vanzelf: uit de video, uit een bestand ernaast of online. Wijzig beide altijd in Instellingen.
german.LanguagesSubtitlesText=Untertitel in dieser Sprache erscheinen von selbst: aus dem Video, aus einer Datei daneben oder online. Beides lässt sich jederzeit in den Einstellungen ändern.
czech.LanguagesSubtitlesText=Titulky v tomto jazyce se zobrazí samy: z videa, ze souboru vedle něj nebo z internetu. Obojí můžete kdykoli změnit v Nastavení.
swedish.LanguagesSubtitlesText=Undertexter på det här språket visas av sig själva: från videon, från en fil bredvid den eller från nätet. Ändra båda när som helst i Inställningar.
portuguese.LanguagesSubtitlesText=As legendas neste idioma aparecem sozinhas: do vídeo, de um ficheiro ao lado dele ou da Internet. Altere ambos quando quiser nas Definições.
french.LanguagesSubtitlesText=Les sous-titres dans cette langue s'affichent d'eux-mêmes : depuis la vidéo, un fichier à côté ou en ligne. Modifiez les deux à tout moment dans les Paramètres.
korean.LanguagesSubtitlesText=이 언어의 자막은 동영상, 옆에 있는 파일 또는 온라인에서 자동으로 표시됩니다. 둘 다 언제든지 설정에서 바꿀 수 있습니다.
japanese.LanguagesSubtitlesText=この言語の字幕は、動画、隣にあるファイル、またはオンラインから自動で表示されます。どちらもいつでも設定で変更できます。
italian.LanguagesSubtitlesText=I sottotitoli in questa lingua compaiono da soli: dal video, da un file accanto o online. Puoi cambiarle entrambe in qualsiasi momento in Impostazioni.
spanish.LanguagesSubtitlesText=Los subtítulos en este idioma aparecen solos: del vídeo, de un archivo junto a él o de internet. Cambia ambos cuando quieras en Configuración.
polish.LanguagesSubtitlesText=Napisy w tym języku pojawiają się same: z filmu, z pliku obok niego lub z internetu. Oba możesz zmienić w dowolnej chwili w Ustawieniach.
ukrainian.LanguagesSubtitlesText=Субтитри цією мовою з’являються самі: з відео, з файлу поруч із ним або з інтернету. Обидві можна будь-коли змінити в Налаштуваннях.
russian.LanguagesSubtitlesText=Субтитры на этом языке появляются сами: из видео, из файла рядом с ним или из интернета. Оба можно изменить в любой момент в Настройках.
arabic.LanguagesSubtitlesText=تظهر الترجمات بهذه اللغة تلقائيًا: من الفيديو أو من ملف بجانبه أو من الإنترنت. يمكنك تغيير كلتيهما في أي وقت من الإعدادات.
turkish.LanguagesSubtitlesText=Bu dildeki altyazılar kendiliğinden gösterilir: videodan, yanındaki bir dosyadan veya internetten. İkisini de istediğiniz zaman Ayarlar'dan değiştirebilirsiniz.
catalan.LanguagesSubtitlesText=Els subtítols en aquest idioma es mostren sols: del vídeo, d'un fitxer al costat o d'internet. Podeu canviar tots dos quan vulgueu a la Configuració.
danish.LanguagesSubtitlesText=Undertekster på dette sprog vises af sig selv: fra videoen, fra en fil ved siden af den eller online. Skift begge når som helst under Indstillinger.
finnish.LanguagesSubtitlesText=Tämänkieliset tekstitykset näkyvät itsestään: videosta, sen vieressä olevasta tiedostosta tai verkosta. Voit vaihtaa molemmat milloin tahansa asetuksista.
hungarian.LanguagesSubtitlesText=Az ilyen nyelvű feliratok maguktól megjelennek: a videóból, a mellette lévő fájlból vagy az internetről. Mindkettő bármikor módosítható a Beállításokban.
bulgarian.LanguagesSubtitlesText=Субтитрите на този език се показват сами: от видеото, от файл до него или от интернет. Можете да промените и двата по всяко време в Настройки.
slovak.LanguagesSubtitlesText=Titulky v tomto jazyku sa zobrazia samy: z videa, zo súboru vedľa neho alebo z internetu. Oboje môžete kedykoľvek zmeniť v Nastaveniach.
slovenian.LanguagesSubtitlesText=Podnapisi v tem jeziku se prikažejo sami: iz videa, iz datoteke ob njem ali s spleta. Oba lahko kadar koli spremenite v Nastavitvah.
norwegian.LanguagesSubtitlesText=Undertekster på dette språket vises av seg selv: fra videoen, fra en fil ved siden av den eller fra nettet. Endre begge når som helst i Innstillinger.
thai.LanguagesSubtitlesText=คำบรรยายในภาษานี้จะแสดงเอง: จากวิดีโอ จากไฟล์ที่อยู่ข้างๆ หรือจากออนไลน์ เปลี่ยนทั้งสองได้ทุกเมื่อในการตั้งค่า
tamil.LanguagesSubtitlesText=இந்த மொழியிலான வசனங்கள் தானாகவே தோன்றும்: காணொளியிலிருந்து, அதன் அருகிலுள்ள கோப்பிலிருந்து அல்லது இணையத்திலிருந்து. இரண்டையும் எப்போது வேண்டுமானாலும் அமைப்புகளில் மாற்றலாம்.
armenian.LanguagesSubtitlesText=Այս լեզվով ենթագրերը ցուցադրվում են ինքնաբերաբար՝ տեսանյութից, կողքի ֆայլից կամ առցանց։ Երկուսն էլ կարող եք փոխել ցանկացած պահի Կարգավորումներում։
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
catalan.PrivacyTitle=Privadesa
danish.PrivacyTitle=Privatliv
finnish.PrivacyTitle=Tietosuoja
hungarian.PrivacyTitle=Adatvédelem
bulgarian.PrivacyTitle=Поверителност
slovak.PrivacyTitle=Súkromie
slovenian.PrivacyTitle=Zasebnost
norwegian.PrivacyTitle=Personvern
thai.PrivacyTitle=ความเป็นส่วนตัว
tamil.PrivacyTitle=தனியுரிமை
armenian.PrivacyTitle=Գաղտնիություն
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
catalan.PrivacyText=Dues funcions es connecten a Internet pel seu compte. Cadascuna només envia això:
danish.PrivacyText=To funktioner går online af sig selv. Hver sender kun dette:
finnish.PrivacyText=Kaksi toimintoa muodostaa yhteyden verkkoon itsestään. Kumpikin lähettää vain tämän:
hungarian.PrivacyText=Két funkció magától csatlakozik az internethez. Mindkettő csak ezt küldi el:
bulgarian.PrivacyText=Две функции се свързват с интернет сами. Всяка изпраща само това:
slovak.PrivacyText=Dve funkcie sa pripájajú na internet samy. Každá odosiela iba toto:
slovenian.PrivacyText=Dve funkciji se z internetom povežeta sami. Vsaka pošlje samo to:
norwegian.PrivacyText=To funksjoner kobler seg til Internett av seg selv. Hver av dem sender bare dette:
thai.PrivacyText=มีสองคุณสมบัติที่เชื่อมต่อออนไลน์เอง แต่ละอย่างส่งเพียงข้อมูลนี้:
tamil.PrivacyText=இரண்டு அம்சங்கள் தாமாகவே இணையத்துக்குச் செல்கின்றன. ஒவ்வொன்றும் இதை மட்டுமே அனுப்புகிறது:
armenian.PrivacyText=Երկու գործառույթ ինքնուրույն են միանում ինտերնետին։ Յուրաքանչյուրն ուղարկում է միայն հետևյալը:
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
catalan.PrivacyUpdates=Cerca actualitzacions un cop al dia
danish.PrivacyUpdates=Søg efter opdateringer én gang om dagen
finnish.PrivacyUpdates=Tarkista päivitykset kerran päivässä
hungarian.PrivacyUpdates=Frissítések keresése naponta egyszer
bulgarian.PrivacyUpdates=Проверка за актуализации веднъж дневно
slovak.PrivacyUpdates=Kontrolovať aktualizácie raz denne
slovenian.PrivacyUpdates=Enkrat dnevno preveri posodobitve
norwegian.PrivacyUpdates=Se etter oppdateringer én gang om dagen
thai.PrivacyUpdates=ตรวจหาการปรับปรุงวันละครั้ง
tamil.PrivacyUpdates=தினமும் ஒருமுறை புதுப்பிப்புகளைச் சரிபார்
armenian.PrivacyUpdates=Ստուգել թարմացումները օրը մեկ անգամ
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
catalan.PrivacyUpdatesText=Una sol·licitud de la darrera versió a GitHub (api.github.com).
danish.PrivacyUpdatesText=En forespørgsel om den nyeste udgivelse til GitHub (api.github.com).
finnish.PrivacyUpdatesText=Pyyntö uusimmasta julkaisusta GitHubille (api.github.com).
hungarian.PrivacyUpdatesText=Egy kérést a legújabb kiadásért a GitHubnak (api.github.com).
bulgarian.PrivacyUpdatesText=Заявка за последната версия към GitHub (api.github.com).
slovak.PrivacyUpdatesText=Dopyt na najnovšie vydanie, odoslaný na GitHub (api.github.com).
slovenian.PrivacyUpdatesText=Zahteva za najnovejšo izdajo, poslana storitvi GitHub (api.github.com).
norwegian.PrivacyUpdatesText=En forespørsel om den nyeste utgivelsen, til GitHub (api.github.com).
thai.PrivacyUpdatesText=คำขอรุ่นเผยแพร่ล่าสุด ส่งไปยัง GitHub (api.github.com)
tamil.PrivacyUpdatesText=சமீபத்திய வெளியீட்டுக்கான கோரிக்கை, GitHub-க்கு (api.github.com).
armenian.PrivacyUpdatesText=Վերջին թողարկման հարցում GitHub-ին (api.github.com)։
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
catalan.PrivacySubtitles=Cerca subtítols en línia automàticament
danish.PrivacySubtitles=Find undertekster online automatisk
finnish.PrivacySubtitles=Hae tekstitykset verkosta automaattisesti
hungarian.PrivacySubtitles=Feliratok automatikus keresése az interneten
bulgarian.PrivacySubtitles=Автоматично търсене на субтитри онлайн
slovak.PrivacySubtitles=Automaticky hľadať titulky online
slovenian.PrivacySubtitles=Samodejno poišči podnapise na spletu
norwegian.PrivacySubtitles=Finn undertekster på nettet automatisk
thai.PrivacySubtitles=ค้นหาบทบรรยายทางออนไลน์โดยอัตโนมัติ
tamil.PrivacySubtitles=வசனங்களை இணையத்தில் தானாகக் கண்டறி
armenian.PrivacySubtitles=Ավտոմատ գտնել ենթագրեր առցանց
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
catalan.PrivacySubtitlesText=El nom de la pel·lícula i una empremta del fitxer, als serveis de subtítols (OpenSubtitles.org, Wizdom, Gestdown, YIFY); el seu nom, a IMDb, Wikidata i Cinemeta.
danish.PrivacySubtitlesText=Filmens navn og et fingeraftryk af filen til underteksttjenester (OpenSubtitles.org, Wizdom, Gestdown, YIFY); navnet til IMDb, Wikidata og Cinemeta.
finnish.PrivacySubtitlesText=Elokuvan nimi ja tiedoston sormenjälki tekstityspalveluille (OpenSubtitles.org, Wizdom, Gestdown, YIFY); sen nimi IMDb:lle, Wikidatalle ja Cinemetalle.
hungarian.PrivacySubtitlesText=A film nevét és a fájl ujjlenyomatát a feliratszolgáltatásoknak (OpenSubtitles.org, Wizdom, Gestdown, YIFY); a nevét az IMDb, a Wikidata és a Cinemeta szolgáltatásnak.
bulgarian.PrivacySubtitlesText=Името на филма и отпечатък на файла към услугите за субтитри (OpenSubtitles.org, Wizdom, Gestdown, YIFY); името му към IMDb, Wikidata и Cinemeta.
slovak.PrivacySubtitlesText=Názov filmu a odtlačok súboru titulkovým službám (OpenSubtitles.org, Wizdom, Gestdown, YIFY); názov aj do IMDb, Wikidata a Cinemeta.
slovenian.PrivacySubtitlesText=Ime filma in prstni odtis datoteke storitvam za podnapise (OpenSubtitles.org, Wizdom, Gestdown, YIFY); njegovo ime storitvam IMDb, Wikidata in Cinemeta.
norwegian.PrivacySubtitlesText=Filmens navn og et fingeravtrykk av filen, til undertekstjenester (OpenSubtitles.org, Wizdom, Gestdown, YIFY); navnet til IMDb, Wikidata og Cinemeta.
thai.PrivacySubtitlesText=ชื่อภาพยนตร์และลายนิ้วมือของแฟ้ม ส่งไปยังบริการบทบรรยาย (OpenSubtitles.org, Wizdom, Gestdown, YIFY) และชื่อส่งไปยัง IMDb, Wikidata และ Cinemeta
tamil.PrivacySubtitlesText=திரைப்படத்தின் பெயரும் கோப்பின் கைரேகையும், வசனச் சேவைகளுக்கு (OpenSubtitles.org, Wizdom, Gestdown, YIFY); அதன் பெயர் IMDb, Wikidata, Cinemeta ஆகியவற்றுக்கு.
armenian.PrivacySubtitlesText=Ֆիլմի անունը և ֆայլի մատնահետքը՝ ենթագրերի ծառայություններին (OpenSubtitles.org, Wizdom, Gestdown, YIFY), անունը՝ IMDb-ին, Wikidata-ին և Cinemeta-ին։
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
catalan.PrivacyCardTitle=Sense telemetria, sense anuncis, sense compte
danish.PrivacyCardTitle=Ingen telemetri, ingen reklamer, ingen konto
finnish.PrivacyCardTitle=Ei telemetriaa, ei mainoksia, ei tiliä
hungarian.PrivacyCardTitle=Nincs telemetria, nincs reklám, nincs fiók
bulgarian.PrivacyCardTitle=Без телеметрия, без реклами, без акаунт
slovak.PrivacyCardTitle=Žiadna telemetria, žiadne reklamy, žiadny účet
slovenian.PrivacyCardTitle=Brez telemetrije, brez oglasov, brez računa
norwegian.PrivacyCardTitle=Ingen telemetri, ingen reklame, ingen konto
thai.PrivacyCardTitle=ไม่เก็บข้อมูลการใช้งาน ไม่มีโฆษณา ไม่ต้องมีบัญชี
tamil.PrivacyCardTitle=தொலைஅளவீடு இல்லை, விளம்பரங்கள் இல்லை, கணக்கு இல்லை
armenian.PrivacyCardTitle=Առանց հեռաչափության, գովազդի և հաշվի
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
catalan.PrivacyCardText=No es connecta res més a Internet tret que ho demaneu. Podeu canviar totes dues opcions més endavant a Paràmetres.
danish.PrivacyCardText=Intet andet går online, medmindre du beder om det. Du kan ændre begge senere i Indstillinger.
finnish.PrivacyCardText=Mikään muu ei mene verkkoon, ellet itse pyydä. Voit muuttaa molempia myöhemmin asetuksista.
hungarian.PrivacyCardText=Semmi más nem kerül az internetre, hacsak Ön nem kéri. Mindkettő később módosítható a Beállításokban.
bulgarian.PrivacyCardText=Нищо друго не се свързва с интернет, освен ако не поискате. Можете да промените и двете по-късно в Настройки.
slovak.PrivacyCardText=Nič iné sa na internet nepripojí, pokiaľ o to nepožiadate. Obe voľby môžete neskôr zmeniť v Nastaveniach.
slovenian.PrivacyCardText=Nič drugega se ne poveže z internetom, razen če to zahtevate. Obe možnosti lahko pozneje spremenite v Nastavitvah.
norwegian.PrivacyCardText=Ingenting annet kobler seg til Internett med mindre du ber om det. Endre begge senere i Innstillinger.
thai.PrivacyCardText=ไม่มีสิ่งอื่นใดออนไลน์นอกจากที่คุณขอ เปลี่ยนทั้งสองอย่างได้ภายหลังในค่าตั้ง
tamil.PrivacyCardText=நீங்கள் கேட்காத வரை வேறு எதுவும் இணையத்துக்குச் செல்லாது. இரண்டையும் பின்னர் அமைப்புகளில் மாற்றலாம்.
armenian.PrivacyCardText=Ուրիշ ոչինչ ինտերնետին չի միանում, եթե դուք չխնդրեք։ Երկուսն էլ կարող եք հետագայում փոխել Կարգավորումներում։
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
catalan.LinkPrivacy=Política de privadesa i de signatura de codi
danish.LinkPrivacy=Politik for privatliv og kodesignering
finnish.LinkPrivacy=Tietosuoja- ja koodinallekirjoituskäytäntö
hungarian.LinkPrivacy=Adatvédelmi és kódaláírási irányelvek
bulgarian.LinkPrivacy=Политика за поверителност и подписване на кода
slovak.LinkPrivacy=Zásady ochrany súkromia a podpisovania kódu
slovenian.LinkPrivacy=Pravilnik o zasebnosti in podpisovanju kode
norwegian.LinkPrivacy=Retningslinjer for personvern og kodesignering
thai.LinkPrivacy=นโยบายความเป็นส่วนตัวและการลงนามโค้ด
tamil.LinkPrivacy=தனியுரிமை மற்றும் குறியீட்டுக் கையொப்பக் கொள்கை
armenian.LinkPrivacy=Գաղտնիության և կոդի ստորագրման քաղաքականություն
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
catalan.InstallingTitle=S'està instal·lant
danish.InstallingTitle=Installerer
finnish.InstallingTitle=Asennetaan
hungarian.InstallingTitle=Telepítés folyamatban
bulgarian.InstallingTitle=Инсталиране
slovak.InstallingTitle=Inštaluje sa
slovenian.InstallingTitle=Nameščanje
norwegian.InstallingTitle=Installerer
thai.InstallingTitle=กำลังติดตั้ง
tamil.InstallingTitle=நிறுவுகிறது
armenian.InstallingTitle=Տեղադրում
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
catalan.InstallingText=Triga aproximadament un minut.
danish.InstallingText=Det tager omkring et minut.
finnish.InstallingText=Tämä kestää noin minuutin.
hungarian.InstallingText=Ez körülbelül egy percig tart.
bulgarian.InstallingText=Това отнема около минута.
slovak.InstallingText=Potrvá to asi minútu.
slovenian.InstallingText=To traja približno minuto.
norwegian.InstallingText=Dette tar omtrent ett minutt.
thai.InstallingText=ใช้เวลาประมาณหนึ่งนาที
tamil.InstallingText=இதற்குச் சுமார் ஒரு நிமிடம் ஆகும்.
armenian.InstallingText=Սա տևում է մոտ մեկ րոպե։
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
catalan.TipTitle=Feu vostre el reproductor
danish.TipTitle=Gør afspilleren til din egen
finnish.TipTitle=Tee soittimesta omasi
hungarian.TipTitle=Szabja testre a lejátszót
bulgarian.TipTitle=Направете плейъра свой
slovak.TipTitle=Prispôsobte si prehrávač
slovenian.TipTitle=Prilagodite si predvajalnik
norwegian.TipTitle=Gjør spilleren til din egen
thai.TipTitle=ปรับแต่งตัวเล่นในแบบของคุณ
tamil.TipTitle=இயக்கியை உங்களுடையதாக்குங்கள்
armenian.TipTitle=Դարձրեք նվագարկիչը ձերը
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
catalan.TipText=Arrossegueu botons cap a la barra de control i fora d'aquesta amb ⋯ → Personalitza el reproductor, i deseu les vostres disposicions com a perfils.
danish.TipText=Træk knapper ind på og ud af kontrollinjen med ⋯ → Tilpas afspiller, og gem dine layouts som profiler.
finnish.TipText=Vedä painikkeita ohjauspalkkiin ja sieltä pois kohdasta ⋯ → Mukauta soitinta, ja tallenna asettelusi profiileiksi.
hungarian.TipText=A ⋯ → Lejátszó testreszabása menüben gombokat húzhat a vezérlősávra és onnan el, az elrendezéseit pedig profilként mentheti.
bulgarian.TipText=Плъзгайте бутони в лентата за управление и извън нея чрез ⋯ → Персонализиране на плейъра и пазете оформленията си като профили.
slovak.TipText=Presúvajte tlačidlá na ovládací panel a z neho cez ⋯ → Prispôsobiť prehrávač a uchovajte svoje rozloženia ako profily.
slovenian.TipText=Gumbe vlecite v vrstico s kontrolniki in iz nje prek ⋯ → Prilagodi predvajalnik ter shranite svoje postavitve kot profile.
norwegian.TipText=Dra knapper inn i og ut av kontrollinjen med ⋯ → Tilpass spilleren, og ta vare på oppsettene dine som profiler.
thai.TipText=ลากปุ่มเข้าและออกจากแถบควบคุมได้ด้วย ⋯ → ปรับแต่งตัวเล่น และเก็บการจัดวางของคุณไว้เป็นโพรไฟล์
tamil.TipText=⋯ → இயக்கியைத் தனிப்பயனாக்கு மூலம் பொத்தான்களைக் கட்டுப்பாட்டுப் பட்டிக்குள்ளும் வெளியேயும் இழுத்து, உங்கள் தளவமைப்புகளைச் சுயவிவரங்களாக வைத்திருங்கள்.
armenian.TipText=Քաշեք կոճակները կառավարման վահանակի մեջ և դուրս՝ ⋯ → Անհատականացնել նվագարկիչը, և պահեք ձեր դասավորությունները որպես պրոֆիլներ։
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
catalan.DoneTitle=El {#AppName} està a punt
danish.DoneTitle={#AppName} er klar
finnish.DoneTitle={#AppName} on valmis
hungarian.DoneTitle=A {#AppName} készen áll
bulgarian.DoneTitle={#AppName} е готов
slovak.DoneTitle={#AppName} je pripravený
slovenian.DoneTitle={#AppName} je pripravljen
norwegian.DoneTitle={#AppName} er klar
thai.DoneTitle={#AppName} พร้อมแล้ว
tamil.DoneTitle={#AppName} தயார்
armenian.DoneTitle={#AppName}-ը պատրաստ է
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
catalan.DoneText=El trobareu al menú Inici.
danish.DoneText=Du finder den i menuen Start.
finnish.DoneText=Löydät sen Käynnistä-valikosta.
hungarian.DoneText=A Start menüben találja.
bulgarian.DoneText=Ще го намерите в менюто „Старт“.
slovak.DoneText=Nájdete ho v ponuke Štart.
slovenian.DoneText=Najdete ga v meniju Start.
norwegian.DoneText=Du finner den på Start-menyen.
thai.DoneText=หาได้ในเมนูเริ่ม
tamil.DoneText=தொடக்க மெனுவில் இதைக் காணலாம்.
armenian.DoneText=Այն կգտնեք «Մեկնարկ» ցանկում։
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
catalan.DefaultTitle=Feu-lo el vostre reproductor per defecte
danish.DefaultTitle=Gør den til din standardafspiller
finnish.DefaultTitle=Tee siitä oletussoittimesi
hungarian.DefaultTitle=Legyen ez az alapértelmezett lejátszó
bulgarian.DefaultTitle=Направете го плейър по подразбиране
slovak.DefaultTitle=Nastavte ho ako predvolený prehrávač
slovenian.DefaultTitle=Nastavite ga kot privzeti predvajalnik
norwegian.DefaultTitle=Gjør den til standardspilleren din
thai.DefaultTitle=ตั้งเป็นตัวเล่นปริยาย
tamil.DefaultTitle=இதை உங்கள் இயல்புநிலை இயக்கியாக்குங்கள்
armenian.DefaultTitle=Դարձրեք այն ձեր լռելյայն նվագարկիչը
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
catalan.DefaultText=Windows conserva les aplicacions que vau triar abans. Obriu Aplicacions per defecte i trieu el {#AppName} per al vostre contingut multimèdia.
danish.DefaultText=Windows beholder de apps, du valgte før. Åbn Standardapps, og vælg {#AppName} til dine medier.
finnish.DefaultText=Windows säilyttää aiemmin valitsemasi sovellukset. Avaa Oletussovellukset ja valitse {#AppName} mediallesi.
hungarian.DefaultText=A Windows megtartja a korábban választott alkalmazásokat. Nyissa meg az Alapértelmezett alkalmazások beállítást, és válassza a {#AppName} programot a médiafájljaihoz.
bulgarian.DefaultText=Windows запазва приложенията, които сте избрали преди. Отворете „Приложения по подразбиране“ и изберете {#AppName} за медиите си.
slovak.DefaultText=Windows si pamätá aplikácie, ktoré ste vybrali predtým. Otvorte Predvolené aplikácie a pre svoje médiá vyberte {#AppName}.
slovenian.DefaultText=Windows ohrani programe, ki ste jih izbrali prej. Odprite Privzete aplikacije in za svoje predstavne vsebine izberite {#AppName}.
norwegian.DefaultText=Windows beholder appene du valgte tidligere. Åpne Standardapper og velg {#AppName} for mediene dine.
thai.DefaultText=Windows จะคงโปรแกรมที่คุณเลือกไว้ก่อนหน้า เปิด “แอปเริ่มต้น” แล้วเลือก {#AppName} สำหรับสื่อของคุณ
tamil.DefaultText=நீங்கள் முன்பு தேர்ந்தெடுத்த பயன்பாடுகளை Windows வைத்திருக்கும். இயல்புநிலைப் பயன்பாடுகளைத் திறந்து, உங்கள் ஊடகத்துக்கு {#AppName}-ஐத் தேர்ந்தெடுக்கவும்.
armenian.DefaultText=Windows-ը պահպանում է նախկինում ընտրված հավելվածները։ Բացեք «Լռելյայն հավելվածներ»-ը և ընտրեք {#AppName}-ը ձեր մեդիայի համար։
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
catalan.OpenDefaultApps=Obre Aplicacions per defecte
danish.OpenDefaultApps=Åbn Standardapps
finnish.OpenDefaultApps=Avaa Oletussovellukset
hungarian.OpenDefaultApps=Alapértelmezett alkalmazások megnyitása
bulgarian.OpenDefaultApps=Приложения по подразбиране
slovak.OpenDefaultApps=Otvoriť Predvolené aplikácie
slovenian.OpenDefaultApps=Odpri Privzete aplikacije
norwegian.OpenDefaultApps=Åpne Standardapper
thai.OpenDefaultApps=เปิดแอปเริ่มต้น
tamil.OpenDefaultApps=இயல்புநிலைப் பயன்பாடுகளைத் திற
armenian.OpenDefaultApps=Լռելյայն հավելվածներ
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
catalan.LaunchNow=Obre el {#AppName} ara
danish.LaunchNow=Åbn {#AppName} nu
finnish.LaunchNow=Avaa {#AppName} nyt
hungarian.LaunchNow=A {#AppName} megnyitása most
bulgarian.LaunchNow=Отваряне на {#AppName} сега
slovak.LaunchNow=Spustiť {#AppName} teraz
slovenian.LaunchNow=Zaženi {#AppName} zdaj
norwegian.LaunchNow=Åpne {#AppName} nå
thai.LaunchNow=เปิด {#AppName} ตอนนี้
tamil.LaunchNow={#AppName}-ஐ இப்போது திற
armenian.LaunchNow=Բացել {#AppName}-ը հիմա
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
catalan.DoneCredit=Gaudiu del disseny de Juxtopposed.
danish.DoneCredit=God fornøjelse med designet af Juxtopposed.
finnish.DoneCredit=Nauti Juxtopposedin suunnittelusta.
hungarian.DoneCredit=Élvezze Juxtopposed dizájnját.
bulgarian.DoneCredit=Насладете се на дизайна на Juxtopposed.
slovak.DoneCredit=Užite si dizajn od Juxtopposed.
slovenian.DoneCredit=Uživajte v oblikovanju Juxtopposed.
norwegian.DoneCredit=Kos deg med designet av Juxtopposed.
thai.DoneCredit=ขอให้สนุกกับการออกแบบของ Juxtopposed
tamil.DoneCredit=Juxtopposed-இன் வடிவமைப்பை அனுபவியுங்கள்.
armenian.DoneCredit=Վայելեք Juxtopposed-ի դիզայնը։
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
catalan.LinkSubscribe=Subscriviu-vos a YouTube
danish.LinkSubscribe=Abonnér på YouTube
finnish.LinkSubscribe=Tilaa YouTubessa
hungarian.LinkSubscribe=Feliratkozás a YouTube-on
bulgarian.LinkSubscribe=Абонирайте се в YouTube
slovak.LinkSubscribe=Odoberať na YouTube
slovenian.LinkSubscribe=Naročite se na YouTubu
norwegian.LinkSubscribe=Abonner på YouTube
thai.LinkSubscribe=ติดตามบน YouTube
tamil.LinkSubscribe=YouTube-இல் குழுசேருங்கள்
armenian.LinkSubscribe=Բաժանորդագրվել YouTube-ում
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
catalan.NoticeTitle=Avís i llicències
danish.NoticeTitle=Oplysninger og licenser
finnish.NoticeTitle=Tiedot ja lisenssit
hungarian.NoticeTitle=Tájékoztató és licencek
bulgarian.NoticeTitle=Бележки и лицензи
slovak.NoticeTitle=Upozornenie a licencie
slovenian.NoticeTitle=Obvestilo in licence
norwegian.NoticeTitle=Merknad og lisenser
thai.NoticeTitle=ประกาศและสัญญาอนุญาต
tamil.NoticeTitle=அறிவிப்பும் உரிமங்களும்
armenian.NoticeTitle=Ծանուցում և լիցենզիաներ
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
catalan.Close=Tanca
danish.Close=Luk
finnish.Close=Sulje
hungarian.Close=Bezárás
bulgarian.Close=Затваряне
slovak.Close=Zavrieť
slovenian.Close=Zapri
norwegian.Close=Lukk
thai.Close=ปิด
tamil.Close=மூடு
armenian.Close=Փակել
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
catalan.BadDir=Trieu una carpeta on instal·lar-lo.
danish.BadDir=Vælg en mappe at installere i.
finnish.BadDir=Valitse asennuskansio.
hungarian.BadDir=Válasszon egy mappát a telepítéshez.
bulgarian.BadDir=Изберете папка за инсталиране.
slovak.BadDir=Vyberte priečinok na inštaláciu.
slovenian.BadDir=Izberite mapo za namestitev.
norwegian.BadDir=Velg en mappe å installere i.
thai.BadDir=เลือกโฟลเดอร์ที่จะติดตั้ง
tamil.BadDir=நிறுவ ஒரு கோப்புறையைத் தேர்ந்தெடுக்கவும்.
armenian.BadDir=Ընտրեք տեղադրման պանակ։
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
catalan.ToggleOn=SÍ
danish.ToggleOn=TIL
finnish.ToggleOn=PÄÄLLÄ
hungarian.ToggleOn=BE
bulgarian.ToggleOn=ВКЛ
slovak.ToggleOn=ZAP
slovenian.ToggleOn=VKL
norwegian.ToggleOn=PÅ
thai.ToggleOn=เปิด
tamil.ToggleOn=ஆன்
armenian.ToggleOn=ԱՅՈ
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
catalan.ToggleOff=NO
danish.ToggleOff=FRA
finnish.ToggleOff=POIS
hungarian.ToggleOff=KI
bulgarian.ToggleOff=ИЗКЛ
slovak.ToggleOff=VYP
slovenian.ToggleOff=IZKL
norwegian.ToggleOff=AV
thai.ToggleOff=ปิด
tamil.ToggleOff=ஆஃப்
armenian.ToggleOff=ՈՉ
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
catalan.TypeVideo=Vídeo %1
danish.TypeVideo=%1-video
finnish.TypeVideo=%1-video
hungarian.TypeVideo=%1-videó
bulgarian.TypeVideo=Видео %1
slovak.TypeVideo=Video %1
slovenian.TypeVideo=Video %1
norwegian.TypeVideo=%1-video
thai.TypeVideo=วีดิทัศน์ %1
tamil.TypeVideo=%1 நிகழ்படம்
armenian.TypeVideo=%1 վիդեո
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
catalan.TypeAudio=Àudio %1
danish.TypeAudio=%1-lyd
finnish.TypeAudio=%1-ääni
hungarian.TypeAudio=%1-hang
bulgarian.TypeAudio=Аудио %1
slovak.TypeAudio=Zvuk %1
slovenian.TypeAudio=Zvok %1
norwegian.TypeAudio=%1-lyd
thai.TypeAudio=เสียง %1
tamil.TypeAudio=%1 ஒலி
armenian.TypeAudio=%1 աուդիո
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
catalan.TypePlaylist=Llista de reproducció %1
danish.TypePlaylist=%1-spilleliste
finnish.TypePlaylist=%1-soittolista
hungarian.TypePlaylist=%1-lejátszólista
bulgarian.TypePlaylist=Плейлист %1
slovak.TypePlaylist=Playlist %1
slovenian.TypePlaylist=Seznam predvajanja %1
norwegian.TypePlaylist=%1-spilleliste
thai.TypePlaylist=ผังเล่น %1
tamil.TypePlaylist=%1 தொகுப்பு பட்டியல்
armenian.TypePlaylist=%1 երգացանկ
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
catalan.VerbPlay=Reprodueix
danish.VerbPlay=Afspil
finnish.VerbPlay=Toista
hungarian.VerbPlay=Lejátszás
bulgarian.VerbPlay=Възпроизвеждане
slovak.VerbPlay=Prehrať
slovenian.VerbPlay=Predvajaj
norwegian.VerbPlay=Spill av
thai.VerbPlay=เล่น
tamil.VerbPlay=இயக்கு
armenian.VerbPlay=Նվագարկել
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
catalan.VerbEnqueue=Afegeix a la cua
danish.VerbEnqueue=Føj til kø
finnish.VerbEnqueue=Lisää jonoon
hungarian.VerbEnqueue=Hozzáadás a sorhoz
bulgarian.VerbEnqueue=Добавяне в опашката
slovak.VerbEnqueue=Pridať do frontu
slovenian.VerbEnqueue=Dodaj v čakalno vrsto
norwegian.VerbEnqueue=Legg til i køen
thai.VerbEnqueue=เพิ่มเข้าคิว
tamil.VerbEnqueue=வரிசையில் சேர்
armenian.VerbEnqueue=Ավելացնել հերթում
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
catalan.ContextPlay=Reprodueix amb el {#AppName}
danish.ContextPlay=Afspil med {#AppName}
finnish.ContextPlay=Toista sovelluksella {#AppName}
hungarian.ContextPlay=Lejátszás a {#AppName} programmal
bulgarian.ContextPlay=Възпроизвеждане с {#AppName}
slovak.ContextPlay=Prehrať pomocou {#AppName}
slovenian.ContextPlay=Predvajaj z {#AppName}
norwegian.ContextPlay=Spill av med {#AppName}
thai.ContextPlay=เล่นด้วย {#AppName}
tamil.ContextPlay={#AppName} மூலம் இயக்கு
armenian.ContextPlay=Նվագարկել {#AppName}-ով
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
catalan.ContextEnqueue=Afegeix a la cua del {#AppName}
danish.ContextEnqueue=Føj til køen i {#AppName}
finnish.ContextEnqueue=Lisää sovelluksen {#AppName} jonoon
hungarian.ContextEnqueue=Hozzáadás a {#AppName} lejátszási sorához
bulgarian.ContextEnqueue=Добавяне в опашката на {#AppName}
slovak.ContextEnqueue=Pridať do frontu {#AppName}
slovenian.ContextEnqueue=Dodaj v čakalno vrsto {#AppName}
norwegian.ContextEnqueue=Legg til i køen i {#AppName}
thai.ContextEnqueue=เพิ่มเข้าคิวของ {#AppName}
tamil.ContextEnqueue={#AppName} வரிசையில் சேர்
armenian.ContextEnqueue=Ավելացնել {#AppName}-ի հերթում
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
catalan.Running=El {#AppName} està obert. L'instal·lador el tancarà ara i el tornarà a obrir quan acabi.
danish.Running={#AppName} er åben. Installationen lukker den nu og åbner den igen, når den er færdig.
finnish.Running={#AppName} on auki. Asennus sulkee sen nyt ja avaa sen uudelleen, kun asennus on valmis.
hungarian.Running=A {#AppName} meg van nyitva. A telepítő most bezárja, és a befejezés után újra megnyitja.
bulgarian.Running={#AppName} е отворен. Инсталаторът ще го затвори сега и ще го отвори отново, когато приключи.
slovak.Running={#AppName} je otvorený. Inštalačný program ho teraz zatvorí a po dokončení znova otvorí.
slovenian.Running={#AppName} je odprt. Namestitveni program ga bo zdaj zaprl in ga po končani namestitvi znova odprl.
norwegian.Running={#AppName} er åpen. Installasjonsprogrammet lukker den nå og åpner den igjen når det er ferdig.
thai.Running={#AppName} เปิดอยู่ ตัวติดตั้งจะปิดโปรแกรมตอนนี้และเปิดอีกครั้งเมื่อเสร็จ
tamil.Running={#AppName} திறந்துள்ளது. நிறுவி அதை இப்போது மூடி, முடிந்ததும் மீண்டும் திறக்கும்.
armenian.Running={#AppName}-ը բաց է։ Տեղադրիչը հիմա կփակի այն և նորից կբացի ավարտելուց հետո։
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
catalan.StillRunning=El {#AppName} no s'ha tancat. Tanqueu-lo i feu clic a Torna-ho a provar.
danish.StillRunning={#AppName} blev ikke lukket. Luk den, og klik på Prøv igen.
finnish.StillRunning={#AppName} ei sulkeutunut. Sulje se ja valitse Yritä uudelleen.
hungarian.StillRunning=A {#AppName} nem zárult be. Zárja be, majd kattintson az Ismét gombra.
bulgarian.StillRunning={#AppName} не се затвори. Затворете го и щракнете върху „Повторен опит“.
slovak.StillRunning={#AppName} sa nezatvoril. Zatvorte ho a kliknite na Opakovať.
slovenian.StillRunning={#AppName} se ni zaprl. Zaprite ga in kliknite Poskusi znova.
norwegian.StillRunning={#AppName} ble ikke lukket. Lukk den og klikk på Prøv igjen.
thai.StillRunning={#AppName} ไม่ได้ปิด ปิดโปรแกรมแล้วคลิก “ลองใหม่”
tamil.StillRunning={#AppName} மூடப்படவில்லை. அதை மூடிவிட்டு மீண்டும் முயல் என்பதைச் சொடுக்கவும்.
armenian.StillRunning={#AppName}-ը չփակվեց։ Փակեք այն և սեղմեք «Կրկին փորձել»։
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
catalan.ShortcutComment=Reproductor multimèdia VLC amb la interfície dissenyada per Juxtopposed
danish.ShortcutComment=VLC media player med brugerfladen designet af Juxtopposed
finnish.ShortcutComment=VLC media player Juxtopposedin suunnittelemalla käyttöliittymällä
hungarian.ShortcutComment=VLC médialejátszó a Juxtopposed által tervezett kezelőfelülettel
bulgarian.ShortcutComment=Медиен плейър VLC с интерфейса, създаден от Juxtopposed
slovak.ShortcutComment=VLC media player s rozhraním od Juxtopposed
slovenian.ShortcutComment=Predvajalnik VLC z vmesnikom, ki ga je oblikovala Juxtopposed
norwegian.ShortcutComment=VLC media player med grensesnittet designet av Juxtopposed
thai.ShortcutComment=โปรแกรมเล่นสื่อ VLC พร้อมส่วนติดต่อที่ออกแบบโดย Juxtopposed
tamil.ShortcutComment=Juxtopposed வடிவமைத்த இடைமுகத்துடன் VLC ஊடக இயக்கி
armenian.ShortcutComment=VLC media player՝ Juxtopposed-ի ստեղծած ինտերֆեյսով
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
LanguageContext_catalan=El vostre Windows és en català.
LanguageQuestion_catalan=Voleu instal·lar el {#AppName} en català?
LanguageYes_catalan=Sí
LanguageNo_catalan=No
LanguageContext_danish=Dit Windows er på dansk.
LanguageQuestion_danish=Installér {#AppName} på dansk?
LanguageYes_danish=Ja
LanguageNo_danish=Nej
LanguageContext_finnish=Windowsisi on suomenkielinen.
LanguageQuestion_finnish=Asennetaanko {#AppName} suomeksi?
LanguageYes_finnish=Kyllä
LanguageNo_finnish=Ei
LanguageContext_hungarian=Az Ön Windows rendszere magyar nyelvű.
LanguageQuestion_hungarian=Magyar nyelven telepíti a {#AppName} programot?
LanguageYes_hungarian=Igen
LanguageNo_hungarian=Nem
LanguageContext_bulgarian=Вашият Windows е на български.
LanguageQuestion_bulgarian=Да се инсталира ли {#AppName} на български?
LanguageYes_bulgarian=Да
LanguageNo_bulgarian=Не
LanguageContext_slovak=Váš systém Windows je v slovenčine.
LanguageQuestion_slovak=Nainštalovať {#AppName} v slovenčine?
LanguageYes_slovak=Áno
LanguageNo_slovak=Nie
LanguageContext_slovenian=Vaš sistem Windows je v slovenščini.
LanguageQuestion_slovenian=Želite namestiti {#AppName} v slovenščini?
LanguageYes_slovenian=Da
LanguageNo_slovenian=Ne
LanguageContext_norwegian=Windows er på norsk.
LanguageQuestion_norwegian=Installere {#AppName} på norsk?
LanguageYes_norwegian=Ja
LanguageNo_norwegian=Nei
LanguageContext_thai=Windows ของคุณเป็นภาษาไทย
LanguageQuestion_thai=ติดตั้ง {#AppName} เป็นภาษาไทยหรือไม่?
LanguageYes_thai=ใช่
LanguageNo_thai=ไม่
LanguageContext_tamil=உங்கள் Windows தமிழில் உள்ளது.
LanguageQuestion_tamil={#AppName}-ஐத் தமிழில் நிறுவவா?
LanguageYes_tamil=ஆம்
LanguageNo_tamil=இல்லை
LanguageContext_armenian=Ձեր Windows-ը հայերեն է։
LanguageQuestion_armenian=Տեղադրե՞լ {#AppName}-ը հայերենով
LanguageYes_armenian=Այո
LanguageNo_armenian=Ոչ

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
; Installed for all users (Program Files): none. One folder every user can
; write to would be every user's profile, so one user's vlcrc or Lua script
; would run in every other user's VLC; each keeps %APPDATA%\VLC Redesign
; (see ProfileDirOf)

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
; page's "Open VLC Redesign now" does (CurStepChanged). As the user who
; started Setup, never as an administrator (an install for everyone)
Filename: "{app}\vlc.exe"; Parameters: "{code:RelaunchArgs}"; Flags: nowait runasoriginaluser; Check: ShouldRelaunch

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
  { The Languages page (LayoutPage): the app's languages (AppLanguageCodes,
    their names in the lists), the interface's and the subtitles' lists,
    whether the subtitles' was changed by hand (else it follows the
    interface's), and the right-to-left toggle's description }
  AppLanguageCodes, AppLanguageNames: array of String;
  InterfaceCombo, SubtitleCombo: TNewComboBox;
  SubtitleTouched: Boolean;
  RtlNote: TLabel;

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

{ The profile of a copy installed in a folder: portable\ next to vlc.exe
  (see [Dirs]); installed for everyone, the user's own %APPDATA%\VLC
  Redesign, where vlc.exe keeps it then (bin/winvlc.c) }
function ProfileDirOf(Dir: String): String;
begin
  if IsAdminInstallMode then
    Result := ExpandConstant('{userappdata}\{#AppName}')
  else
    Result := AddBackslash(Dir) + 'portable';
end;

{ The profile of the copy installed: its settings }
function ProfileFile(Name: String): String;
begin
  Result := AddBackslash(ProfileDirOf(WizardDirValue)) + Name;
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
{ ---- The Languages page ------------------------------------------------ }
{ The languages of the app (the complete ones of simple_preferences.cpp),
  each in its own name and in English: the language of the interface, and
  the default one of the subtitles (the app's list has more; Settings) }
procedure AddAppLanguage(Code, Native, English: String);
var
  N: Integer;
begin
  N := GetArrayLength(AppLanguageCodes);
  SetArrayLength(AppLanguageCodes, N + 1);
  SetArrayLength(AppLanguageNames, N + 1);
  AppLanguageCodes[N] := Code;
  if Native = English then
    AppLanguageNames[N] := English
  else
    AppLanguageNames[N] := Native + ' - ' + English;
end;

procedure LoadAppLanguages();
begin
  AddAppLanguage('af', 'Afrikaans', 'Afrikaans');
  AddAppLanguage('sq', 'Shqip', 'Albanian');
  AddAppLanguage('ar', 'العربية', 'Arabic');
  AddAppLanguage('hy', 'Հայերեն', 'Armenian');
  AddAppLanguage('eu', 'Euskara', 'Basque');
  AddAppLanguage('be', 'Беларуская', 'Belarusian');
  AddAppLanguage('bn', 'বাংলা', 'Bengali');
  AddAppLanguage('bs', 'Bosanski', 'Bosnian');
  AddAppLanguage('bg', 'Български', 'Bulgarian');
  AddAppLanguage('ca', 'Català', 'Catalan');
  AddAppLanguage('ckb', 'کوردی', 'Central Kurdish');
  AddAppLanguage('zh_CN', '简体中文', 'Chinese (Simplified)');
  AddAppLanguage('zh_TW', '繁體中文', 'Chinese (Traditional)');
  AddAppLanguage('hr', 'Hrvatski', 'Croatian');
  AddAppLanguage('cs', 'Čeština', 'Czech');
  AddAppLanguage('da', 'Dansk', 'Danish');
  AddAppLanguage('nl', 'Nederlands', 'Dutch');
  AddAppLanguage('en', 'English', 'English');
  AddAppLanguage('en_GB', 'English (United Kingdom)', 'English (United Kingdom)');
  AddAppLanguage('et', 'Eesti', 'Estonian');
  AddAppLanguage('fi', 'Suomi', 'Finnish');
  AddAppLanguage('fr', 'Français', 'French');
  AddAppLanguage('gl', 'Galego', 'Galician');
  AddAppLanguage('ka', 'ქართული', 'Georgian');
  AddAppLanguage('de', 'Deutsch', 'German');
  AddAppLanguage('el', 'Ελληνικά', 'Greek');
  AddAppLanguage('gu', 'ગુજરાતી', 'Gujarati');
  AddAppLanguage('he', 'עברית', 'Hebrew');
  AddAppLanguage('hi', 'हिन्दी', 'Hindi');
  AddAppLanguage('hu', 'Magyar', 'Hungarian');
  AddAppLanguage('is', 'Íslenska', 'Icelandic');
  AddAppLanguage('id', 'Bahasa Indonesia', 'Indonesian');
  AddAppLanguage('ga', 'Gaeilge', 'Irish');
  AddAppLanguage('it', 'Italiano', 'Italian');
  AddAppLanguage('ja', '日本語', 'Japanese');
  AddAppLanguage('kn', 'ಕನ್ನಡ', 'Kannada');
  AddAppLanguage('kk', 'Қазақ тілі', 'Kazakh');
  AddAppLanguage('km', 'ខ្មែរ', 'Khmer');
  AddAppLanguage('ko', '한국어', 'Korean');
  AddAppLanguage('lv', 'Latviešu', 'Latvian');
  AddAppLanguage('lt', 'Lietuvių', 'Lithuanian');
  AddAppLanguage('mk', 'Македонски', 'Macedonian');
  AddAppLanguage('ms', 'Bahasa Melayu', 'Malay');
  AddAppLanguage('ml', 'മലയാളം', 'Malayalam');
  AddAppLanguage('mr', 'मराठी', 'Marathi');
  AddAppLanguage('mn', 'Монгол', 'Mongolian');
  AddAppLanguage('ne', 'नेपाली', 'Nepali');
  AddAppLanguage('nb', 'Norsk bokmål', 'Norwegian Bokmål');
  AddAppLanguage('nn', 'Norsk nynorsk', 'Norwegian Nynorsk');
  AddAppLanguage('oc', 'Occitan', 'Occitan');
  AddAppLanguage('fa', 'فارسی', 'Persian');
  AddAppLanguage('pl', 'Polski', 'Polish');
  AddAppLanguage('pt_BR', 'Português (Brasil)', 'Portuguese (Brazil)');
  AddAppLanguage('pt_PT', 'Português (Portugal)', 'Portuguese (Portugal)');
  AddAppLanguage('pa', 'ਪੰਜਾਬੀ', 'Punjabi');
  AddAppLanguage('ro', 'Română', 'Romanian');
  AddAppLanguage('ru', 'Русский', 'Russian');
  AddAppLanguage('gd', 'Gàidhlig', 'Scottish Gaelic');
  AddAppLanguage('sr', 'Српски', 'Serbian');
  AddAppLanguage('si', 'සිංහල', 'Sinhala');
  AddAppLanguage('sk', 'Slovenčina', 'Slovak');
  AddAppLanguage('sl', 'Slovenščina', 'Slovenian');
  AddAppLanguage('es', 'Español', 'Spanish');
  AddAppLanguage('es_MX', 'Español (México)', 'Spanish (Mexico)');
  AddAppLanguage('sv', 'Svenska', 'Swedish');
  AddAppLanguage('ta', 'தமிழ்', 'Tamil');
  AddAppLanguage('te', 'తెలుగు', 'Telugu');
  AddAppLanguage('th', 'ไทย', 'Thai');
  AddAppLanguage('tr', 'Türkçe', 'Turkish');
  AddAppLanguage('uk', 'Українська', 'Ukrainian');
  AddAppLanguage('ur', 'اردو', 'Urdu');
  AddAppLanguage('vi', 'Tiếng Việt', 'Vietnamese');
  AddAppLanguage('wa', 'Walon', 'Walloon');
  AddAppLanguage('cy', 'Cymraeg', 'Welsh');
end;

function AppLanguageIndex(Code: String): Integer;
var
  I: Integer;
begin
  Result := -1;
  for I := 0 to GetArrayLength(AppLanguageCodes) - 1 do
    if CompareText(AppLanguageCodes[I], Code) = 0 then
    begin
      Result := I;
      Exit;
    end;
  { a language with a region the list doesn't have: the language }
  if Pos('_', Code) > 0 then
    Result := AppLanguageIndex(Copy(Code, 1, Pos('_', Code) - 1));
end;

function RightToLeftCode(Code: String): Boolean;
begin
  Result := (Code = 'he') or (Code = 'ar') or (Code = 'fa') or (Code = 'ur') or (Code = 'ckb');
end;

function ComboCode(Combo: TNewComboBox): String;
begin
  if Combo.ItemIndex >= 0 then
    Result := AppLanguageCodes[Combo.ItemIndex]
  else
    Result := 'en';
end;

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
  /RIGHTTOLEFT or /LEFTTORIGHT (neither: the app follows its language). }
procedure SaveChoices();
var
  Updates, Subtitles, RightToLeft, WriteUpdates, WriteSubtitles, WriteLayout: Boolean;
  FileName, SubtitleLanguage: String;
begin
  SubtitleLanguage := '';
  if LayoutShown and not WizardSilent then
  begin
    { right to left: as the toggle says; left to right: as the language }
    RightToLeft := Checked(T_RTL);
    WriteLayout := RightToLeftCode(ComboCode(InterfaceCombo));
    SubtitleLanguage := ComboCode(SubtitleCombo);
  end
  else
  begin
    RightToLeft := HasSwitch('RIGHTTOLEFT');
    WriteLayout := RightToLeft or HasSwitch('LEFTTORIGHT');
    if AppLanguageIndex(ExpandConstant('{param:SUBLANG|}')) >= 0 then
      SubtitleLanguage := AppLanguageCodes[AppLanguageIndex(ExpandConstant('{param:SUBLANG|}'))];
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
    if (not WriteUpdates) and (not WriteSubtitles) and (not WriteLayout) and (SubtitleLanguage = '') and
       (ChoicesMade() or ProfileHasSettings()) then
    begin
      Log('Privacy: nothing to change, the profile keeps its choices');
      Exit;
    end;
  end;
  FileName := ProfileFile(CHOICES_FILE);
  { (%APPDATA%\VLC Redesign of an install for everyone: made here) }
  ForceDirectories(ExtractFileDir(FileName));
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
  if SubtitleLanguage <> '' then
    SetIniString('subtitles', 'language', SubtitleLanguage, FileName);
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

{ The language of the app in a folder (its profile's language), '' if none }
function LanguageIn(Dir: String): String;
var
  S: AnsiString;
begin
  Result := '';
  if (Dir <> '') and LoadStringFromFile(AddBackslash(ProfileDirOf(Dir)) + LANGUAGE_FILE, S) then
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
  { the Languages page's choice, else /UILANG= }
  if LayoutShown and not WizardSilent then
    AppLanguage := ComboCode(InterfaceCombo)
  else if AppLanguageIndex(ExpandConstant('{param:UILANG|}')) >= 0 then
    AppLanguage := AppLanguageCodes[AppLanguageIndex(ExpandConstant('{param:UILANG|}'))];
  if AppLanguage = '' then
    Exit;
  FileName := ProfileFile(LANGUAGE_FILE);
  ForceDirectories(ExtractFileDir(FileName));
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

{ The right-to-left toggle, for a right-to-left language of the interface }
procedure ShowRtlToggle();
var
  Rtl: Boolean;
begin
  Rtl := RightToLeftCode(ComboCode(InterfaceCombo));
  Toggles[T_RTL].Box.Visible := Rtl;
  Toggles[T_RTL].Text.Visible := Rtl;
  RtlNote.Visible := Rtl;
end;

procedure InterfaceComboChange(Sender: TObject);
begin
  { the subtitles in the language of the interface, until chosen apart }
  if not SubtitleTouched then
    SubtitleCombo.ItemIndex := InterfaceCombo.ItemIndex;
  ShowRtlToggle();
end;

procedure SubtitleComboChange(Sender: TObject);
begin
  SubtitleTouched := True;
end;

function NewLanguageCombo(Parent: TWinControl; T: Integer): TNewComboBox;
var
  I: Integer;
begin
  Result := TNewComboBox.Create(WizardForm);
  Result.Parent := Parent;
  Result.Style := csDropDownList;
  Result.DropDownCount := 14;
  Result.Color := CLR_WELL;
  Result.Font.Name := Face(False);
  Result.Font.Height := FontPx(13);
  Result.Font.Color := CLR_TEXT;
  for I := 0 to GetArrayLength(AppLanguageNames) - 1 do
    Result.Items.Add(AppLanguageNames[I]);
  Place(Result, True, CONTENT_L, T, CONTENT_W, 30);
end;

{ The language of the app's interface and of its subtitles, each in a list,
  the interface's preselected in Setup's language (or as the profile has
  it); the subtitles' follow it until chosen apart. Right to left, the
  app's layout too: mirrored to begin with as the app lays out the
  language (see the top) }
procedure CreateLayout();
var
  P: TWinControl;
  Code: String;
begin
  LoadAppLanguages();
  LayoutPage := CreateCustomPage(OptionsPage.ID, CustomMessage('LanguagesTitle'), CustomMessage('LanguagesText'));
  P := LayoutPage.Surface;
  NewLabel(P, True, CONTENT_L, 100, 0, 13, False, CLR_MUTED, CLR_BACK, CustomMessage('LanguagesInterface'));
  InterfaceCombo := NewLanguageCombo(P, 120);
  NewLabel(P, True, CONTENT_L, 168, 0, 13, False, CLR_MUTED, CLR_BACK, CustomMessage('LanguagesSubtitles'));
  SubtitleCombo := NewLanguageCombo(P, 188);
  NewLabel(P, True, CONTENT_L, 224, CONTENT_W, 11, False, CLR_FAINT, CLR_BACK, CustomMessage('LanguagesSubtitlesText'));
  NewToggle(T_RTL, P, True, 262, CustomMessage('LayoutRTL'), not HasSwitch('LEFTTORIGHT'));
  RtlNote := NewLabel(P, True, CONTENT_L, 300, CONTENT_W, 11, False, CLR_FAINT, CLR_BACK, CustomMessage('LayoutRTLText'));

  Code := ExpandConstant('{param:UILANG|}');
  if Code = '' then Code := AppLanguage;
  if Code = '' then Code := Languages[Lang].Code;
  InterfaceCombo.ItemIndex := AppLanguageIndex(Code);
  if InterfaceCombo.ItemIndex < 0 then
    InterfaceCombo.ItemIndex := AppLanguageIndex('en');
  Code := ExpandConstant('{param:SUBLANG|}');
  if (Code <> '') and (AppLanguageIndex(Code) >= 0) then
  begin
    SubtitleCombo.ItemIndex := AppLanguageIndex(Code);
    SubtitleTouched := True;
  end
  else
    SubtitleCombo.ItemIndex := InterfaceCombo.ItemIndex;
  InterfaceCombo.OnChange := @InterfaceComboChange;
  SubtitleCombo.OnChange := @SubtitleComboChange;
  ShowRtlToggle();
end;

{ The Languages page shows: once, for a profile without a language (a new
  install, or one from before the page); a silent install takes /UILANG=
  and /SUBLANG= }
function ShowLayout(): Boolean;
begin
  Result := not FileExists(ProfileFile(LANGUAGE_FILE));
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

{ An earlier install for everyone made portable\ there, writable by every
  user and so every user's profile (see [Dirs]). It is set aside, never a
  profile again; any user may have changed what it holds, so it is copied
  into no one's profile. The uninstaller removes it. }
procedure RetireSharedProfile();
var
  Shared, Aside: String;
begin
  Shared := ExpandConstant('{app}\portable');
  Aside := ExpandConstant('{app}\portable-shared');
  if not DirExists(Shared) then
    Exit;
  if DirExists(Aside) then
    DelTree(Aside, True, True, True);
  if RenameFile(Shared, Aside) then
    Log('Shared profile ' + Shared + ' set aside as ' + Aside)
  else
    Log('Shared profile ' + Shared + ' could not be set aside');
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  Code: Integer;
begin
  if (CurStep = ssInstall) and IsAdminInstallMode then
    RetireSharedProfile();
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
      Not --version: it opens a console and waits for a key. As the user
      who started Setup: nothing of VLC runs as an administrator }
    ExecAsOriginalUser(AppExe(), '--prewarm', '', SW_HIDE, ewNoWait, Code);
    ExecAsOriginalUser(ExpandConstant('{app}\vlc-preparser.exe'), '--help', '', SW_HIDE, ewNoWait, Code);
    ExecAsOriginalUser(ExpandConstant('{app}\vlc-waveform.exe'), '', '', SW_HIDE, ewNoWait, Code);
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
    if DirExists(ExpandConstant('{app}\portable-shared')) then
      DelTree(ExpandConstant('{app}\portable-shared'), True, True, True);
    { no copy starts with these settings any more }
    RegDeleteValue(HKCU, PROFILES, ExpandConstant('{app}\portable'));
    RegDeleteKeyIfEmpty(HKCU, PROFILES);
    RegDeleteKeyIfEmpty(HKCU, 'Software\{#AppName}');
    RemoveDir(ExpandConstant('{app}'));
  end;
end;
