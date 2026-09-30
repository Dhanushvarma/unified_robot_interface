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

# LLVM 14 headers don't compile against libstdc++ >= 12 in C++20: use GCC <= 11's
# libstdc++ (e.g. `sudo apt-get install libstdc++-11-dev` on Ubuntu 24.04).
try_gcc_toolchain_workaround() {
  local current_gcc older
  current_gcc="$(clang-14 -v 2>&1 | grep -oP 'Selected GCC installation: \K.*' || true)"
  older="$(ls -d /usr/lib/gcc/x86_64-linux-gnu/*/ 2>/dev/null | sed 's:/$::' \
    | awk -F/ '$NF + 0 <= 11' | sort -V | tail -1)"
  if [ -z "${older}" ] || [ "${older}" = "${current_gcc}" ]; then
    echo "==> No GCC <= 11 toolchain found for the LLVM-14/libstdc++ workaround" >&2
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

  if try_gcc_toolchain_workaround; then
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

# Anchor every "## " section of the hand-written pages, add a sidebar sub-entry
# for each section of the pages below, and add every section to the search
# index (hdoc only indexes code symbols).
SUBTAB_PAGES="GettingStarted CommandLine Simulation"
python3 - hdoc-output ${SUBTAB_PAGES} <<'PY'
import glob, html, json, os, re, sys

out_dir, subtab_pages = sys.argv[1], sys.argv[2:]
PAGE_TYPE = 7  # search index "type" of a documentation section (hdoc uses 0-6)

def slug(text):
    return re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")

def to_text(fragment):
    return " ".join(html.unescape(re.sub(r"<[^>]+>", " ", fragment)).split())

menus = {}
entries = []
for path in [f"{out_dir}/index.html"] + sorted(glob.glob(f"{out_dir}/doc*.html")):
    name = os.path.basename(path)
    with open(path) as f:
        doc = f.read()
    sections = []

    def add_id(m):
        title = to_text(m.group(1))
        anchor = base = slug(title)
        n = 1
        while any(a == anchor for a, _ in sections):
            n += 1
            anchor = f"{base}-{n}"
        sections.append((anchor, title))
        return f'<h2 id="{anchor}">{m.group(1)}</h2>'

    doc = re.sub(r'<h2(?: id="[^"]*")?>(.*?)</h2>', add_id, doc, flags=re.S)
    with open(path, "w") as f:
        f.write(doc)

    page = name[len("doc"):-len(".html")] if name.startswith("doc") else None
    if page in subtab_pages:
        menus[page] = "<ul>" + "".join(
            f'<li><a href="{name}#{a}">{html.escape(t)}</a></li>' for a, t in sections) + "</ul>"

    # One search entry for the page intro, then one per "## " section.
    main = re.search(r'<main class="content">(.*?)</main>', doc, flags=re.S)
    if not main:
        continue
    h1 = re.search(r"<h1[^>]*>(.*?)</h1>", main.group(1), flags=re.S)
    page_title = to_text(h1.group(1)) if h1 else name
    parts = re.split(r'<h2 id="([^"]*)">(.*?)</h2>', main.group(1), flags=re.S)
    entries.append({"sid": name, "name": page_title, "decl": page_title,
                    "text": to_text(parts[0]), "type": PAGE_TYPE})
    for anchor, title, body in zip(parts[1::3], parts[2::3], parts[3::3]):
        title = to_text(title)
        entries.append({"sid": f"{name}#{anchor}", "name": title,
                        "decl": f"{page_title} › {title}", "text": to_text(body),
                        "type": PAGE_TYPE})

index_path = f"{out_dir}/index.json"
with open(index_path) as f:
    index = [e for e in json.load(f) if e.get("type") != PAGE_TYPE]
with open(index_path, "w") as f:
    json.dump(index + entries, f, separators=(",", ":"))

# Make hdoc's search also match the section text and link "page" results.
def patch(path, old, new):
    with open(path) as f:
        js = f.read()
    if new not in js:
        if old not in js:
            sys.exit(f"error: cannot patch {path}: '{old}' not found (hdoc version changed?)")
        js = js.replace(old, new, 1)
    with open(path, "w") as f:
        f.write(js)

for js in ("worker.js", "search.js"):
    patch(f"{out_dir}/{js}", "fields: ['name', 'decl'],", "fields: ['name', 'decl', 'text'],")
patch(f"{out_dir}/search.js", "    case 6:\n        return \"enum val\";",
      "    case 6:\n        return \"enum val\";\n    case 7:\n        return \"page\";")
patch(f"{out_dir}/search.js", "        // Enum or enum val\n",
      "        // Documentation page section\n"
      "        if (obj.type === 7) {\n"
      "            a.setAttribute(\"href\", obj.id);\n"
      "        }\n"
      "        // Enum or enum val\n")

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
