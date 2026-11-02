param(
    [Parameter(Mandatory=$true)][string]$QtAndroid,
    [Parameter(Mandatory=$true)][string]$QtHost,
    [Parameter(Mandatory=$true)][string]$OpenCVDir,
    [Parameter(Mandatory=$true)][string]$SdkRoot,
    [Parameter(Mandatory=$true)][string]$NdkRoot,
    [Parameter(Mandatory=$true)][string]$JdkRoot
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$buildDir = Join-Path $projectRoot 'build-arm64'
$apkDir = Join-Path $projectRoot 'artifacts'
$savedJava = $env:JAVA_HOME
try {
    $env:JAVA_HOME = $JdkRoot
    & cmake -S $projectRoot -B $buildDir -G Ninja `
        '-DCMAKE_BUILD_TYPE=Debug' `
        "-DCMAKE_TOOLCHAIN_FILE=$QtAndroid/lib/cmake/Qt6/qt.toolchain.cmake" `
        "-DQT_HOST_PATH=$QtHost" "-DOpenCV_DIR=$OpenCVDir" `
        "-DANDROID_SDK_ROOT=$SdkRoot" "-DANDROID_NDK_ROOT=$NdkRoot" `
        '-DANDROID_ABI=arm64-v8a' '-DANDROID_PLATFORM=android-23'
    if ($LASTEXITCODE -ne 0) { throw 'Android configure failed' }
    & cmake --build $buildDir --target MarkScan_prepare_apk_dir
    if ($LASTEXITCODE -ne 0) { throw 'Android native build failed' }
    New-Item -ItemType Directory -Force $apkDir | Out-Null
    & (Join-Path $QtHost 'bin/androiddeployqt.exe') `
        --input (Join-Path $buildDir 'android-MarkScan-deployment-settings.json') `
        --output (Join-Path $buildDir 'android-build') `
        --android-platform android-33 --jdk $JdkRoot `
        --apk (Join-Path $apkDir 'MarkScan-debug-arm64.apk')
    if ($LASTEXITCODE -ne 0) { throw 'Android packaging failed' }
} finally {
    $env:JAVA_HOME = $savedJava
}
