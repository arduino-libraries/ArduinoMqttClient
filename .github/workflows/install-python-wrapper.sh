#!/bin/bash -x
# This script is used as an alternate entrypoint to allow installing Python 3 (a dependency of
# the ESP8266 core for Arduino) in the Docker container used by the compile-examples action

# Install Python 3
apt-get update && apt-get install -y python3

# Run the standard entrypoint script
/entrypoint.sh "$@"
