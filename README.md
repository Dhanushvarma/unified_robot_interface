# Unified Robot Interface - URI

## Goals

The goal here with this interface is to standardise robot interface creation.

Goal is to create a RobotFactory in charge of creating all the robot and their connection to the network or locally.

Ideas :
* Give users the possibily to easily deploy new robots with mc_rtc.
* Communicate with the robot using existing protocols (tcp / udp / zenoh / ethercat / ...)
* Use MessagePack to compress data
* Define / create driver which means that users can create different interface quickly and experiment while keeping exisint one working
* Repect robot timestep and control command
* Consider the case where we may had robot in the loop dynamically

## Tasks
- [ ] Pseudo network to communicate between robot manager and robot interface
- [ ] Scale to multi-thread system
- [ ] Integrated with `mc_rtc` and sample robot interface
- [x] Parse and send information from config `yaml`

## Installation

URI currently installs next to [mc_rtc](https://jrl-umi3218.github.io/mc_rtc/),
its only controller backend so far; support for other controllers is planned.
The simplest way is [mc-rtc-superbuild](https://github.com/mc-rtc/mc-rtc-superbuild)
with `-DWITH_URI=ON`. See [`docs/GettingStarted.md`](docs/GettingStarted.md) for
all installation options.

## Development environment

A devcontainer is provided (`.devcontainer/`) for a clean, reproducible build environment: Ubuntu 24.04 with ROS Jazzy, `mc_rtc` installed from the mc-rtc `head` apt repository (with ROS support), plus this project's own extra dependencies (`zenoh-c`/`zenoh-cpp`, FlatBuffers, Protobuf, GTest).

On first start, `postCreateCommand` configures, builds and installs `unified_robot_interface` and the enabled drivers into `build/install`.

### Using VSCode

- Install [Docker](https://docs.docker.com/engine/install/ubuntu/) and the [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers).
- Open this repository's folder in VSCode.
- When prompted, choose "Reopen in Container" (or run the `Dev Containers: Reopen in Container` command manually).
- VSCode will build the image, start the container, and run `postCreateCommand` automatically.

See [Developing inside a Container](https://code.visualstudio.com/docs/devcontainers/containers) for more details.

### Using DevPod CLI

- Install [DevPod CLI](https://devpod.sh/docs/getting-started/install#install-devpod-cli).
- Add Docker as a provider (once): `devpod provider add docker`
- From the repository root:
  ```sh
  devpod up . --ide=none
  ```
- Connect over SSH:
  ```sh
  ssh unified_robot_interface.devpod
  ```
- Or use VSCode through DevPod: `devpod up . --ide=vscode`

If you use signed commits, forward your GPG agent by adding to `~/.devpod/config.yaml`:
```yaml
contexts:
  default:
    defaultProvider: docker
    options:
      GPG_AGENT_FORWARDING:
        userProvided: true
        value: "true"
```
## Supported robots

Existing drivers: UR (`rtde_driver`), Kinova (`kortex_driver`), xArm
(`xarm_driver`), TriOrb (`triorb_driver`), any ros2_control robot
(`ros2_control_driver`) and Mirokai (`miroki_driver`). See
[`docs/SupportedRobots.md`](docs/SupportedRobots.md).
Simulators are connected the same way, as drivers: see
[`docs/Simulation.md`](docs/Simulation.md).

## Creating a new robot driver

`robot_interface` drivers are standalone plugin projects implementing
`mc_robot_interface::RobotDriver` (see
[`robot_interface/include/robot_interface/RobotDriverTemplate.h`](robot_interface/include/robot_interface/RobotDriverTemplate.h)),
loaded at runtime as shared libraries (full guide: [`docs/NewRobotDriver.md`](docs/NewRobotDriver.md)).
Scaffold a new one with:

```bash
uri create_new_driver <DriverName> [folder]   # e.g. uri create_new_driver Franka ~/devel
```

This generates a `<name>_driver/` project (CMakeLists, header/source skeleton,
example config, README) ready to fill in and build against this repository's
install prefix. The generator script ([`robot_interface/tools/create_new_driver.sh`](robot_interface/tools/create_new_driver.sh))
is embedded in the `uri` binary at build time. Run `uri create_new_driver --help` for details.

## API documentation

[hdoc](https://hdoc.io) generates static HTML documentation from
`build/compile_commands.json` (API reference) and the hand-written pages in
[`docs/`](docs/) (context and architecture, communication, robot manager,
robot interface, running examples, supported robots, creating a new driver), configured via
[`.hdoc.toml`](.hdoc.toml).
This project uses the self-hosted, open-source hdoc (AGPLv3) -- no account or
external upload required. Generate it with:

```bash
cmake -S . -B build   # if not already configured, see Development environment above
scripts/gen-hdoc-docs.sh
```

The first run builds and caches the `hdoc` binary itself (under
`~/.cache/hdoc-build` by default; override with `$HDOC_CACHE_DIR`), which
takes a few minutes; later runs reuse it. Output goes to `hdoc-output/`
(gitignored) -- open `hdoc-output/index.html` in a browser.

hdoc pins LLVM/Clang 14; install it with
`sudo apt-get install llvm-14-dev libclang-14-dev clang-14` if the build
script reports it's missing.
