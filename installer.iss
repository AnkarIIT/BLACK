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
Name: "setdefaultbrowser"; Description: "Set BLACK as default browser"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "build\Release\BLACK.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\Release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*.obj,*.pch,*.cpp,*.h,*.rc"

[Icons]
Name: "{group}\BLACK"; Filename: "{app}\BLACK.exe"
Name: "{group}\{cm:UninstallProgram,BLACK}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\BLACK"; Filename: "{app}\BLACK.exe"; Tasks: desktopicon

[Registry]
; HTTP protocol handler
Root: HKCR; Subkey: "BLACKHTML"; ValueType: string; ValueData: "BLACK HTML Document"; Flags: uninsdeletekey
Root: HKCR; Subkey: "BLACKHTML\shell\open\command"; ValueType: string; ValueData: """{app}\BLACK.exe"" ""%1"""; Flags: uninsdeletekey
Root: HKCR; Subkey: "BLACKHTML\DefaultIcon"; ValueType: string; ValueData: "{app}\BLACK.exe,0"; Flags: uninsdeletekey

Root: HKCR; Subkey: "http\shell\open\command"; ValueType: string; ValueData: """{app}\BLACK.exe"" ""%1"""; Flags: uninsdeletekey
Root: HKCR; Subkey: "https\shell\open\command"; ValueType: string; ValueData: """{app}\BLACK.exe"" ""%1"""; Flags: uninsdeletekey

; File associations for .html/.htm
Root: HKCR; Subkey: ".html"; ValueType: string; ValueData: "BLACKHTML"; Flags: uninsdeletevalue
Root: HKCR; Subkey: ".htm"; ValueType: string; ValueData: "BLACKHTML"; Flags: uninsdeletevalue

; black:// internal protocol
Root: HKCR; Subkey: "black"; ValueType: string; ValueData: "URL:BLACK Internal Protocol"; Flags: uninsdeletekey
Root: HKCR; Subkey: "black\shell\open\command"; ValueType: string; ValueData: """{app}\BLACK.exe"" ""%1"""; Flags: uninsdeletekey
Root: HKCR; Subkey: "black\DefaultIcon"; ValueType: string; ValueData: "{app}\BLACK.exe,0"; Flags: uninsdeletekey

; App Paths for command-line access
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\BLACK.exe"; ValueType: string; ValueData: "{app}\BLACK.exe"; Flags: uninsdeletekey

[Icons]
Name: "{group}\BLACK"; Filename: "{app}\BLACK.exe"
Name: "{group}\{cm:UninstallProgram,BLACK}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\BLACK"; Filename: "{app}\BLACK.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\BLACK.exe"; Description: "{cm:LaunchProgram,BLACK}"; Flags: postinstall nowait skipifsilent

[Code]
function InitializeSetup(): Boolean;
begin
  Result := True;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
end;

procedure CurStepChanged(CurStep: Integer);
begin
  if CurStep = ssPostInstall then begin
    // Set as default browser if task was selected
    if IsTaskSelected('setdefaultbrowser') then begin
      Exec('cmd.exe', '/c start "" "ms-settings:defaultapps"', '', SW_HIDE, ewNoWait, ErrorCode);
    end;
  end;
end;