# PKE Docker 运行脚本

# 检查是否已有运行中的 pke_container 容器
$existing = docker ps -a --filter 'name=pke_container' --format '{{.Names}}'

if ($existing -eq 'pke_container') {
    Write-Host '容器已存在，正在启动...' -ForegroundColor Yellow
    docker start pke_container
    Write-Host '进入容器...' -ForegroundColor Green
    docker exec -it pke_container /bin/bash
} else {
    Write-Host '创建新容器并运行...' -ForegroundColor Green
    # 挂载本地代码目录到容器中
    docker run -it --name pke_container 
        -v 'C:\Users\honor\Desktop\OS2025LAB\riscv-pke:/workspace' 
        -w /workspace/riscv-pke 
        tjr9098/amd64_pke_mirrors:1.0 /bin/bash
}
