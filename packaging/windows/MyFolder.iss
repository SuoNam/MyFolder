#ifndef SourceDir
  #error SourceDir must point to the deployed application directory
#endif

#ifndef OutputDir
  #define OutputDir "."
#endif

#define AppName "MyFolder"
#define AppVersion "1.1.1"
#define AppPublisher "MyFolder"
#define AppExeName "appMyFolder.exe"

[Setup]
AppId={{B36DF7B3-477B-4A2D-8B6D-F35CF7EF8944}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={localappdata}\Programs\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#OutputDir}
OutputBaseFilename=MyFolder-v{#AppVersion}-Windows-x64-Setup
SetupIconFile={#SourcePath}\..\..\Icons\app.ico
UninstallDisplayIcon={app}\{#AppExeName}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
RestartApplications=no
VersionInfoVersion={#AppVersion}.0
VersionInfoProductName={#AppName}
VersionInfoProductVersion={#AppVersion}

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "其他选项："; Flags: checkedonce

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\MyFolder"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"
Name: "{group}\卸载 MyFolder"; Filename: "{uninstallexe}"
Name: "{autodesktop}\MyFolder"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "Software\Classes\myfolder"; ValueType: string; ValueName: ""; ValueData: "URL:MyFolder OAuth Callback"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\myfolder"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\myfolder\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExeName},0"
Root: HKCU; Subkey: "Software\Classes\myfolder\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" ""%1"""
Root: HKCU; Subkey: "Software\Classes\*\shell\MyFolder.UploadServer"; ValueType: string; ValueName: ""; ValueData: "发送到 MyFolder 服务器"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\*\shell\MyFolder.UploadServer"; ValueType: string; ValueName: "Icon"; ValueData: "{app}\{#AppExeName},0"
Root: HKCU; Subkey: "Software\Classes\*\shell\MyFolder.UploadServer\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" --upload-server ""%1"""
Root: HKCU; Subkey: "Software\Classes\*\shell\MyFolder.SendClient"; ValueType: string; ValueName: ""; ValueData: "发送到指定 MyFolder 客户端"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\*\shell\MyFolder.SendClient"; ValueType: string; ValueName: "Icon"; ValueData: "{app}\{#AppExeName},0"
Root: HKCU; Subkey: "Software\Classes\*\shell\MyFolder.SendClient\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" --send-client ""%1"""

[Run]
Filename: "{app}\{#AppExeName}"; Description: "启动 MyFolder"; Flags: nowait postinstall skipifsilent
