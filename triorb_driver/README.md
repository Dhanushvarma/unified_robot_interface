# triorb_driver

`triorb_driver` provides an `mc_robot_interface::RobotDriver` plugin for controlling a TriOrb omnidirectional mobile base through USB serial communication.

## Features

- Serial communication through `/dev/ttyACM0`
- Planar pose feedback: `[x, y, theta]`
- Planar velocity commands: `[vx, vy, wz]`
- Position control implemented through velocity commands
- Odometry-origin reset during startup
- Watchdog and zero-velocity shutdown

# Installation

Install `mc_rtc_interface`

```sh
git clone https://github.com/ThomasDuvinage/mc_rtc_interface.git
cd mc_rtc_interface
cmake -S . -B build
cmake --build --parallel $(nproc)
cmake --install build
```

Install `triorb_driver`
```sh
cd triorb_driver
cmake -S . -B build
cmake --build --parallel $(nproc)
cmake --install build
```

# Usage

Delare triorb and all specification in the yaml configuration file. You can use the provided [example](../robot_manager/etc/mc_rtc_triorb.yaml) for reference.

```yaml
Robots:
  triorb:
    module: triorb
    controller:
      mode: position
      time_step: 0.001
    network_interface:
      protocol: zenoh
    robot_interface:
      driver: RobotDriverTriOrb
      ip: /dev/ttyACM0
      port: 0

      autostart: true
```

Run
```sh
cd <mc_rtc_interface>
MCFleetControl -f robot_manager/etc/mc_rtc_triorb.yaml
```

Note

The driver uses the TriOrb MOVING_SPEED_RELATIVE command. In velocity mode, [vx, vy, wz] is sent directly to the robot. In position mode, the requested [x, y, theta] target is compared with measured odometry. The driver converts the pose error into body-relative velocity commands and sends them through the same velocity interface. The driver does not use TriOrb native position commands.
