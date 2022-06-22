#!/bin/bash
set -e

# Build the image
echo "Building the image..."

if docker history -q ransomware >/dev/null 2>&1; then
    read -r -p "The image already exists. The project volume mounted can be older. Build it again? [y/N] " response
    case "${response}" in
        [yY]|[yY][eE][sS])
            docker build -t ransomware . ;;
    esac
else
    docker build -t ransomware .
fi


# Compile the binaries
echo "Compiling the binaries..."

if [ $# -eq 0 ]
  then
    echo "Please inform a command to execute inside the container"
    exit
fi

docker run --rm -v "$PWD/bin":/app/bin ransomware "$@"

# We need change the owner of the binaries generated
echo "Fix binaries permissions..."
sudo chown -R $USER:$USER ./bin/
