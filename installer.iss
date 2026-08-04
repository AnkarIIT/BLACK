; Inno Setup Script for BLACK Browser
[Setup]
AppId={{8B44A7E8-5D12-42B1-9457-4E2E0A86E123}
AppName=BLACK
AppVersion=1.0.0
AppPublisher=BLACK Software
DefaultDirName={autopf}\BLACK
DefaultGroupName=BLACK
DisableProgramGroupPage=yes
OutputBaseFilename=BLACK_Setup_v1.0
SetupIconFile=app.ico
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "build\Release\BLACK.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\Release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*.obj,*.pch,*.cpp,*.h,*.rc"

[Icons]
Name: "{group}\BLACK"; Filename: "{app}\BLACK.exe"
Name: "{group}\{cm:UninstallProgram,BLACK}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\BLACK"; Filename: "{app}\BLACK.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\BLACK.exe"; Description: "{cm:LaunchProgram,BLACK}"; Flags: postinstall nowait skipifsilent
