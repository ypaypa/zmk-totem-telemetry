# SPDX-License-Identifier: MIT

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_TOTEM_LAYER_TELEMETRY src/totem_layer_telemetry.c)
