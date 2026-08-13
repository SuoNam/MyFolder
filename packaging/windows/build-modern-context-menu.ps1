param(
    [Parameter(Mandatory = $true)][string]$BuildDirectory,
    [Parameter(Mandatory = $true)][string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
$sourceDirectory = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$sdkBin = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64'
$makeAppx = Join-Path $sdkBin 'makeappx.exe'
$signTool = Join-Path $sdkBin 'signtool.exe'
$magick = 'C:\Program Files\ImageMagick-7.1.2-Q16-HDRI\magick.exe'
$mingwBin = 'G:\Qt\Tools\mingw1310_64\bin'
$layout = Join-Path $OutputDirectory 'sparse-layout'
$assets = Join-Path $layout 'Assets'

New-Item -ItemType Directory -Force -Path $assets | Out-Null
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'AppxManifest.xml') -Destination (Join-Path $layout 'AppxManifest.xml') -Force
Copy-Item -LiteralPath (Join-Path $BuildDirectory 'MyFolderShellExtension.dll') -Destination (Join-Path $layout 'MyFolderShellExtension.dll') -Force
# Explorer loads the COM server outside the Qt application process. Keep the
# MinGW threading runtime beside the DLL so dllhost.exe can resolve it.
Copy-Item -LiteralPath (Join-Path $mingwBin 'libwinpthread-1.dll') -Destination (Join-Path $layout 'libwinpthread-1.dll') -Force
# MakeAppx requires the declared executable to exist in the layout even for an
# external-location package. The shell commands still launch the Inno-installed
# executable via HKCU\Software\MyFolder\InstallPath.
Copy-Item -LiteralPath (Join-Path $BuildDirectory 'appMyFolder.exe') -Destination (Join-Path $layout 'appMyFolder.exe') -Force

$icon = Join-Path $sourceDirectory 'Icons\app.png'
& $magick $icon -resize '50x50!' (Join-Path $assets 'StoreLogo.png')
& $magick $icon -resize '44x44!' (Join-Path $assets 'Square44x44Logo.png')
& $magick $icon -resize '150x150!' (Join-Path $assets 'Square150x150Logo.png')

$certificate = Get-ChildItem Cert:\CurrentUser\My | Where-Object {
    $_.Subject -eq 'CN=MyFolder' -and $_.HasPrivateKey -and
    $_.EnhancedKeyUsageList.ObjectId -contains '1.3.6.1.5.5.7.3.3'
} | Sort-Object NotAfter -Descending | Select-Object -First 1
if (-not $certificate -or $certificate.NotAfter -lt (Get-Date).AddMonths(6)) {
    $certificate = New-SelfSignedCertificate -Type Custom -Subject 'CN=MyFolder' `
        -FriendlyName 'MyFolder package signing' -CertStoreLocation 'Cert:\CurrentUser\My' `
        -KeyAlgorithm RSA -KeyLength 3072 -HashAlgorithm SHA256 -KeyUsage DigitalSignature `
        -NotAfter (Get-Date).AddYears(5) `
        -TextExtension @('2.5.29.37={text}1.3.6.1.5.5.7.3.3')
}

$packagePath = Join-Path $OutputDirectory 'MyFolderShell.msix'
$certificatePath = Join-Path $OutputDirectory 'MyFolderShell.cer'
$thumbprintPath = Join-Path $OutputDirectory 'MyFolderShell.thumbprint'
Export-Certificate -Cert $certificate -FilePath $certificatePath -Force | Out-Null
Set-Content -LiteralPath $thumbprintPath -Value $certificate.Thumbprint -Encoding ascii -NoNewline

if (Test-Path -LiteralPath $packagePath) { Remove-Item -LiteralPath $packagePath -Force }
& $makeAppx pack /d $layout /p $packagePath /o
if ($LASTEXITCODE -ne 0) { throw "MakeAppx failed with exit code $LASTEXITCODE" }
& $signTool sign /fd SHA256 /sha1 $certificate.Thumbprint /s My $packagePath
if ($LASTEXITCODE -ne 0) { throw "SignTool failed with exit code $LASTEXITCODE" }

Get-Item -LiteralPath $packagePath, $certificatePath, $thumbprintPath
