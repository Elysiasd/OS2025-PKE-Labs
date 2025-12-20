# PKE Docker run and debug script
param(
    [Parameter(Position=0)]
    [ValidateSet('build', 'run', 'debug', 'clean', 'shell', 'objdump')]
    [string]$Command = 'run'
)

$DOCKER_IMAGE = 'tjr9098/amd64_pke_mirrors:1.0'
$CONTAINER_NAME = 'pke_container'
$WORKSPACE_PATH = $PWD.Path
$CONTAINER_WORKSPACE = '/workspace'

Write-Host "`n====== PKE Experiment Docker Runner ======" -ForegroundColor Cyan
Write-Host "Working directory: $WORKSPACE_PATH" -ForegroundColor Gray
Write-Host "Current branch: " -NoNewline -ForegroundColor Gray
git branch --show-current
Write-Host "==========================================`n" -ForegroundColor Cyan

# Check if Docker is running
try {
    docker info | Out-Null
} catch {
    Write-Host "Error: Docker is not running!" -ForegroundColor Red
    exit 1
}

# Check if container exists
$existing = docker ps -a --filter "name=$CONTAINER_NAME" --format '{{.Names}}'

function Run-InContainer {
    param([string]$cmd)
    
    if ($existing -eq $CONTAINER_NAME) {
        $status = docker ps --filter "name=$CONTAINER_NAME" --format '{{.Status}}'
        if (-not $status) {
            Write-Host "Starting existing container..." -ForegroundColor Yellow
            docker start $CONTAINER_NAME | Out-Null
        }
        docker exec -i $CONTAINER_NAME bash -c $cmd
    } else {
        Write-Host "Creating new container..." -ForegroundColor Green
        docker run --rm -v "${WORKSPACE_PATH}:${CONTAINER_WORKSPACE}" -w $CONTAINER_WORKSPACE $DOCKER_IMAGE bash -c $cmd
    }
}

switch ($Command) {
    'build' {
        Write-Host "`n[Build] Compiling PKE kernel and application..." -ForegroundColor Green
        Run-InContainer 'make clean && make'
    }
    'run' {
        Write-Host "`n[Run] Compile and run experiment..." -ForegroundColor Green
        Run-InContainer 'make clean && make && make run'
    }
    'debug' {
        Write-Host "`n[Debug] Compile and show detailed info..." -ForegroundColor Green
        $debugCmd = 'make clean && make && echo "====== Kernel Info ======" && riscv64-unknown-elf-readelf -h obj/riscv-pke && echo "" && echo "====== App Info ======" && riscv64-unknown-elf-readelf -h obj/app_* && echo "" && echo "====== Run Result ======" && make run'
        Run-InContainer $debugCmd
    }
    'clean' {
        Write-Host "`n[Clean] Cleaning build artifacts..." -ForegroundColor Yellow
        Run-InContainer 'make clean'
    }
    'objdump' {
        Write-Host "`n[Objdump] Generating disassembly files..." -ForegroundColor Green
        $objdumpCmd = 'make objdump && echo "Disassembly files generated:" && ls -lh obj/*_dump'
        Run-InContainer $objdumpCmd
    }
    'shell' {
        Write-Host "`n[Shell] Entering interactive shell..." -ForegroundColor Cyan
        if ($existing -eq $CONTAINER_NAME) {
            docker start $CONTAINER_NAME | Out-Null
            docker exec -it $CONTAINER_NAME /bin/bash
        } else {
            docker run -it --name $CONTAINER_NAME -v "${WORKSPACE_PATH}:${CONTAINER_WORKSPACE}" -w $CONTAINER_WORKSPACE $DOCKER_IMAGE /bin/bash
        }
    }
}

Write-Host "`n====== Execution Complete ======" -ForegroundColor Cyan

