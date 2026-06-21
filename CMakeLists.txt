# SPDX-License-Identifier: MIT

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_TOTEM_LAYER_TELEMETRY src/totem_layer_telemetry.c)

# ZMK application headers (zmk/event_manager.h, zmk/keymap.h, zmk/events/*) live
# in zmk/app/include, which is not on the default include path for an
# out-of-tree module. The `zmk` west project sits next to this module under the
# workspace topdir, so add its app include directory for this library.
zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../zmk/app/include)
