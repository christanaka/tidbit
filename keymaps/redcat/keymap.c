/* Copyright 2021 Chris Tanaka <https://github.com/christanaka>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "bootanim.h"
#include "pet.h"
#include "status.h"
#include "clock.h"
#include "14seg_animation.h"
#include QMK_KEYBOARD_H

#define _BASE     0
#define _LAYER1     1
#define _LAYER2     2
#define _LAYER3     3

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [_BASE] = LAYOUT(
    KC_NO, KC_NO, KC_NO,
  KC_MUTE, KC_ESC, KC_ENT, KC_NLCK,
  TG(_LAYER1), KC_KP_7,     KC_KP_8, KC_KP_9,
  TG(_LAYER2), KC_KP_4,     KC_KP_5, KC_KP_6,
  TG(_LAYER3), KC_KP_1, KC_KP_2, KC_KP_3
  ),

  [_LAYER1] = LAYOUT(
           KC_NO, KC_NO, KC_NO,
  KC_TRNS, KC_F10, KC_F11, KC_F12,
  KC_TRNS, KC_F7, KC_F8, KC_F9,
  KC_TRNS, KC_F4, KC_F5, KC_F6,
  KC_TRNS, KC_F1, KC_F2, KC_F3
  ),

  [_LAYER2] = LAYOUT(
           KC_NO, KC_NO, KC_NO,
  KC_TRNS, KC_BSPC, KC_SPC, KC_CLCK,
  KC_TRNS, KC_LCTRL, KC_LSHIFT, KC_LALT,
  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
  ),

  [_LAYER3] = LAYOUT(
           KC_NO, KC_NO, KC_NO,
  KC_TRNS, RESET, KC_TRNS, KC_TRNS,
  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
  ),
};

// Quad alphanumeric display configuration
#define DISP_ADDR 0x70
HT16K33 *disp;
animation_t *animation;
char message[16];

// HID configuration
#define HID_BYTE_OFFSET 2

oled_rotation_t oled_init_user(oled_rotation_t rotation) { return OLED_ROTATION_270; }

void oled_task_user(void) {
  if (!is_bootanim_done) {
    bootanim_render(0, 1);
  } else {
    status_render_wpm(0, 0);
    status_render_layer(0, 3);
    status_render_caps_lock(0, 7);

    pet_render(0, 13);
  }
}

void matrix_init_user(void) {
  matrix_init_remote_kb();

  // Initialize quad display
  disp = newHT16K33(4, DISP_ADDR);

  animation = newAnimation(disp);
  animation->message = message;
  animation->mode = DISP_MODE_BOUNCE;
  animation->enabled = true;

  // Clear quad display
  char tmp[] = BLANK;
  strcpy(message, tmp);
  force_animation_refresh(animation);
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
  process_record_remote_kb(keycode, record);
  pet_process_record(keycode, record);

  return true;
}

bool encoder_update_user(uint8_t index, bool clockwise) {
  if (clockwise) {
    tap_code(KC_VOLU);
  } else {
    tap_code(KC_VOLD);
  }
  return true;
}

bool wpm_keycode_user(uint16_t keycode) {
  if ((keycode >= QK_MOD_TAP && keycode <= QK_MOD_TAP_MAX) ||
      (keycode >= QK_LAYER_TAP && keycode <= QK_LAYER_TAP_MAX) ||
      (keycode >= QK_MODS && keycode <= QK_MODS_MAX)) {
    keycode = keycode & 0xFF;
  } else if (keycode > 0xFF) {
    keycode = 0;
  }

  // Include keys in WPM calculation
  if ((keycode >= KC_A && keycode <= KC_0) || // Alphas - Numbers
      (keycode >= KC_TAB && keycode <= KC_SLASH) || // Tab - Slash (Symbols, Punctuation, Space)
      (keycode >= KC_KP_1 && keycode <= KC_KP_DOT) ||  // Keypad numbers - Keypad Dot
      (keycode >= KC_F1 && keycode <= KC_F12) || // F1 - F12
      (keycode >= TG(_LAYER1) && keycode <= TG(_LAYER3)) // Layer Toggles
    ) {
    return true;
  }

  return false;
}

void raw_hid_receive(uint8_t *data, uint8_t length) {
  char *clock_data = (char *)data + HID_BYTE_OFFSET;

  update_clock_host(clock_data);

  char tmp[6];
  strftime(tmp, 16, "%H%M", &clock_time);
  strcpy(message, tmp);
  force_animation_refresh(animation);
}
