#!/usr/bin/env bash
# Runs once when the devcontainer is created: builds mc_rtc + its dependency
# chain via mc-rtc-superbuild (ROS support disabled), then configures and
# builds this project (mc_rtc_interface) against that install prefix.
set -euo pipefail

SUPERBUILD_DIR="${HOME}/superbuild"
WORKSPACE_DIR="${HOME}/workspace"
PROJECT_DIR="${HOME}/mc_rtc_interface"

# Fresh named volumes mount root-owned; VSCode's Dev Containers extension
# normally fixes this itself, but don't depend on that lifecycle step running
# (e.g. DevPod's docker provider, or a plain `docker run`, may not do it).
mkdir -p "${SUPERBUILD_DIR}" "${WORKSPACE_DIR}"
sudo chown -R "$(id -u):$(id -g)" "${SUPERBUILD_DIR}" "${WORKSPACE_DIR}"

# mc-rtc-superbuild creates a meta-repository with submodules, which needs a
# git identity. Fall back to a placeholder if none was forwarded from the host
# (see the .gitconfig mount in devcontainer.json).
if [ -z "$(git config --global user.email || true)" ]; then
  git config --global user.name "mc_rtc_interface devcontainer"
  git config --global user.email "devcontainer@mc_rtc_interface.local"
fi

if [ ! -d "${SUPERBUILD_DIR}/.git" ]; then
  echo "==> Cloning mc-rtc-superbuild"
  git clone https://github.com/mc-rtc/mc-rtc-superbuild "${SUPERBUILD_DIR}"
fi

# Custom preset: same as relwithdebinfo-noble but with ROS support disabled,
# per this project's choice (its own robot_manager/robot_interface code does
# not use ROS; only some of mc_rtc's own bundled plugins/controllers do).
cat > "${SUPERBUILD_DIR}/CMakeUserPresets.json" <<'EOF'
{
  "version": 10,
  "configurePresets": [
    {
      "name": "mc_rtc_interface",
      "displayName": "RelWithDebInfo (noble, no ROS)",
      "inherits": ["relwithdebinfo-noble"],
      "cacheVariables": {
        "WITH_ROS_SUPPORT": "OFF"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "mc_rtc_interface",
      "displayName": "RelWithDebInfo (noble, no ROS)",
      "configurePreset": "mc_rtc_interface",
      "configuration": "RelWithDebInfo",
      "targets": ["install"]
    }
  ]
}
EOF

echo "==> Building mc_rtc + dependencies via mc-rtc-superbuild (this can take a while on first run)"
cd "${SUPERBUILD_DIR}"
# mc-rtc-superbuild pip-installs pre-commit for its own git hooks; Ubuntu
# 24.04's system Python rejects system-wide pip installs (PEP 668) otherwise.
PIP_BREAK_SYSTEM_PACKAGES=1 cmake --preset mc_rtc_interface
cmake --build --preset mc_rtc_interface

echo "==> Configuring mc_rtc_interface"
cmake -S "${PROJECT_DIR}" -B "${PROJECT_DIR}/build" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_PREFIX_PATH="${WORKSPACE_DIR}/install:${HOME}/.local/zenoh"

echo "==> Building mc_rtc_interface"
cmake --build "${PROJECT_DIR}/build" --parallel "$(nproc)"

echo "==> Done. Source ${WORKSPACE_DIR}/install/setup_mc_rtc.sh in new shells to pick up mc_rtc's environment."
