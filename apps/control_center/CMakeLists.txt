# ##############################################################################
# apps/examples/control_center/CMakeLists.txt
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

if(CONFIG_EXAMPLES_CONTROL_CENTER)
  nuttx_add_application(
    NAME
    control_center
    PRIORITY
    ${CONFIG_EXAMPLES_CONTROL_CENTER_PRIORITY}
    STACKSIZE
    ${CONFIG_EXAMPLES_CONTROL_CENTER_STACKSIZE}
    MODULE
    ${CONFIG_EXAMPLES_CONTROL_CENTER}
    SRCS
    control_center.c
    http.c
    ipc_udp.c
    uuid.c
    websocket_client.c
    leds.c)

  target_include_directories(
    apps PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                 ${NUTTX_APPS_DIR}/netutils/cjson/cJSON
                 ${NUTTX_APPS_DIR}/netutils/libwebsockets
                 ${NUTTX_APPS_DIR}/netutils/libwebsockets/libwebsockets/include
                 ${NUTTX_APPS_DIR}/crypto/openssl_mbedtls_wrapper/include
                 ${NUTTX_APPS_DIR}/../external/curl/curl/include)
endif()
