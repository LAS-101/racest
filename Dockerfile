# syntax=docker/dockerfile:1
ARG EMSDK_VERSION=4.0.23
ARG PYTHON_VERSION=3.12

FROM docker.io/emscripten/emsdk:${EMSDK_VERSION} AS web-build
WORKDIR /app
COPY CMakeLists.txt ./
COPY src/game/ src/game/
COPY assets/ assets/
COPY web/ web/
RUN emcmake cmake -S . -B build/web -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build/web --parallel 2

FROM docker.io/library/debian:bookworm-slim AS native-build
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates cmake g++ git make pkg-config python3 \
        libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY CMakeLists.txt ./
COPY src/ src/
COPY assets/ assets/
COPY tests/ tests/
RUN cmake -S . -B build/native -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
    && cmake --build build/native --parallel 2 \
    && ctest --test-dir build/native --output-on-failure

FROM docker.io/library/python:${PYTHON_VERSION}-slim-bookworm AS runtime-base
ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1
WORKDIR /app
RUN groupadd --gid 10001 racest \
    && useradd --uid 10001 --gid racest --create-home racest \
    && mkdir -p /app/data \
    && chown racest:racest /app/data
COPY src/agent/ src/agent/
COPY src/words/ src/words/
COPY assets/ assets/
USER racest
ENTRYPOINT ["python", "src/agent/run.py"]

FROM runtime-base AS simulation
USER root
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        libgl1 libx11-6 libxrandr2 libxi6 libxcursor1 libxinerama1 \
    && rm -rf /var/lib/apt/lists/*
COPY --from=native-build /app/build/native/racest build/native/racest
USER racest
CMD ["--simulate", "--seconds", "300", "--seed", "42"]

FROM runtime-base AS web
COPY --from=web-build /app/build/web/public/ build/web/public/
EXPOSE 8000
HEALTHCHECK --interval=10s --timeout=5s --start-period=15s --retries=3 \
    CMD ["python", "-c", "import urllib.request; urllib.request.urlopen('http://127.0.0.1:8000/api/agent/config', timeout=3).close()"]
CMD ["--web", "--host", "0.0.0.0", "--online-words"]
