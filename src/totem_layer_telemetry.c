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
static uint8_t last_layer;

static void totem_layer_telemetry_emit(bool force) {
    /* Portable across ZMK versions: only use the stable highest-layer API.
     * For a linear keymap (no layer reordering) the index equals the id, and
     * the host app uses this number to pick the layer image/bounds.
     * state/locks are not used by the host app, so emit 0. */
    uint8_t layer = (uint8_t)zmk_keymap_highest_layer_active();
    const char *name = zmk_keymap_layer_name(layer);

    if (!force && last_layer_valid && layer == last_layer) {
        return;
    }

    last_layer_valid = true;
    last_layer = layer;

    if (name == NULL || name[0] == '\0') {
        name = "unnamed";
    }

    printk("TOTEM_LAYER active=%u id=%u name=%s state=0x%08x locks=0x%08x\n",
           (unsigned int)layer, (unsigned int)layer, name, (uint32_t)0, (uint32_t)0);
    LOG_INF("TOTEM_LAYER active=%u id=%u name=%s state=0x%08x locks=0x%08x",
            (unsigned int)layer, (unsigned int)layer, name, (uint32_t)0, (uint32_t)0);
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
