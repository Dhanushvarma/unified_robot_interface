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
