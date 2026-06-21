/*
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>

LOG_MODULE_REGISTER(totem_layer_telemetry, CONFIG_TOTEM_LAYER_TELEMETRY_LOG_LEVEL);

static bool last_layer_valid;
static zmk_keymap_layer_index_t last_active_index;
static zmk_keymap_layer_id_t last_active_id;
static zmk_keymap_layers_state_t last_state;
static zmk_keymap_layers_state_t last_locks;

static void totem_layer_telemetry_emit(bool force) {
    zmk_keymap_layer_index_t active_index = zmk_keymap_highest_layer_active();
    zmk_keymap_layer_id_t active_id = zmk_keymap_layer_index_to_id(active_index);
    zmk_keymap_layers_state_t state = zmk_keymap_layer_state();
    zmk_keymap_layers_state_t locks = zmk_keymap_layer_locks();
    const char *name = zmk_keymap_layer_name(active_id);

    if (!force && last_layer_valid && active_index == last_active_index &&
        active_id == last_active_id && state == last_state && locks == last_locks) {
        return;
    }

    last_layer_valid = true;
    last_active_index = active_index;
    last_active_id = active_id;
    last_state = state;
    last_locks = locks;

    if (name == NULL || name[0] == '\0') {
        name = "unnamed";
    }

    printk("TOTEM_LAYER active=%u id=%u name=%s state=0x%08x locks=0x%08x\n",
           (unsigned int)active_index, (unsigned int)active_id, name,
           (uint32_t)state, (uint32_t)locks);
    LOG_INF("TOTEM_LAYER active=%u id=%u name=%s state=0x%08x locks=0x%08x",
            (unsigned int)active_index, (unsigned int)active_id, name,
            (uint32_t)state, (uint32_t)locks);
}

static void totem_layer_telemetry_boot_work_handler(struct k_work *work);
static void totem_layer_telemetry_poll_work_handler(struct k_work *work);

K_WORK_DELAYABLE_DEFINE(totem_layer_telemetry_boot_work,
                        totem_layer_telemetry_boot_work_handler);
K_WORK_DELAYABLE_DEFINE(totem_layer_telemetry_poll_work,
                        totem_layer_telemetry_poll_work_handler);

static void totem_layer_telemetry_boot_work_handler(struct k_work *work) {
    ARG_UNUSED(work);

    totem_layer_telemetry_emit(true);
}

static void totem_layer_telemetry_poll_work_handler(struct k_work *work) {
    ARG_UNUSED(work);

    totem_layer_telemetry_emit(false);
    k_work_schedule(&totem_layer_telemetry_poll_work,
                    K_MSEC(CONFIG_TOTEM_LAYER_TELEMETRY_POLL_INTERVAL_MS));
}

static int totem_layer_telemetry_listener(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);

    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    totem_layer_telemetry_emit(true);
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(totem_layer_telemetry, totem_layer_telemetry_listener);
ZMK_SUBSCRIPTION(totem_layer_telemetry, zmk_layer_state_changed);

static int totem_layer_telemetry_init(void) {
    k_work_schedule(&totem_layer_telemetry_boot_work,
                    K_MSEC(CONFIG_TOTEM_LAYER_TELEMETRY_BOOT_DELAY_MS));
    k_work_schedule(&totem_layer_telemetry_poll_work,
                    K_MSEC(CONFIG_TOTEM_LAYER_TELEMETRY_BOOT_DELAY_MS +
                           CONFIG_TOTEM_LAYER_TELEMETRY_POLL_INTERVAL_MS));

    return 0;
}

SYS_INIT(totem_layer_telemetry_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
