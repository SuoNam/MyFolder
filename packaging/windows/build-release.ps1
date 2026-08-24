param(
    [string]$BuildDirectory = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'build-verify-lan-proxy')
)

$ErrorActionPreference = 'Stop'
$sourceDirectory = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$distDirectory = Join-Path $sourceDirectory 'dist'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$stageDirectory = Join-Path $distDirectory "stage-v1.1.1-release-$stamp"
$installerDirectory = Join-Path $distDirectory "installer-$stamp"
$versionedPortable = Join-Path $distDirectory "MyFolder-v1.1.1-Windows-x64-Portable-$stamp.zip"
$latestPortable = Join-Path $distDirectory 'MyFolder-v1.1.1-Windows-x64-Portable.zip'
$latestSetup = Join-Path $distDirectory 'MyFolder-v1.1.1-Windows-x64-Setup.exe'
$latestMsix = Join-Path $distDirectory 'MyFolder-v1.1.1-Windows-x64-Shell.msix'
$windeployqt = 'G:\Qt\6.9.1\mingw_64\bin\windeployqt.exe'
$iscc = Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe'

cmake --build $BuildDirectory --config Release --parallel 4
if ($LASTEXITCODE -ne 0) { throw "Client build failed with exit code $LASTEXITCODE" }

New-Item -ItemType Directory -Path $stageDirectory, $installerDirectory | Out-Null
Copy-Item -LiteralPath (Join-Path $BuildDirectory 'appMyFolder.exe') -Destination $stageDirectory

& $windeployqt --release --qmldir $sourceDirectory --compiler-runtime `
    (Join-Path $stageDirectory 'appMyFolder.exe') *> (Join-Path $stageDirectory 'windeployqt.log')
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed with exit code $LASTEXITCODE" }

Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'qt.conf') -Destination $stageDirectory
& (Join-Path $PSScriptRoot 'build-modern-context-menu.ps1') `
    -BuildDirectory $BuildDirectory -OutputDirectory $stageDirectory `
    *> (Join-Path $stageDirectory 'msix-build.log')
if ($LASTEXITCODE -ne 0) { throw "MSIX build failed with exit code $LASTEXITCODE" }

Compress-Archive -Path (Join-Path $stageDirectory '*') `
    -DestinationPath $versionedPortable -CompressionLevel Optimal

& $iscc "/DSourceDir=$stageDirectory" "/DOutputDir=$installerDirectory" `
    (Join-Path $PSScriptRoot 'MyFolder.iss') *> (Join-Path $installerDirectory 'iscc-build.log')
if ($LASTEXITCODE -ne 0) { throw "Inno Setup failed with exit code $LASTEXITCODE" }

$versionedSetup = Join-Path $installerDirectory 'MyFolder-v1.1.1-Windows-x64-Setup.exe'
Copy-Item -LiteralPath $versionedSetup -Destination $latestSetup -Force
Copy-Item -LiteralPath $versionedPortable -Destination $latestPortable -Force
Copy-Item -LiteralPath (Join-Path $stageDirectory 'MyFolderShell.msix') -Destination $latestMsix -Force
Set-Content -LiteralPath (Join-Path $distDirectory 'latest-stage.txt') `
    -Value $stageDirectory -Encoding utf8NoBOM -NoNewline

$artifacts = Get-Item -LiteralPath $latestSetup, $latestPortable, $latestMsix
$result = [ordered]@{
    stamp = $stamp
    stage = $stageDirectory
    versionedSetup = $versionedSetup
    versionedPortable = $versionedPortable
    artifacts = @($artifacts | ForEach-Object {
        [ordered]@{
            path = $_.FullName
            length = $_.Length
            sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash
        }
    })
}
$result | ConvertTo-Json -Depth 4
