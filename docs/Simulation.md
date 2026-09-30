# Simulation

For URI, a simulator is just another robot. `uri manager` and
`uri interface` cannot tell a simulated robot from a real one: both are a
`RobotDriver` plugin that reports a state and accepts commands. Switching
between simulation and hardware means changing the `driver` of a robot entry
in `mc_rtc.yaml`, nothing else.

There are three ways to simulate robots:

- **Through ros2_control**, available today: run the simulator behind a
  ros2_control `controller_manager` (`mujoco_ros2_control`,
  `gz_ros2_control`, ...) and use `ros2_control_driver`. See
  [Supported robots](docSupportedRobots.html).
- **With a simulation driver** that embeds the simulator itself. This page
  explains how to write one, taking a MuJoCo driver as the example. No such
  driver exists yet.
- **With a simulator that speaks `robot_comm`** directly, for several robots
  in one world. See [Several robots in one scene](#several-robots-in-one-scene).

## A MuJoCo driver

Scaffold it like any other driver (see
[Create a new robot driver](docNewRobotDriver.html)):

```bash
uri create_new_driver MuJoCo ~/devel     # -> mujoco_driver, class RobotDriverMuJoCo
```

Then implement each `RobotDriver` method against the simulation:

| Method | In a simulation driver |
|---|---|
| constructor | Load the model (e.g. an MJCF file passed through `config_path`). Map each RobotModule joint to its simulator joint and actuator, by name. Set the initial joint positions. |
| `sync()` | Apply the latest command, advance the physics by one robot period, then wait until that period has elapsed in real time (see below). |
| `getActualQ()` / `getActualQd()` | Read the simulated joint positions and velocities, in the RobotModule's joint order. |
| `getJointTorques()` | Read the torques applied by the actuators. |
| `getIMUs()` / `getForceSensors()` | Read the simulated sensors, keyed by the RobotModule's sensor names. |
| `servoJ()` / `speedJ()` / `tauJ()` | Store the command. `sync()` applies it on the next step: directly for matching actuators, or through a PD loop to turn position targets into torques. |

The driver does not depend on mc_rtc: the RobotModule only provides the joint
and sensor names, which the simulation model must use too.

Configure the robot like a real one, with `autostart` so the simulation runs
on the control PC:

```yaml
Robots:
  ur5e:
    module: UR5e
    controller:
      mode: position
      time_step: 0.001
    robot_interface:
      driver: RobotDriverMuJoCo
      config_path: /path/to/ur5e_scene.xml
      autostart: true
    network_interface:
      protocol: zenoh/shm
```

`ip` and `port` are optional and can be ignored by the driver.

## Several robots in one scene

A simulator like mc_mujoco runs one world with several robots and objects.
A `RobotDriver` cannot do that, because a driver serves a single robot. The
simulator instead takes the place of `uri interface` for all of its robots:
it links `robot_comm` and opens **one communication interface per robot**.
For each simulated robot, it must:

1. Create a `robot_comm::Communication` for the robot's name
   (`CommunicationFactory::makeCommunication`) and call `setupClient()`.
2. Answer the init query on `{name}/init` with `"OK"`. The payload is the
   robot's configuration, including `controller.mode`.
3. Every robot period, `send()` a `State` and apply the latest `Command`
   from `receive()`.

```text
                        ur5e/init, ur5e/state, ur5e/command
uri manager (mc_rtc) <----------------------------------------> simulator
                        panda/init, panda/state, panda/command   (one world)
```

The manager sees no difference. Set `autostart: false` for these robots so
it doesn't start a `uri interface` for them, and start the simulator before
the manager. [Communication](docCommunication.html) describes the topics
and messages.

## Isaac Sim

Isaac Sim's ROS 2 bridge exchanges `sensor_msgs/JointState` in both
directions. `ros2_control_driver` can read its joint states directly (set
`joint_states_topic`), but publishes commands as `Float64MultiArray`, which
Isaac does not accept. Put a `controller_manager` in between, using the
[`topic_based_ros2_control`](https://github.com/PickNikRobotics/topic_based_ros2_control)
hardware plugin with a `joint_state_broadcaster` and a forward controller,
and use `ros2_control_driver` as for any ros2_control robot.

Set Isaac's physics and publish rate to the robot's `controller.time_step`,
and keep it running in real time (see below).

## Things to know

- **Run in real time.** `uri manager` ticks on the wall clock, not on the
  robots' states. `sync()` must therefore return once per
  `controller.time_step` of real time: step the physics, then sleep until the
  step's deadline. A simulation that runs faster or slower than real time
  gets out of step with the controller. You can run several physics
  substeps per `sync()` if the model needs a smaller time step.
- **One robot per driver.** Each robot runs its own `uri interface` and its
  own driver instance, so robots in separate simulation drivers are in
  separate worlds and cannot touch each other. For a shared scene, see
  [Several robots in one scene](#several-robots-in-one-scene).
- **No floating-base state yet.** A `State` carries joint, IMU and force
  sensor data only. The ground-truth pose of a floating base, which a
  simulator knows, is not sent to the controller. Mobile and legged robots
  must estimate it from the sensors, as on hardware.
- **Test it on its own first.** `uri viewer -c <config>` loads the driver and
  prints its joint state without the manager, as for any driver.
