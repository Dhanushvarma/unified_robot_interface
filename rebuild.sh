#!/bin/bash
set -e

source /opt/ros/humble/setup.bash

mkdir -p ~/workspace/sandbox/mc_robot_manager/robot_manager/build
cd ~/workspace/sandbox/mc_robot_manager/robot_manager/build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
sudo make install

mkdir -p ~/workspace/sandbox/mc_robot_manager/local_robot/build
cd ~/workspace/sandbox/mc_robot_manager/local_robot/build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
sudo make install

sudo ldconfig

exec "$@"
