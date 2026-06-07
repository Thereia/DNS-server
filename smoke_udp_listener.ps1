$ErrorActionPreference = "Stop"

$root = $PSScriptRoot
$serverExe = Join-Path $root "build\dnsrelay_stage1.exe"
$senderExe = Join-Path $root "build\udp_sender.exe"
$serverLog = Join-Path $root "build\stage1-server.log"

if (Test-Path $serverLog) {
    Remove-Item -LiteralPath $serverLog -Force
}

$server = Start-Process -FilePath $serverExe -PassThru -RedirectStandardOutput $serverLog
Start-Sleep -Milliseconds 500

& $senderExe "127.0.0.1" 5533 "hello-stage1"
Start-Sleep -Milliseconds 500

$output = Get-Content -LiteralPath $serverLog -Raw
$server | Stop-Process -Force

if ($output -notmatch "received 12 bytes from 127\.0\.0\.1") {
    throw "Server log did not contain the expected receive trace."
}
