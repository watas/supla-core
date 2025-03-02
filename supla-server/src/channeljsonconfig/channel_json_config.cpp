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

#include "channel_json_config.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

#define MAX_STR_LEN 100

channel_json_config::channel_json_config(void) {
  this->properties_root = NULL;
  this->user_root = NULL;
  this->root = NULL;
}

channel_json_config::channel_json_config(
    const channel_json_config &json_config) {
  this->properties_root = NULL;
  this->user_root = NULL;
  this->root = NULL;

  *this = json_config;
}

channel_json_config &channel_json_config::operator=(
    const channel_json_config &json_config) {
  json_clear();
  this->root = json_config.root;

  if (json_config.properties_root) {
    this->properties_root =
        cJSON_Duplicate(json_config.properties_root, cJSON_True);
  }

  if (json_config.user_root) {
    this->user_root = cJSON_Duplicate(json_config.user_root, cJSON_True);
  }

  return *this;
}

void channel_json_config::json_clear(void) {
  if (this->properties_root) {
    cJSON_Delete(this->properties_root);
    this->properties_root = NULL;
  }

  if (this->user_root) {
    cJSON_Delete(this->user_root);
    this->user_root = NULL;
  }
}

channel_json_config::channel_json_config(channel_json_config *root) {
  this->properties_root = NULL;
  this->user_root = NULL;
  this->root = root;
}

channel_json_config::channel_json_config(channel_json_config *root,
                                         bool copy_and_detach) {
  this->properties_root = NULL;
  this->user_root = NULL;
  this->root = root;

  if (root && copy_and_detach) {
    this->properties_root =
        cJSON_Duplicate(root->get_properties_root(), cJSON_True);
    this->user_root = cJSON_Duplicate(root->get_user_root(), cJSON_True);
    this->root = NULL;
  }
}

channel_json_config::~channel_json_config(void) { json_clear(); }

// User

cJSON *channel_json_config::get_user_root(void) {
  if (root) {
    return root->get_user_root();
  }

  if (user_root == NULL) {
    user_root = cJSON_CreateObject();
  }

  return user_root;
}

bool channel_json_config::is_root_exists(void) { return root != NULL; }

void channel_json_config::set_user_config(const char *config) {
  if (root) {
    root->set_user_config(config);
    return;
  }

  if (user_root != NULL) {
    cJSON_Delete(user_root);
    user_root = NULL;
  }

  if (config) {
    user_root = cJSON_Parse(config);
  }
}

char *channel_json_config::get_user_config(void) {
  cJSON *json = get_user_root();
  if (json) {
    return cJSON_PrintUnformatted(json);
  }
  return NULL;
}

// Properties

cJSON *channel_json_config::get_properties_root(void) {
  if (root) {
    return root->get_properties_root();
  }

  if (properties_root == NULL) {
    properties_root = cJSON_CreateObject();
  }

  return properties_root;
}

void channel_json_config::set_properties(const char *properties) {
  if (root) {
    root->set_properties(properties);
    return;
  }

  if (properties_root != NULL) {
    cJSON_Delete(properties_root);
    properties_root = NULL;
  }

  if (properties) {
    properties_root = cJSON_Parse(properties);
  }
}

char *channel_json_config::get_properties(void) {
  cJSON *json = get_properties_root();
  if (json) {
    return cJSON_PrintUnformatted(json);
  }
  return NULL;
}

int channel_json_config::get_map_size(void) { return 0; }

int channel_json_config::get_map_key(int index) { return 0; }

const char *channel_json_config::get_map_str(int index) { return NULL; }

bool channel_json_config::key_exists(int key) {
  int size = get_map_size();

  for (int a = 0; a < size; a++) {
    if (get_map_key(a) == key) {
      return true;
    }
  }

  return false;
}

const char *channel_json_config::string_with_key(int key) {
  int size = get_map_size();

  for (int a = 0; a < size; a++) {
    if (get_map_key(a) == key) {
      return get_map_str(a);
    }
  }

  return NULL;
}

int channel_json_config::key_with_string(const char *str) {
  int size = get_map_size();

  for (int a = 0; a < size; a++) {
    if (equal(get_map_str(a), str)) {
      return get_map_key(a);
    }
  }

  return 0;
}

int channel_json_config::json_to_key(cJSON *item) {
  if (item) {
    return key_with_string(cJSON_GetStringValue(item));
  }

  return 0;
}

bool channel_json_config::equal(const char *str1, const char *str2) {
  if (!str1 || !str2) {
    return false;
  }

  size_t str_len = strnlen(str1, MAX_STR_LEN);

  if (str_len != strnlen(str2, MAX_STR_LEN)) {
    return false;
  }

  for (size_t a = 0; a < str_len; a++) {
    if (tolower(str1[a]) != tolower(str2[a])) {
      return false;
    }
  }

  return true;
}

bool channel_json_config::equal(cJSON *item, const char *str) {
  return item && equal(cJSON_GetStringValue(item), str);
}

int channel_json_config::get_int(const char *key) {
  cJSON *root = get_user_root();
  if (!root) {
    return 0;
  }

  cJSON *value = cJSON_GetObjectItem(root, key);
  if (value && cJSON_IsNumber(value)) {
    return value->valueint;
  }

  return 0;
}

double channel_json_config::get_double(const char *key) {
  cJSON *root = get_user_root();
  if (!root) {
    return false;
  }

  cJSON *value = cJSON_GetObjectItem(root, key);
  if (value && cJSON_IsNumber(value)) {
    return value->valuedouble;
  }

  return 0;
}

bool channel_json_config::get_bool(const char *key) {
  cJSON *root = get_user_root();
  if (!root) {
    return false;
  }

  cJSON *value = cJSON_GetObjectItem(root, key);
  return value && cJSON_IsBool(value) && cJSON_IsTrue(value);
}
