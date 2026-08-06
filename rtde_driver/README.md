# rtde_driver

A `robot_interface` driver plugin for Universal Robots controllers via the
[Universal Robots Client Library](https://github.com/UniversalRobots/Universal_Robots_Client_Library).

It implements the `mc_robot_interface::RobotDriver` interface so it can be loaded
by `robot_interface` at runtime as a shared library — no mc_rtc dependency required.

## Dependencies

| Dependency | Where to get it |
|---|---|
| `mc_robot_interface` | Built from this repository |
| `ur_client_library` | ROS package **or** built from source (see below) |

### Installing ur_client_library

**Option A — ROS package** (Humble / Iron / Jazzy):

```bash
sudo apt install ros-<distro>-ur-client-library
```

**Option B — from source** (no ROS required):

```bash
git clone https://github.com/UniversalRobots/Universal_Robots_Client_Library.git
cd Universal_Robots_Client_Library
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build .
sudo cmake --install .
```

`ur_client_library` must be discoverable by CMake.  Either source a ROS workspace,
install it system-wide, or pass the prefix explicitly:

```bash
# ROS install
cmake -DCMAKE_PREFIX_PATH="/path/to/mc_rtc_interface/install;/opt/ros/humble" ..

# Source / system install
cmake -DCMAKE_PREFIX_PATH="/path/to/mc_rtc_interface/install;/usr/local" ..
```

## Build

```bash
cd rtde_driver
mkdir build && cd build
cmake .. \
  -DCMAKE_PREFIX_PATH="/path/to/install;/opt/ros/humble" \
  -DCMAKE_INSTALL_PREFIX=/path/to/install
cmake --build .
cmake --install .
```

The plugin is installed to `lib/robot_driver/libRobotDriverRTDE.so`.

## Robot-side setup

`rtde_driver` runs in **headless mode**: it sends the URScript program directly to
the robot over the primary interface, so no URCap installation is needed.

The URScript file (`external_control.urscript`) ships with `ur_client_library` and
is found automatically.  Override the path with the `UR_SCRIPT_FILE` environment
variable if needed:

```bash
export UR_SCRIPT_FILE=/path/to/external_control.urscript
```

Search order:
1. `$UR_SCRIPT_FILE` environment variable
2. `/opt/ros/<humble|iron|jazzy>/share/ur_client_library/resources/external_control.urscript`
3. `/usr/share/ur_client_library/resources/external_control.urscript`

## RTDE output fields

The driver requests the following RTDE fields from the robot:

| Field | Used by |
|---|---|
| `actual_q` | `getActualQ()` — joint positions \[rad\] |
| `actual_qd` | `getActualQd()` — joint velocities \[rad/s\] |
| `target_moment` | `getJointTorques()` — target joint torques \[Nm\] |

## Control loop

`sync()` blocks until the robot delivers a new RTDE data package, pacing the
control loop at the robot's RTDE frequency (500 Hz by default).

```
loop:
  sync()          ← blocks, receives data package
  getActualQ()    ← reads from the cached package
  getActualQd()
  getJointTorques()
  servoJ(q)       ← sends position command (MODE_SERVOJ)
  speedJ(qd)      ← sends velocity command (MODE_SPEEDJ)
```

## Plugin API

The shared library exports three C symbols consumed by `robot_interface`'s plugin loader:

```cpp
void MC_RTC_ROBOT_DRIVER(std::vector<std::string> & classes);  // registers "RobotDriverRTDE"
mc_robot_interface::RobotDriver * create(const std::string & name,
                                         const std::string & ip,
                                         const uint16_t & port);
void destroy(mc_robot_interface::RobotDriver * ptr);
```

Configure it in `robot_manager/etc/mc_rtc.yaml` under the `robot_interface` key of the
relevant robot entry:

```yaml
Robots:
  ur5e:
    module: ur5e_module
    robot_interface:
      driver: RobotDriverRTDE
      ip: 192.168.1.100
      # port: 0          # optional, unused (RTDE uses a fixed port)
```
