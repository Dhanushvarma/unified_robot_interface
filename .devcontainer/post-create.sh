#!/usr/bin/env bash
# Runs once when the devcontainer is created: builds mc_rtc + its dependency
# chain via mc-rtc-superbuild (ROS support disabled), then configures and
# builds this project (mc_rtc_interface) against that install prefix.
set -euo pipefail

BUILD_KORTEX_DRIVER="${BUILD_KORTEX_DRIVER:-ON}"
BUILD_RTDE_DRIVER="${BUILD_RTDE_DRIVER:-ON}"
BUILD_TRIORB_DRIVER="${BUILD_TRIORB_DRIVER:-ON}"
BUILD_TRIORB_TEST_CONTROLLERS="${BUILD_TRIORB_TEST_CONTROLLERS:-ON}"

PROJECT_DIR="${HOME}/mc_rtc_interface"
SUPERBUILD_DIR="${HOME}/superbuild"
WORKSPACE_DIR="${HOME}/workspace"
EXTRA_DEPS_PREFIX="${HOME}/.local/mc_rtc_interface-deps"

BUILD_JOBS="${BUILD_JOBS:-4}"
export CMAKE_BUILD_PARALLEL_LEVEL="${BUILD_JOBS}"

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
      "displayName": "RelWithDebInfo",
      "inherits": ["relwithdebinfo-noble"],
      "cacheVariables": {
        "WITH_ROS_SUPPORT": "ON",
        "BUILD_TESTING": "OFF"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "mc_rtc_interface",
      "displayName": "RelWithDebInfo",
      "configurePreset": "mc_rtc_interface",
      "configuration": "RelWithDebInfo",
      "targets": ["install"]
    }
  ]
}
EOF

echo "==> Building mc_rtc + dependencies via mc-rtc-superbuild with ${BUILD_JOBS} jobs (this can take a while on first run)"
cd "${SUPERBUILD_DIR}"
# mc-rtc-superbuild pip-installs pre-commit for its own git hooks; Ubuntu
# 24.04's system Python rejects system-wide pip installs (PEP 668) otherwise.
PIP_BREAK_SYSTEM_PACKAGES=1 cmake --preset mc_rtc_interface
cmake --build --preset mc_rtc_interface --parallel "${BUILD_JOBS}"

echo "==> Loading installed mc_rtc environment"
if [ ! -f "${WORKSPACE_DIR}/install/setup_mc_rtc.sh" ]; then
  echo "ERROR: Missing ${WORKSPACE_DIR}/install/setup_mc_rtc.sh" >&2
  exit 1
fi
set +u
source "${WORKSPACE_DIR}/install/setup_mc_rtc.sh"
set -u
# export LD_LIBRARY_PATH="${WORKSPACE_DIR}/install/lib:${EXTRA_DEPS_PREFIX}/lib:${LD_LIBRARY_PATH:-}"

echo "==> Building mc_rtc_interface with ${BUILD_JOBS} jobs"

rm -rf "${PROJECT_DIR}/build"

cmake -S "${PROJECT_DIR}" -B "${PROJECT_DIR}/build" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_PREFIX_PATH="${WORKSPACE_DIR}/install;${EXTRA_DEPS_PREFIX}"

cmake --build "${PROJECT_DIR}/build" --parallel "${BUILD_JOBS}"

cmake --install "${PROJECT_DIR}/build"

echo "==> Removing MCFleetControl file capability"

MCFLEET_BIN="${PROJECT_DIR}/build/bin/MCFleetControl"
if [ -f "${MCFLEET_BIN}" ]; then
  sudo setcap -r "${MCFLEET_BIN}" 2>/dev/null || true
fi

if [ "${BUILD_KORTEX_DRIVER}" = "ON" ]; then
  echo "==> Building kortex_driver"

  rm -rf "${PROJECT_DIR}/robot_driver/kortex_driver/build"

  cmake -S "${PROJECT_DIR}/robot_driver/kortex_driver" \
        -B "${PROJECT_DIR}/robot_driver/kortex_driver/build" \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_PREFIX_PATH="${PROJECT_DIR}/build/install;${WORKSPACE_DIR}/install;${EXTRA_DEPS_PREFIX}" \
        -DCMAKE_INSTALL_PREFIX="${PROJECT_DIR}/build/install"

  cmake --build "${PROJECT_DIR}/robot_driver/kortex_driver/build" \
        --parallel "${BUILD_JOBS}"

  cmake --install "${PROJECT_DIR}/robot_driver/kortex_driver/build"
fi

if [ "${BUILD_RTDE_DRIVER}" = "ON" ]; then
  echo "==> Installing UR Client Library"

  sudo apt-get update

  sudo apt-get install -y ros-$ROS_DISTRO-ur-client-library

  echo "==> Building rtde_driver"

  rm -rf "${PROJECT_DIR}/robot_driver/rtde_driver/build"

  cmake -S "${PROJECT_DIR}/robot_driver/rtde_driver" \
        -B "${PROJECT_DIR}/robot_driver/rtde_driver/build" \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_PREFIX_PATH="${PROJECT_DIR}/build/install;${WORKSPACE_DIR}/install;${EXTRA_DEPS_PREFIX}" \
        -DCMAKE_INSTALL_PREFIX="${PROJECT_DIR}/build/install"

  cmake --build "${PROJECT_DIR}/robot_driver/rtde_driver/build" \
        --parallel "${BUILD_JOBS}"

  cmake --install "${PROJECT_DIR}/robot_driver/rtde_driver/build"
fi

if [ "${BUILD_TRIORB_DRIVER}" = "ON" ]; then
  echo "==> Building triorb_driver"

  if [ "${BUILD_TRIORB_TEST_CONTROLLERS}" = "ON" ]; then
    echo "===> With test_controllers"
  fi

  rm -rf "${PROJECT_DIR}/robot_driver/triorb_driver/build"

  cmake -S "${PROJECT_DIR}/robot_driver/triorb_driver" \
        -B "${PROJECT_DIR}/robot_driver/triorb_driver/build" \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DBUILD_CONTROLLER="${BUILD_TRIORB_TEST_CONTROLLERS}" \
        -DCMAKE_PREFIX_PATH="${PROJECT_DIR}/build/install;${WORKSPACE_DIR}/install;${EXTRA_DEPS_PREFIX}" \
        -DCMAKE_INSTALL_PREFIX="${PROJECT_DIR}/build/install"

  cmake --build "${PROJECT_DIR}/robot_driver/triorb_driver/build" \
        --parallel "${BUILD_JOBS}"

  cmake --install "${PROJECT_DIR}/robot_driver/triorb_driver/build"
fi

echo "==> Registering mc_rtc shared libraries"
MC_RTC_LIB_DIR="${WORKSPACE_DIR}/install/lib"
if [ ! -d "${MC_RTC_LIB_DIR}" ]; then
  echo "ERROR: mc_rtc library directory does not exist:" >&2
  echo "       ${MC_RTC_LIB_DIR}" >&2
  exit 1
fi
printf '%s\n' "${MC_RTC_LIB_DIR}" |
  sudo tee /etc/ld.so.conf.d/mc_rtc-interface.conf > /dev/null
sudo ldconfig

echo "==> Setting up environment"
ENV_FILE="${HOME}/.mc_rtc_interface_env"
cat > "${ENV_FILE}" <<EOF
if [ -f "${WORKSPACE_DIR}/install/setup_mc_rtc.sh" ]; then
  source "${WORKSPACE_DIR}/install/setup_mc_rtc.sh"
fi
export PATH="${PROJECT_DIR}/build/bin:${WORKSPACE_DIR}/install/bin:\${PATH}"
export CMAKE_PREFIX_PATH="${WORKSPACE_DIR}/install:${EXTRA_DEPS_PREFIX}:\${CMAKE_PREFIX_PATH:-}"
export LD_LIBRARY_PATH="${WORKSPACE_DIR}/install/lib:${EXTRA_DEPS_PREFIX}/lib:\${LD_LIBRARY_PATH:-}"
EOF

ENV_SOURCE_LINE='source "$HOME/.mc_rtc_interface_env"'

for STARTUP_FILE in "${HOME}/.bashrc" "${HOME}/.profile"; do
  if ! grep -qxF "${ENV_SOURCE_LINE}" "${STARTUP_FILE}" 2>/dev/null; then
    printf '\n%s\n' "${ENV_SOURCE_LINE}" >> "${STARTUP_FILE}"
  fi
done

echo "==> Verifying shell startup configuration"

grep -qxF "${ENV_SOURCE_LINE}" "${HOME}/.bashrc"
grep -qxF "${ENV_SOURCE_LINE}" "${HOME}/.profile"

test -x "${PROJECT_DIR}/build/bin/MCFleetControl"
test -x "${PROJECT_DIR}/build/bin/RobotInterface"

echo "==> Done. The environment will be loaded automatically in new Bash sessions."
