# An NVidia AI example

## Description

A simple Deepstream C++ pipeline for video recognition.

I used a Docker image provided in a scope of Nvidia AI course. The pipeline decodes a video stream downloaded from Youtube. It detects 3 classes of objects - people, cars and bikes, and draws color rectangles over them using colors, which are defined for every class.

The rectangle drawer is implemented customly as a Gstream probe. It is called on each frame right after the inference. The probe callback function has access to a buffer which contains both RGB frame and inference metadata with the rectangles and object classes. The CPU is used for rectangle drawing. For large and numerous rectangles it may drop frames. It is possible to use a custom GPU kernel for this, though.

The output is shown on a compact display attached to the Jetson Nano board.

## Prerequisites

### Setting up Jetson Board

From https://developer.nvidia.com/embedded/downloads download "Jetson Nano Developer Kit SD Card Image 4.6.1" and flash it to SD card.

### Running Docker image

Open Terminal on Jetson Nano and run following commands.

`echo "sudo docker run --runtime nvidia -it --rm --network host -v /tmp/.X11-unix/:/tmp/.X11-unix -v /tmp/argus_socket:/tmp/argus_socket -v ~/my_apps:/dli/task/my_apps --device /dev/video0 nvcr.io/nvidia/dli/dli-nano-deepstream:v2.0.0-DS6.0.1 " > ds_docker_run.sh`
`chmod +x ds_docker_run.sh`
`./ds_docker_run.sh`

### Installing additional dependencies

In the terminal window run:

`apt update`
`apt install -y libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev`

## Compilation

Download the code:

`git clone https://github.com/tslabs/nvidia-ai.git`
`cd nvidia-ai`
`make`

## Run

`./poc`