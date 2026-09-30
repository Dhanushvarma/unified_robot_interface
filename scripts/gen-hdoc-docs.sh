#!/usr/bin/env bash
# Builds hdoc (https://hdoc.io, AGPLv3) if needed, then runs it over this
# project to generate static API documentation from build/compile_commands.json
# per .hdoc.toml. hdoc is not vendored in this repo; its source is fetched and
# the binary is cached under $HDOC_CACHE_DIR so this is only slow once.
#
# Usage: scripts/gen-hdoc-docs.sh [extra hdoc args]
# Env:   HDOC_CACHE_DIR (default: ~/.cache/hdoc-build)
#        FORCE_HDOC_REBUILD=1 to rebuild hdoc even if already cached
set -euo pipefail

HDOC_VERSION="1.4.1"
HDOC_CACHE_DIR="${HDOC_CACHE_DIR:-$HOME/.cache/hdoc-build}"
HDOC_SRC_DIR="${HDOC_CACHE_DIR}/hdoc-${HDOC_VERSION}"
HDOC_BIN="${HDOC_SRC_DIR}/build/hdoc"

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# hdoc requires LLVM/Clang 14: force it even if a newer version is installed.
patch_meson_build_for_llvm_pinning() {
  sed -i \
    -e "s/dependency('LLVM', include_type: 'system')\$/dependency('LLVM', include_type: 'system', method: 'config-tool')/" \
    -e "s|dependency('Clang', include_type: 'system', method: 'cmake', modules: clang_modules)\$|dependency('Clang', include_type: 'system', method: 'cmake', modules: clang_modules, cmake_args: ['-DClang_DIR=' + llvm_libdir / 'cmake' / 'clang', '-DLLVM_DIR=' + llvm_libdir / 'cmake' / 'llvm'])|" \
    "${HDOC_SRC_DIR}/meson.build"
}

find_llvm_config() {
  for v in 14; do
    if command -v "llvm-config-${v}" >/dev/null 2>&1; then
      command -v "llvm-config-${v}"
      return 0
    fi
  done
  return 1
}

# LLVM 14 headers don't compile against libstdc++ >= 12 in C++20: use an older GCC toolchain.
try_gcc_toolchain_workaround() {
  local current_gcc newer_gcc
  current_gcc="$(clang-14 -v 2>&1 | grep -oP 'Selected GCC installation: \K.*' || true)"
  local older
  older="$(ls -d /usr/lib/gcc/x86_64-linux-gnu/*/ 2>/dev/null | sed 's:/$::' | sort -V | head -1)"
  if [ -z "${older}" ] || [ "${older}" = "${current_gcc}" ]; then
    return 1
  fi

  echo "==> Working around LLVM-14/libstdc++ incompatibility using $(basename "${older}")'s libstdc++ headers"
  local shim="${HDOC_CACHE_DIR}/gcc-toolchain-shim"
  mkdir -p "${shim}/lib/gcc/x86_64-linux-gnu"
  ln -sf "${older}" "${shim}/lib/gcc/x86_64-linux-gnu/$(basename "${older}")"
  export CFLAGS="--gcc-toolchain=${shim} ${CFLAGS:-}"
  export CXXFLAGS="--gcc-toolchain=${shim} ${CXXFLAGS:-}"
  export LDFLAGS="--gcc-toolchain=${shim} ${LDFLAGS:-}"
  return 0
}

build_hdoc() {
  echo "==> Building hdoc ${HDOC_VERSION} (one-time; cached at ${HDOC_SRC_DIR})"

  if ! command -v meson >/dev/null 2>&1; then
    echo "==> Installing meson (user-local, via pip)"
    pip3 install --user meson
  fi
  export PATH="$HOME/.local/bin:${PATH}"

  local llvm_config
  if ! llvm_config="$(find_llvm_config)"; then
    echo "error: llvm-config-14 not found." >&2
    echo "hdoc is pinned to LLVM/Clang 14. Install it with:" >&2
    echo "  sudo apt-get install llvm-14-dev libclang-14-dev clang-14" >&2
    exit 1
  fi

  if [ ! -d "${HDOC_SRC_DIR}" ]; then
    mkdir -p "${HDOC_CACHE_DIR}"
    git clone --branch "${HDOC_VERSION}" --depth 1 https://github.com/hdoc/hdoc.git "${HDOC_SRC_DIR}"
  fi
  patch_meson_build_for_llvm_pinning

  export CC=clang-14
  export CXX=clang++-14
  export LLVM_CONFIG="${llvm_config}"

  rm -rf "${HDOC_SRC_DIR}/build"
  ( cd "${HDOC_SRC_DIR}" && meson setup build )

  local log="${HDOC_CACHE_DIR}/build.log"
  if ( cd "${HDOC_SRC_DIR}" && ninja -C build hdoc ) >"${log}" 2>&1; then
    return 0
  fi

  if grep -q "forward declaration of 'llvm::json::Value'" "${log}" \
      && try_gcc_toolchain_workaround; then
    rm -rf "${HDOC_SRC_DIR}/build"
    ( cd "${HDOC_SRC_DIR}" && meson setup build )
    ( cd "${HDOC_SRC_DIR}" && ninja -C build hdoc )
    return 0
  fi

  echo "==> hdoc build failed:" >&2
  cat "${log}" >&2
  exit 1
}

if [ "${FORCE_HDOC_REBUILD:-0}" = "1" ] || [ ! -x "${HDOC_BIN}" ]; then
  build_hdoc
fi

cd "${PROJECT_ROOT}"

if [ ! -f "build/compile_commands.json" ]; then
  echo "error: build/compile_commands.json not found." >&2
  echo "Configure the project first (see README.md), e.g.:" >&2
  echo "  cmake -S . -B build -DCMAKE_PREFIX_PATH=..." >&2
  exit 1
fi

"${HDOC_BIN}" --verbose "$@"

sed -i 's|<li><a href="https://hdoc.io">Made with hdoc</a></li>||g' hdoc-output/*.html

REPO_URL="$(sed -n 's/^git_repo_url *= *"\(.*\)"/\1/p' .hdoc.toml)"
if [ -n "${REPO_URL}" ]; then
  GITHUB_LINK="<a href=\"${REPO_URL}\" title=\"View on GitHub\" aria-label=\"View on GitHub\" style=\"position:fixed;top:16px;right:20px;z-index:100;color:#24292f;line-height:0\"><svg width=\"32\" height=\"32\" viewBox=\"0 0 16 16\" fill=\"currentColor\" aria-hidden=\"true\"><path d=\"M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.013 8.013 0 0016 8c0-4.42-3.58-8-8-8z\"/></svg></a>"
  sed -i "s|<body>|<body>${GITHUB_LINK}|" hdoc-output/*.html
fi

# Add a sidebar sub-entry for every "## " section of the pages below.
SUBTAB_PAGES="GettingStarted CommandLine Simulation"
python3 - hdoc-output ${SUBTAB_PAGES} <<'PY'
import glob, html, re, sys

out_dir, pages = sys.argv[1], sys.argv[2:]

def slug(text):
    return re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")

menus = {}
for page in pages:
    path = f"{out_dir}/doc{page}.html"
    with open(path) as f:
        doc = f.read()
    sections = []

    def add_id(m):
        title = html.unescape(re.sub(r"<[^>]+>", "", m.group(1))).strip()
        anchor = slug(title)
        sections.append((anchor, title))
        return f'<h2 id="{anchor}">{m.group(1)}</h2>'

    doc = re.sub(r"<h2>(.*?)</h2>", add_id, doc, flags=re.S)
    with open(path, "w") as f:
        f.write(doc)
    menus[page] = "<ul>" + "".join(
        f'<li><a href="doc{page}.html#{a}">{html.escape(t)}</a></li>' for a, t in sections) + "</ul>"

for path in glob.glob(f"{out_dir}/*.html"):
    with open(path) as f:
        doc = f.read()
    for page, menu in menus.items():
        entry = f'<a href="doc{page}.html">{page}</a>'
        doc = doc.replace(f"<li>{entry}</li>", f"<li>{entry}{menu}</li>", 1)
    with open(path, "w") as f:
        f.write(doc)
PY

# hdoc renders docs/*.md but does not copy the images they reference.
if [ -d docs/images ]; then
  mkdir -p hdoc-output/images
  cp -r docs/images/. hdoc-output/images/
fi
