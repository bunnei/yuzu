#!/bin/bash -ex

docker run -v $(pwd):/yuzu ubuntu:18.04 /bin/bash -ex /yuzu/.travis/clang-format/docker.sh
