#!/bin/bash
set -e

source /opt/ros/humble/setup.bash

mkdir -p ~/workspace/sandbox/mc_robot_manager/robot_manager/build
cd ~/workspace/sandbox/mc_robot_manager/robot_manager/build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
make
sudo make install

mkdir -p ~/workspace/sandbox/mc_robot_manager/local_robot/build
cd ~/workspace/sandbox/mc_robot_manager/local_robot/build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
make
sudo make install

cd ~/workspace/sandbox/mc_robot_manager
if command -v jq >/dev/null 2>&1; then
    echo "Merging compilation databases..."
    jq -s 'add' robot_manager/build/compile_commands.json local_robot/build/compile_commands.json > compile_commands.json
else
    echo "Warning: jq not found. Compilation database not merged."
fi

sudo ldconfig

exec "$@"
