# Extends the official mc-rtc-superbuild devcontainer image (Ubuntu 24.04 / noble)
# with mc_rtc_interface's own extra dependencies that are outside the superbuild's
# scope: zenoh-c / zenoh-cpp (zenohcxx), FlatBuffers, Protobuf, and GTest.
#
# The base image already provides: build tooling, ccache pre-populated for
# mc-rtc-superbuild, boost/eigen/etc. needed by mc_rtc itself.
FROM ghcr.io/mc-rtc/mc-rtc-superbuild:noble-devcontainer-latest

USER root

# ── Extra system dependencies specific to mc_rtc_interface ──────────────────
# Note: FlatBuffers is NOT installed via apt here - Ubuntu 24.04 ships 2.0.8
# (2021), which lacks the flatbuffers_generate_headers() CMake helper this
# project's build relies on. Built from source below instead.
RUN --mount=type=cache,target=/var/cache/apt \
    apt-get update && apt-get install -y --no-install-recommends \
      protobuf-compiler \
      libprotobuf-dev \
      libgtest-dev \
      libnanomsg-dev \
      libltdl-dev \
      libboost-program-options-dev \
      libboost-thread-dev \
      pkg-config \
      curl \
      openssh-server \
      xauth \
      x11-apps \
      mesa-utils \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /run/sshd /etc/ssh/sshd_config.d \
    && chmod 0755 /run/sshd \
    && printf '%s\n' \
         'X11Forwarding yes' \
         'X11UseLocalhost yes' \
         'XAuthLocation /usr/bin/xauth' \
       > /etc/ssh/sshd_config.d/60-x11-forwarding.conf \
    && sshd -t

USER vscode

# ── Rust toolchain (needed to build zenoh-c) ─────────────────────────────────
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --profile minimal
ENV PATH="/home/vscode/.cargo/bin:${PATH}"

ARG ZENOH_C_VERSION=1.9.0
ARG ZENOH_CPP_VERSION=1.9.0
ARG FLATBUFFERS_VERSION=25.12.19
# Shared install prefix for this project's from-source dependencies.
ARG EXTRA_DEPS_PREFIX=/home/vscode/.local/mc_rtc_interface-deps
ARG ZENOH_INSTALL_PREFIX=${EXTRA_DEPS_PREFIX}

# ── FlatBuffers: build a current release from source (see note above) ──────
RUN git clone --branch v${FLATBUFFERS_VERSION} --depth 1 https://github.com/google/flatbuffers.git /tmp/flatbuffers \
    && cmake -S /tmp/flatbuffers -B /tmp/flatbuffers/build \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_INSTALL_PREFIX=${EXTRA_DEPS_PREFIX} \
        -DFLATBUFFERS_BUILD_TESTS=OFF \
    && cmake --build /tmp/flatbuffers/build --parallel $(nproc) \
    && cmake --install /tmp/flatbuffers/build \
    && rm -rf /tmp/flatbuffers

# ── zenoh-c: build from source (release binaries don't ship a CMake config
#    with the shared-memory feature enabled, which this project relies on) ──
RUN git clone --branch ${ZENOH_C_VERSION} --depth 1 https://github.com/eclipse-zenoh/zenoh-c.git /tmp/zenoh-c \
    && cmake -S /tmp/zenoh-c -B /tmp/zenoh-c/build \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_INSTALL_PREFIX=${ZENOH_INSTALL_PREFIX} \
        -DZENOHC_BUILD_WITH_SHARED_MEMORY=ON \
    && cmake --build /tmp/zenoh-c/build --parallel $(nproc) \
    && cmake --install /tmp/zenoh-c/build \
    && rm -rf /tmp/zenoh-c

# ── zenoh-cpp (zenohcxx): header-only C++ bindings over zenoh-c ─────────────
RUN git clone --branch ${ZENOH_CPP_VERSION} --depth 1 https://github.com/eclipse-zenoh/zenoh-cpp.git /tmp/zenoh-cpp \
    && cmake -S /tmp/zenoh-cpp -B /tmp/zenoh-cpp/build \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_INSTALL_PREFIX=${ZENOH_INSTALL_PREFIX} \
        -DCMAKE_PREFIX_PATH=${ZENOH_INSTALL_PREFIX} \
        -DZENOHCXX_ZENOHC=ON \
        -DZENOHCXX_ZENOHPICO=OFF \
        -DZENOHCXX_EXAMPLES=OFF \
    && cmake --build /tmp/zenoh-cpp/build --parallel $(nproc) \
    && cmake --install /tmp/zenoh-cpp/build \
    && rm -rf /tmp/zenoh-cpp

# Make zenoh discoverable by CMake's find_package() for this project and
# the mc-rtc-superbuild install prefix (set up by post-create.sh at runtime).
ENV CMAKE_PREFIX_PATH="${ZENOH_INSTALL_PREFIX}:${CMAKE_PREFIX_PATH}"
ENV LD_LIBRARY_PATH="${ZENOH_INSTALL_PREFIX}/lib:${LD_LIBRARY_PATH}"

WORKDIR /home/vscode/mc_rtc_interface
