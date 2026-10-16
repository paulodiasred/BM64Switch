<#
.SYNOPSIS
    Builds Bomberman 64: Recompiled for the Nintendo Switch using Docker (no local toolchains needed).

.EXAMPLE
    .\build.ps1 generate C:\roms\bomberman64.z64   # once: recompiles the game from your ROM
    .\build.ps1 switch                             # dist\BM64Recompiled.nro
    .\build.ps1 all C:\roms\bomberman64.z64        # generate + switch
    .\build.ps1 nsp C:\keys\prod.keys              # dist\BM64Recompiled.nsp (installable, icon on HOME)
    .\build.ps1 override 0100000000010000          # dist\sd: runs the game in place of an installed one (no keys)
    .\build.ps1 patches                            # after editing patches\*.c
    .\build.ps1 linux                              # Linux test build (build-linux\)
    .\build.ps1 test run.sh                        # runs a tools\test script headless (needs the ROM in run\)
    .\build.ps1 shell                              # shell in the host image
#>
param(
    [Parameter(Position = 0)]
    [ValidateSet('generate', 'patches', 'switch', 'nsp', 'override', 'linux', 'test', 'all', 'shell', 'clean')]
    [string]$Command = 'switch',

    [Parameter(Position = 1, ValueFromRemainingArguments = $true)]
    [string[]]$Rest = @()
)

# Not 'Stop': Windows PowerShell treats anything docker writes to stderr (progress, warnings) as an error when output
# is redirected. Failures are detected through $LASTEXITCODE instead.
$ErrorActionPreference = 'Continue'
$Root = $PSScriptRoot
$HostImage = 'bm64switch-host'
$SwitchImage = 'bm64switch-nx'

function Ensure-Image([string]$Image, [string]$Dockerfile) {
    # (docker image inspect would write to stderr, which Windows PowerShell turns into an error.)
    if (-not (docker images -q $Image)) {
        Write-Host "== Building Docker image $Image"
        docker build -t $Image -f (Join-Path $Root "tools\docker\$Dockerfile") (Join-Path $Root 'tools\docker')
        if ($LASTEXITCODE -ne 0) { throw "docker build $Image failed" }
    }
}

# Runs a command in a container with the repository mounted at /src. Extra -v mounts go in $Mounts.
function Invoke-Container([string]$Image, [string]$Script, [string[]]$Mounts = @()) {
    $DockerArgs = @('run', '--rm', '-v', "${Root}:/src", '-w', '/src') + $Mounts + @($Image, 'bash', '-c', $Script)
    docker @DockerArgs
    if ($LASTEXITCODE -ne 0) { throw "'$Script' failed (exit $LASTEXITCODE)" }
}

function Invoke-Setup {
    Ensure-Image $HostImage 'host.Dockerfile'
    Invoke-Container $HostImage 'bash tools/setup.sh'
}

function Invoke-Generate([string]$Rom) {
    if (-not $Rom) { throw 'Usage: .\build.ps1 generate <path to the Bomberman 64 (USA) ROM>' }
    $RomItem = Get-Item -LiteralPath $Rom
    Invoke-Setup
    # Mount only the ROM's folder, read-only.
    $Mount = @('-v', "$($RomItem.DirectoryName):/rom:ro")
    Invoke-Container $HostImage "bash tools/generate.sh '/rom/$($RomItem.Name)'" $Mount
}

function Invoke-SwitchBuild {
    Invoke-Setup
    Ensure-Image $SwitchImage 'switch.Dockerfile'
    Invoke-Container $SwitchImage 'bash tools/build-switch.sh'
    Write-Host "Done: $(Join-Path $Root 'dist\BM64Recompiled.nro')"
}

switch ($Command) {
    'generate' { Invoke-Generate $Rest[0] }
    'patches' {
        Invoke-Setup
        Invoke-Container $HostImage 'bash tools/generate.sh patches'
    }
    'switch' { Invoke-SwitchBuild }
    'override' {
        if (-not $Rest[0]) { throw 'Usage: .\build.ps1 override <title ID of an installed game, e.g. 0100000000010000>' }
        Invoke-SwitchBuild
        Invoke-Container $SwitchImage "bash tools/build-nsp.sh override '$($Rest[0])'"
        Write-Host "Done: copy the 'atmosphere' folder inside $(Join-Path $Root 'dist\sd') to the root of the SD card."
    }
    'nsp' {
        $Keys = $Rest[0]
        if (-not $Keys) { $Keys = Join-Path $HOME '.switch\prod.keys' }
        if (-not (Test-Path -LiteralPath $Keys)) {
            throw "prod.keys not found ($Keys). Usage: .\build.ps1 nsp <path to your console's prod.keys>"
        }
        $KeysItem = Get-Item -LiteralPath $Keys
        Invoke-SwitchBuild
        Invoke-Container $HostImage 'bash tools/build-nsp.sh fetch'
        Invoke-Container $SwitchImage 'bash tools/build-nsp.sh stage'
        # The only step that sees the keys runs without network access, with just the key file mounted read-only.
        $KeyMount = @('--network', 'none', '-v', "$($KeysItem.FullName):/keys/prod.keys:ro")
        Invoke-Container $HostImage 'bash tools/build-nsp.sh pack' $KeyMount
        Write-Host "Done: $(Join-Path $Root 'dist\BM64Recompiled.nsp')"
    }
    'linux' {
        Invoke-Setup
        Invoke-Container $HostImage 'bash tools/build-linux.sh'
    }
    'test' {
        $TestScript = if ($Rest.Count -gt 0) { $Rest[0] } else { 'run.sh' }
        Ensure-Image $HostImage 'host.Dockerfile'
        # Remaining arguments are passed as environment variables (e.g. SHOTS="10 20" or STEPS="...").
        $EnvArgs = @()
        foreach ($Pair in ($Rest | Select-Object -Skip 1)) { $EnvArgs += @('-e', $Pair) }
        Invoke-Container $HostImage "bash tools/build-linux.sh && bash tools/test/$TestScript" $EnvArgs
    }
    'all' {
        Invoke-Generate $Rest[0]
        Invoke-SwitchBuild
    }
    'shell' {
        Ensure-Image $HostImage 'host.Dockerfile'
        docker run --rm -it -v "${Root}:/src" -w /src $HostImage bash
    }
    'clean' {
        foreach ($Dir in 'build-linux', 'build-switch', 'dist') {
            $Path = Join-Path $Root $Dir
            if (Test-Path $Path) { Remove-Item -Recurse -Force $Path }
        }
    }
}
