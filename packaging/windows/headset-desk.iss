; Build after deploying Qt. Supply /DStageDir=<absolute portable directory>
; and /DOutputDir=<absolute output directory> to ISCC.
#ifndef StageDir
  #error StageDir must point to a complete deployed Windows app folder
#endif
#ifndef OutputDir
  #error OutputDir must point to the release output directory
#endif
#define AppVersion "0.3.0-beta.1"

[Setup]
AppId={{0A8D957F-52C5-4EEA-9931-2F37B9C242DD}
AppName=Headset Desk
AppVersion={#AppVersion}
AppVerName=Headset Desk {#AppVersion}
AppPublisher=Headset Desk contributors
AppPublisherURL=https://github.com/ashwinpr15/headset-desk
AppSupportURL=https://github.com/ashwinpr15/headset-desk/issues
AppUpdatesURL=https://github.com/ashwinpr15/headset-desk/releases
DefaultDirName={localappdata}\Programs\Headset Desk
DefaultGroupName=Headset Desk
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.22000
OutputDir={#OutputDir}
OutputBaseFilename=headset-desk-v{#AppVersion}-windows-x64-setup
SetupIconFile=..\..\apps\desktop\resources\headset-desk.ico
UninstallDisplayIcon={app}\headset-desk.exe
VersionInfoVersion=0.3.0.1
LicenseFile=..\..\LICENSE
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
AppMutex=Local\HeadsetDeskRunningV1
CloseApplications=no
RestartApplications=no
Uninstallable=yes

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: unchecked

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\Headset Desk"; Filename: "{app}\headset-desk.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\Headset Desk"; Filename: "{app}\headset-desk.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\headset-desk.exe"; Description: "Open Headset Desk"; Flags: nowait postinstall skipifsilent unchecked
