# Close the Unreal Editor first, then run this from PowerShell.
# -DryRun : only creates meshes in /Game/_ScratchBroken, blueprints are untouched.
param([switch]$DryRun, [string]$Script = "create_broken_led_meshes.py")
$proj = (Resolve-Path (Join-Path $PSScriptRoot "..\..\FPS_BoranagaJin.uproject")).Path
$script = (Resolve-Path (Join-Path $PSScriptRoot $Script)).Path
if ($DryRun) { $env:BROKEN_DRY_RUN = "1" } else { $env:BROKEN_DRY_RUN = "0" }
& "C:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $proj -EnablePlugins=PythonScriptPlugin -run=pythonscript "-script=$script" -unattended -nosplash -stdout -FullStdOutLogOutput
