$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$toolRoot=Join-Path $projectRoot 'tools/android-build'
$env:JAVA_HOME=(Get-ChildItem "$toolRoot/jdk" -Directory | Select-Object -First 1).FullName
$env:ANDROID_HOME=Join-Path $toolRoot 'sdk'
$env:GRADLE_USER_HOME=Join-Path $toolRoot 'gradle-cache'
& "$toolRoot/gradle-8.9/bin/gradle.bat" -p $PSScriptRoot --no-daemon assembleDebug lintDebug
if ($LASTEXITCODE -ne 0) { throw 'Android build/check failed' }
$testOutput=Join-Path $PSScriptRoot 'app/build/packet-tests'
New-Item -ItemType Directory -Force $testOutput | Out-Null
& "$env:JAVA_HOME/bin/javac.exe" -d $testOutput "$PSScriptRoot/app/src/main/java/tr/deney/espmonitor/Packet.java" "$PSScriptRoot/tests/PacketTest.java"
if ($LASTEXITCODE -ne 0) { throw 'Packet test compilation failed' }
& "$env:JAVA_HOME/bin/java.exe" -cp $testOutput tr.deney.espmonitor.PacketTest
if ($LASTEXITCODE -ne 0) { throw 'Packet tests failed' }