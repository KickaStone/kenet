# 使用超轻量基础镜像
FROM nexus3.kksk.dev/debian:bookworm-slim

# 把可执行文件拷贝进去
COPY press_client /usr/local/bin/press_client

# 确保有执行权限
RUN chmod +x /usr/local/bin/press_client

# 设置默认工作目录
WORKDIR /app

# 容器启动后执行程序（可被 docker run 命令覆盖）
ENTRYPOINT ["/usr/local/bin/press_client"]

# docker run --rm press_client -h 172.17.0.2 -p 8080 -c 1000 -t 60