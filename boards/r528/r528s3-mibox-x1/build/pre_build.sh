#!/bin/bash
set -x
./nuttx/tools/buildinfo.sh >./vendor/allwinnertech/boards/r528/r528s3-mibox-x1/src/etc/build.prop
