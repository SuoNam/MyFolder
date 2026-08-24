#define AppName "MyFolder"
#define AppVersion "1.1.1"
#define AppPublisher "MyFolder"
#define AppExeName "appMyFolder.exe"
#define StageDir GetEnv("MYFOLDER_STAGE_DIR")
#define BuildOutputDir GetEnv("MYFOLDER_INSTALLER_OUTPUT")

[Setup]
AppId={{B36DF7B3-477B-4A2D-8B6D-F35CF7EF8944}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\MyFolder
DefaultGroupName=MyFolder
DisableProgramGroupPage=yes
OutputDir={#BuildOutputDir}
OutputBaseFilename=MyFolder-v1.1.1-Windows-x64-Setup
SetupIconFile=..\Icons\app.ico
UninstallDisplayIcon={app}\{#AppExeName}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
CloseApplications=force
RestartApplications=no

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "附加快捷方式："; Flags: unchecked

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\MyFolder"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"
Name: "{autodesktop}\MyFolder"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "Software\Classes\myfolder"; ValueType: string; ValueName: ""; ValueData: "URL:MyFolder OAuth Callback"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\myfolder"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\myfolder\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExeName},0"
Root: HKCU; Subkey: "Software\Classes\myfolder\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" ""%1"""
Root: HKCU; Subkey: "Software\MyFolder"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\MyFolder\ShellCommands"; ValueType: dword; ValueName: "UploadServer"; ValueData: "1"; Flags: createvalueifdoesntexist
Root: HKCU; Subkey: "Software\MyFolder\ShellCommands"; ValueType: dword; ValueName: "SendClient"; ValueData: "1"; Flags: createvalueifdoesntexist
Root: HKCU; Subkey: "Software\Classes\*\shell\MyFolderUpload"; Flags: deletekey
Root: HKCU; Subkey: "Software\Classes\*\shell\MyFolderSend"; Flags: deletekey

[Run]
Filename: "netsh.exe"; Parameters: "advfirewall firewall delete rule name=""MyFolder LAN Receiver"""; Flags: runhidden waituntilterminated
Filename: "netsh.exe"; Parameters: "advfirewall firewall add rule name=""MyFolder LAN Receiver"" dir=in action=allow program=""{app}\{#AppExeName}"" enable=yes profile=private protocol=TCP"; Flags: runhidden waituntilterminated
Filename: "certutil.exe"; Parameters: "-addstore TrustedPeople ""{app}\MyFolderShell.cer"""; Flags: runhidden waituntilterminated
Filename: "certutil.exe"; Parameters: "-addstore Root ""{app}\MyFolderShell.cer"""; Flags: runhidden waituntilterminated
Filename: "powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -Command ""Get-AppxPackage -Name 'MyFolder.Desktop' | Remove-AppxPackage -ErrorAction SilentlyContinue"""; Flags: runhidden waituntilterminated
Filename: "powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -Command ""Add-AppxPackage -Path '{app}\MyFolderShell.msix' -ExternalLocation '{app}' -ForceUpdateFromAnyVersion"""; Flags: runhidden waituntilterminated
Filename: "{app}\{#AppExeName}"; Description: "启动 MyFolder"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "netsh.exe"; Parameters: "advfirewall firewall delete rule name=""MyFolder LAN Receiver"""; Flags: runhidden waituntilterminated
Filename: "powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -Command ""Get-AppxPackage -Name 'MyFolder.Desktop' | Remove-AppxPackage -ErrorAction SilentlyContinue"""; Flags: runhidden waituntilterminated
Filename: "powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -Command ""$t=Get-Content -LiteralPath '{app}\MyFolderShell.thumbprint' -ErrorAction SilentlyContinue; if($t){{Remove-Item -LiteralPath ('Cert:\LocalMachine\TrustedPeople\'+$t) -Force -ErrorAction SilentlyContinue}}"""; Flags: runhidden waituntilterminated
Filename: "powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -Command ""$t=Get-Content -LiteralPath '{app}\MyFolderShell.thumbprint' -ErrorAction SilentlyContinue; if($t){{Remove-Item -LiteralPath ('Cert:\LocalMachine\Root\'+$t) -Force -ErrorAction SilentlyContinue}}"""; Flags: runhidden waituntilterminated
