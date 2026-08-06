# mc_robot_interface

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

## Development environment

A devcontainer is provided (`.devcontainer/`) for a clean, reproducible build environment: Ubuntu 24.04, `mc_rtc` and its dependency chain built via [mc-rtc-superbuild](https://github.com/mc-rtc/mc-rtc-superbuild) (ROS support disabled), plus this project's own extra dependencies (`zenoh-c`/`zenoh-cpp`, FlatBuffers, Protobuf, GTest).

On first start, `postCreateCommand` builds `mc_rtc` + dependencies (this can take a while the first time; a persistent Docker volume is used so this only happens once) and then configures/builds `mc_rtc_interface` itself.

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
  ssh mc_rtc_interface.devpod
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
