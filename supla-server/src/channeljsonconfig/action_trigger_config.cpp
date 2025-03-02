/*
 Copyright (C) AC SOFTWARE SP. Z O.O.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "channeljsonconfig/action_trigger_config.h"

#include "proto.h"
#include "tools.h"

using std::function;

// static
const _atc_map_t action_trigger_config::map[] = {
    {.cap = SUPLA_ACTION_CAP_TURN_ON, .str = "TURN_ON"},
    {.cap = SUPLA_ACTION_CAP_TURN_OFF, .str = "TURN_OFF"},
    {.cap = SUPLA_ACTION_CAP_TOGGLE_x1, .str = "TOGGLE_X1"},
    {.cap = SUPLA_ACTION_CAP_TOGGLE_x2, .str = "TOGGLE_X2"},
    {.cap = SUPLA_ACTION_CAP_TOGGLE_x3, .str = "TOGGLE_X3"},
    {.cap = SUPLA_ACTION_CAP_TOGGLE_x4, .str = "TOGGLE_X4"},
    {.cap = SUPLA_ACTION_CAP_TOGGLE_x5, .str = "TOGGLE_X5"},
    {.cap = SUPLA_ACTION_CAP_HOLD, .str = "HOLD"},
    {.cap = SUPLA_ACTION_CAP_SHORT_PRESS_x1, .str = "SHORT_PRESS_X1"},
    {.cap = SUPLA_ACTION_CAP_SHORT_PRESS_x2, .str = "SHORT_PRESS_X2"},
    {.cap = SUPLA_ACTION_CAP_SHORT_PRESS_x3, .str = "SHORT_PRESS_X3"},
    {.cap = SUPLA_ACTION_CAP_SHORT_PRESS_x4, .str = "SHORT_PRESS_X4"},
    {.cap = SUPLA_ACTION_CAP_SHORT_PRESS_x5, .str = "SHORT_PRESS_X5"}};

// static
const char action_trigger_config::caps_key[] = "actionTriggerCapabilities";

// static
const char action_trigger_config::disables_local_operation_key[] =
    "disablesLocalOperation";

// static
const char action_trigger_config::action_key[] = "action";

// static
const char action_trigger_config::actions_key[] = "actions";

// static
const char action_trigger_config::param_key[] = "param";

action_trigger_config::action_trigger_config(void)
    : abstract_action_config(), channel_json_config() {
  active_cap = 0;
  channel_id_if_subject_not_set = 0;
}

action_trigger_config::action_trigger_config(channel_json_config *root)
    : abstract_action_config(), channel_json_config(root) {
  active_cap = 0;
  channel_id_if_subject_not_set = 0;
}

action_trigger_config::~action_trigger_config(void) {}

int action_trigger_config::get_map_size(void) {
  return sizeof(map) / sizeof(_atc_map_t);
}

int action_trigger_config::get_map_key(int index) { return map[index].cap; }

const char *action_trigger_config::get_map_str(int index) {
  return map[index].str.c_str();
}

int action_trigger_config::get_action_id(int cap) {
  cJSON *cap_cfg = get_cap_user_config(cap);

  if (!cap_cfg) {
    return 0;
  }

  cJSON *action = cJSON_GetObjectItem(cap_cfg, action_key);

  if (action) {
    cJSON *id = cJSON_GetObjectItem(action, "id");
    if (id && cJSON_IsNumber(id)) {
      return id->valueint;
    }
  }

  return 0;
}

int action_trigger_config::get_action_id(void) {
  if (!active_cap) {
    return 0;
  }

  return get_action_id(active_cap);
}

_subjectType_e action_trigger_config::get_subject_type(int cap) {
  cJSON *cap_cfg = get_cap_user_config(cap);

  if (!cap_cfg) {
    return stUnknown;
  }

  cJSON *subjectType = cJSON_GetObjectItem(cap_cfg, "subjectType");
  if (equal(subjectType, "channelGroup")) {
    return stChannelGroup;
  } else if (equal(subjectType, "channel")) {
    return stChannel;
  } else if (equal(subjectType, "scene")) {
    return stScene;
  } else if (channel_id_if_subject_not_set) {
    cJSON *action = cJSON_GetObjectItem(cap_cfg, action_key);
    if (action) {
      cJSON *id = cJSON_GetObjectItem(action, "id");
      if (id && cJSON_IsNumber(id) && id->valueint == ACTION_FORWARD_OUTSIDE) {
        return stChannel;
      }
    }
  }
  return stUnknown;
}

_subjectType_e action_trigger_config::get_subject_type(void) {
  if (!active_cap) {
    return stUnknown;
  }

  return get_subject_type(active_cap);
}

int action_trigger_config::get_subject_id(int cap) {
  cJSON *cap_cfg = get_cap_user_config(cap);

  if (!cap_cfg) {
    return 0;
  }

  cJSON *id = cJSON_GetObjectItem(cap_cfg, "subjectId");
  if (id && cJSON_IsNumber(id) && id->valueint) {
    return id->valueint;
  } else if (channel_id_if_subject_not_set) {
    cJSON *action = cJSON_GetObjectItem(cap_cfg, action_key);
    if (action) {
      id = cJSON_GetObjectItem(action, "id");
      if (id && cJSON_IsNumber(id) && id->valueint == ACTION_FORWARD_OUTSIDE) {
        return channel_id_if_subject_not_set;
      }
    }
  }

  return 0;
}

int action_trigger_config::get_subject_id(void) {
  if (!active_cap) {
    return 0;
  }

  return get_subject_id(active_cap);
}

void action_trigger_config::set_channel_id_if_subject_not_set(int subject_id) {
  channel_id_if_subject_not_set = subject_id;
}

int action_trigger_config::get_source_id(const char *key) {
  if (!active_cap) {
    return 0;
  }

  cJSON *cap_cfg = get_cap_user_config(active_cap);

  if (!cap_cfg) {
    return 0;
  }

  cJSON *action = cJSON_GetObjectItem(cap_cfg, action_key);

  if (action) {
    cJSON *param = cJSON_GetObjectItem(action, param_key);

    if (param) {
      cJSON *id = cJSON_GetObjectItem(param, key);
      if (id && cJSON_IsNumber(id)) {
        return id->valueint;
      }
    }
  }

  return 0;
}

int action_trigger_config::get_source_device_id(void) {
  return get_source_id("sourceDeviceId");
}

int action_trigger_config::get_source_channel_id(void) {
  return get_source_id("sourceChannelId");
}

unsigned int action_trigger_config::get_capabilities(const char *key) {
  unsigned int result = 0;
  cJSON *json = get_properties_root();
  if (!json) {
    return result;
  }

  cJSON *st = cJSON_GetObjectItem(json, key);
  if (!st || !cJSON_IsArray(st)) {
    return result;
  }

  int st_size = cJSON_GetArraySize(st);

  for (short a = 0; a < st_size; a++) {
    result |= json_to_key(cJSON_GetArrayItem(st, a));
  }

  return result;
}

bool action_trigger_config::set_capabilities(const char *key,
                                             function<unsigned int()> get_caps,
                                             unsigned int caps) {
  cJSON *json = get_properties_root();
  if (!json) {
    return false;
  }

  int map_size = sizeof(map) / sizeof(_atc_map_t);

  unsigned int all_possible = 0;

  for (int a = 0; a < map_size; a++) {
    all_possible |= map[a].cap;
  }

  caps = all_possible & caps;

  if (caps == get_caps()) {
    return false;
  }

  cJSON *st = cJSON_GetObjectItem(json, key);
  if (st) {
    while (cJSON_GetArraySize(st)) {
      cJSON_DeleteItemFromArray(st, 0);
    }
  }

  if (!st) {
    st = cJSON_CreateArray();
    if (st) {
      cJSON_AddItemToObject(json, key, st);
    }
  }

  if (st) {
    for (int a = 0; a < map_size; a++) {
      if (caps & map[a].cap) {
        cJSON *jstr = cJSON_CreateString(map[a].str.c_str());
        if (jstr) {
          cJSON_AddItemToArray(st, jstr);
        }
      }
    }
  }

  return true;
}

unsigned int action_trigger_config::get_capabilities(void) {
  return get_capabilities(caps_key);
}

bool action_trigger_config::set_capabilities(unsigned int caps) {
  return set_capabilities(
      caps_key, [this]() -> unsigned int { return get_capabilities(); }, caps);
}

unsigned int action_trigger_config::get_caps_that_disables_local_operation(
    void) {
  return get_capabilities(disables_local_operation_key);
}

bool action_trigger_config::set_caps_that_disables_local_operation(
    unsigned int caps) {
  return set_capabilities(
      disables_local_operation_key,
      [this]() -> unsigned int {
        return get_caps_that_disables_local_operation();
      },
      caps);
}

unsigned int action_trigger_config::get_active_actions(void) {
  unsigned int result = 0;

  cJSON *json = get_user_root();
  if (!json) {
    return result;
  }

  unsigned int caps = get_capabilities();
  if (caps == 0) {
    return result;
  }

  cJSON *actions = cJSON_GetObjectItem(json, actions_key);

  int size = sizeof(map) / sizeof(_atc_map_t);

  for (int a = 0; a < size; a++) {
    if (cJSON_GetObjectItem(actions, map[a].str.c_str())) {
      result |= map[a].cap;
    }
  }

  return caps & result;
}

cJSON *action_trigger_config::get_cap_user_config(int cap) {
  if (!(get_capabilities() & cap)) {
    return NULL;
  }

  cJSON *root = get_user_root();
  if (!root) {
    return NULL;
  }

  cJSON *actions = cJSON_GetObjectItem(root, actions_key);

  if (!actions) {
    return NULL;
  }

  const char *cap_str = string_with_key(cap);

  if (!cap_str) {
    return NULL;
  }

  return cJSON_GetObjectItem(actions, cap_str);
}

char action_trigger_config::get_percentage(void) {
  char result = -1;

  if (!active_cap) {
    return result;
  }

  cJSON *cap_cfg = get_cap_user_config(active_cap);

  if (!cap_cfg) {
    return result;
  }

  cJSON *action = cJSON_GetObjectItem(cap_cfg, action_key);

  if (action) {
    cJSON *param = cJSON_GetObjectItem(action, "param");
    if (param) {
      cJSON *percentage = cJSON_GetObjectItem(param, "percentage");
      if (percentage && cJSON_IsNumber(percentage) &&
          percentage->valueint >= 0 && percentage->valueint <= 100) {
        result = percentage->valueint;
      }
    }
  }

  return result;
}

TAction_RGBW_Parameters action_trigger_config::get_rgbw(void) {
  TAction_RGBW_Parameters result = {.Brightness = -1,
                                    .ColorBrightness = -1,
                                    .Color = 0,
                                    .ColorRandom = false,
                                    .OnOff = false};
  if (!active_cap) {
    return result;
  }

  cJSON *cap_cfg = get_cap_user_config(active_cap);

  if (!cap_cfg) {
    return result;
  }

  cJSON *action = cJSON_GetObjectItem(cap_cfg, action_key);

  if (action) {
    cJSON *param = cJSON_GetObjectItem(action, "param");
    if (param) {
      cJSON *item = cJSON_GetObjectItem(param, "brightness");
      if (item && cJSON_IsNumber(item) && item->valueint >= 0 &&
          item->valueint <= 100) {
        result.Brightness = item->valueint;
      }

      item = cJSON_GetObjectItem(param, "color_brightness");
      if (item && cJSON_IsNumber(item) && item->valueint >= 0 &&
          item->valueint <= 100) {
        result.ColorBrightness = item->valueint;
      }

      item = cJSON_GetObjectItem(param, "hue");
      if (item) {
        if (cJSON_IsNumber(item)) {
          result.Color = st_hue2rgb(item->valuedouble);
        } else if (equal(item, "white")) {
          result.Color = 0xFFFFFF;
        } else if (equal(item, "random")) {
          while (!result.Color) {
            struct timeval time = {};
            gettimeofday(&time, NULL);
            unsigned int seed = time.tv_sec + time.tv_usec;
            result.Color = st_hue2rgb(rand_r(&seed) % 360);
          }

          result.ColorRandom = true;
        }
      }
    }
  }

  return result;
}

int action_trigger_config::get_cap(void) { return active_cap; }

bool action_trigger_config::channel_exists(int channel_id) {
  unsigned int caps = get_capabilities();
  short size = sizeof(caps) * 8;
  for (short n = 0; n < size; n++) {
    if (caps & (1 << n)) {
      if (get_action_id(1 << n) && get_subject_type(1 << n) == stChannel &&
          get_subject_id(1 << n) == channel_id) {
        return true;
      }
    }
  }

  return false;
}

void action_trigger_config::set_active_cap(int cap) { active_cap = cap; }
