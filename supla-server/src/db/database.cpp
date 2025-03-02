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

#include <amazon/alexacredentials.h>
#include <ctype.h>
#include <google/googlehomecredentials.h>
#include <mysql.h>
#include <stdio.h>
#include <time.h>

#include "channeljsonconfig/action_trigger_config.h"
#include "safearray.h"

// https://bugs.mysql.com/bug.php?id=28184
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <string.h>

#include "database.h"
#include "log.h"
#include "svrcfg.h"
#include "tools.h"
#include "userchannelgroups.h"

char *database::get_user_email(int UserID) {
  char *result = NULL;

  char sql[] = "SELECT `email` FROM `supla_user` WHERE id = ?";

  MYSQL_STMT *stmt = NULL;

  MYSQL_BIND pbind;
  memset(&pbind, 0, sizeof(pbind));

  pbind.buffer_type = MYSQL_TYPE_LONG;
  pbind.buffer = (char *)&UserID;

  if (stmt_execute((void **)&stmt, sql, &pbind, 1, true)) {
    char email[SUPLA_EMAIL_MAXSIZE];
    unsigned long size = 0;
    my_bool is_null = true;

    MYSQL_BIND rbind;
    memset(&rbind, 0, sizeof(rbind));

    rbind.buffer_type = MYSQL_TYPE_STRING;
    rbind.buffer = email;
    rbind.buffer_length = SUPLA_EMAIL_MAXSIZE;
    rbind.length = &size;
    rbind.is_null = &is_null;

    if (mysql_stmt_bind_result(stmt, &rbind)) {
      supla_log(LOG_ERR, "MySQL - stmt bind error - %s",
                mysql_stmt_error(stmt));
    } else {
      mysql_stmt_store_result(stmt);

      if (mysql_stmt_num_rows(stmt) > 0 && !mysql_stmt_fetch(stmt)) {
        set_terminating_byte(email, sizeof(email), size, is_null);
        result = strndup(email, SUPLA_EMAIL_MAXSIZE);
      }
    }
    mysql_stmt_close(stmt);
  }

  return result;
}

bool database::get_user_uniqueid(int UserID, char *id, bool longid) {
  bool result = false;
  char sqls[] = "SELECT `short_unique_id` FROM `supla_user` WHERE id = ?";

  char sqll[] = "SELECT `long_unique_id` FROM `supla_user` WHERE id = ?";

  MYSQL_STMT *stmt = NULL;

  MYSQL_BIND pbind[1];
  memset(pbind, 0, sizeof(pbind));

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&UserID;

  if (stmt_execute((void **)&stmt, longid ? sqll : sqls, pbind, 1, true)) {
    MYSQL_BIND rbind;
    memset(&rbind, 0, sizeof(rbind));

    unsigned long size = 0;
    my_bool is_null = true;

    rbind.buffer_type = MYSQL_TYPE_STRING;
    rbind.buffer = id;
    rbind.buffer_length =
        (longid ? LONG_UNIQUEID_MAXSIZE : SHORT_UNIQUEID_MAXSIZE) - 1;
    rbind.length = &size;
    rbind.is_null = &is_null;

    if (mysql_stmt_bind_result(stmt, &rbind)) {
      supla_log(LOG_ERR, "MySQL - stmt bind error - %s",
                mysql_stmt_error(stmt));
    } else {
      mysql_stmt_store_result(stmt);

      if (mysql_stmt_num_rows(stmt) > 0 && !mysql_stmt_fetch(stmt)) {
        id[is_null ? 0 : size] = 0;
        result = true;
      }
    }
    mysql_stmt_close(stmt);
  }

  return result;
}

int database::get_user_id_by_suid(const char *suid) {
  if (_mysql == NULL || suid == NULL || suid[0] == 0) return 0;

  MYSQL_BIND pbind[1];
  memset(pbind, 0, sizeof(pbind));

  pbind[0].buffer_type = MYSQL_TYPE_STRING;
  pbind[0].buffer = (char *)suid;
  pbind[0].buffer_length = strnlen(suid, SHORT_UNIQUEID_MAXSIZE);

  int UserID = 0;
  MYSQL_STMT *stmt = NULL;

  if (stmt_get_int((void **)&stmt, &UserID, NULL, NULL, NULL,
                   "SELECT id FROM supla_user WHERE short_unique_id = ?", pbind,
                   1)) {
    return UserID;
  }

  return 0;
}

void database::get_device_channels(int UserID, int DeviceID,
                                   supla_device_channels *channels) {
  MYSQL_STMT *stmt = NULL;
  const char sql[] =
      "SELECT c.`type`, c.`func`, c.`param1`, c.`param2`, c.`param3`, "
      "c.`param4`, c.`text_param1`, c.`text_param2`, c.`text_param3`, "
      "c.`channel_number`, c.`id`, c.`hidden`, c.`flags`, v.`value`, "
      "TIME_TO_SEC(TIMEDIFF(v.`valid_to`, UTC_TIMESTAMP())) + 2, "
      "c.`user_config`, c.`properties` FROM `supla_dev_channel` c  LEFT JOIN "
      "`supla_dev_channel_value` v ON v.channel_id = c.id AND v.valid_to >= "
      "UTC_TIMESTAMP() WHERE c.`iodevice_id` = ? ORDER BY c.`channel_number`";

  MYSQL_BIND pbind[1];
  memset(pbind, 0, sizeof(pbind));

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&DeviceID;

  if (stmt_execute((void **)&stmt, sql, pbind, 1, true)) {
    my_bool is_null[11] = {};

    MYSQL_BIND rbind[17];
    memset(rbind, 0, sizeof(rbind));

    int type = 0;
    int func = 0;
    int param1 = 0;
    int param2 = 0;
    int param3 = 0;
    int param4 = 0;
    int number = 0;
    int id = 0;
    int hidden = 0;
    int flags = 0;

    char text_param1[256] = {};
    char text_param2[256] = {};
    char text_param3[256] = {};

    unsigned long text_param1_size = 0;
    unsigned long text_param2_size = 0;
    unsigned long text_param3_size = 0;

    char user_config[2049] = {};
    char properties[2049] = {};

    unsigned long user_config_size = 0;
    unsigned long properties_size = 0;

    char value[SUPLA_CHANNELVALUE_SIZE];
    memset(value, 0, SUPLA_CHANNELVALUE_SIZE);
    my_bool value_is_null = true;

    unsigned _supla_int_t validity_time_sec = 0;
    my_bool validity_time_is_null = true;

    rbind[0].buffer_type = MYSQL_TYPE_LONG;
    rbind[0].buffer = (char *)&type;
    rbind[0].is_null = &is_null[0];

    rbind[1].buffer_type = MYSQL_TYPE_LONG;
    rbind[1].buffer = (char *)&func;
    rbind[1].is_null = &is_null[1];

    rbind[2].buffer_type = MYSQL_TYPE_LONG;
    rbind[2].buffer = (char *)&param1;
    rbind[2].is_null = &is_null[2];

    rbind[3].buffer_type = MYSQL_TYPE_LONG;
    rbind[3].buffer = (char *)&param2;
    rbind[3].is_null = &is_null[3];

    rbind[4].buffer_type = MYSQL_TYPE_LONG;
    rbind[4].buffer = (char *)&param3;
    rbind[4].is_null = &is_null[4];

    rbind[5].buffer_type = MYSQL_TYPE_LONG;
    rbind[5].buffer = (char *)&param4;
    rbind[5].is_null = &is_null[5];

    rbind[6].buffer_type = MYSQL_TYPE_STRING;
    rbind[6].buffer = text_param1;
    rbind[6].is_null = &is_null[6];
    rbind[6].buffer_length = sizeof(text_param1) - 1;
    rbind[6].length = &text_param1_size;

    rbind[7].buffer_type = MYSQL_TYPE_STRING;
    rbind[7].buffer = text_param2;
    rbind[7].is_null = &is_null[7];
    rbind[7].buffer_length = sizeof(text_param2) - 1;
    rbind[7].length = &text_param2_size;

    rbind[8].buffer_type = MYSQL_TYPE_STRING;
    rbind[8].buffer = text_param3;
    rbind[8].is_null = &is_null[8];
    rbind[8].buffer_length = sizeof(text_param3) - 1;
    rbind[8].length = &text_param3_size;

    rbind[9].buffer_type = MYSQL_TYPE_LONG;
    rbind[9].buffer = (char *)&number;

    rbind[10].buffer_type = MYSQL_TYPE_LONG;
    rbind[10].buffer = (char *)&id;

    rbind[11].buffer_type = MYSQL_TYPE_LONG;
    rbind[11].buffer = (char *)&hidden;

    rbind[12].buffer_type = MYSQL_TYPE_LONG;
    rbind[12].buffer = (char *)&flags;

    rbind[13].buffer_type = MYSQL_TYPE_BLOB;
    rbind[13].buffer = value;
    rbind[13].buffer_length = SUPLA_CHANNELVALUE_SIZE;
    rbind[13].is_null = &value_is_null;

    rbind[14].buffer_type = MYSQL_TYPE_LONG;
    rbind[14].buffer = (char *)&validity_time_sec;
    rbind[14].buffer_length = sizeof(unsigned _supla_int_t);
    rbind[14].is_null = &validity_time_is_null;

    rbind[15].buffer_type = MYSQL_TYPE_STRING;
    rbind[15].buffer = user_config;
    rbind[15].is_null = &is_null[9];
    rbind[15].buffer_length = sizeof(user_config) - 1;
    rbind[15].length = &user_config_size;

    rbind[16].buffer_type = MYSQL_TYPE_STRING;
    rbind[16].buffer = properties;
    rbind[16].is_null = &is_null[10];
    rbind[16].buffer_length = sizeof(properties) - 1;
    rbind[16].length = &properties_size;

    if (mysql_stmt_bind_result(stmt, rbind)) {
      supla_log(LOG_ERR, "MySQL - stmt bind error - %s",
                mysql_stmt_error(stmt));
    } else {
      mysql_stmt_store_result(stmt);

      if (mysql_stmt_num_rows(stmt) > 0) {
        while (!mysql_stmt_fetch(stmt)) {
          if (is_null[0] == true) type = 0;
          if (is_null[1] == true) func = 0;
          if (is_null[2] == true) param1 = 0;
          if (is_null[3] == true) param2 = 0;
          if (is_null[4] == true) param3 = 0;
          if (is_null[5] == true) param4 = 0;
          if (is_null[6] == true) text_param1_size = 0;
          if (is_null[7] == true) text_param2_size = 0;
          if (is_null[8] == true) text_param3_size = 0;
          if (is_null[9] == true) user_config_size = 0;
          if (is_null[10] == true) properties_size = 0;

          text_param1[text_param1_size] = 0;
          text_param2[text_param2_size] = 0;
          text_param3[text_param3_size] = 0;

          user_config[user_config_size] = 0;
          properties[properties_size] = 0;

          if (value_is_null) {
            memset(value, 0, SUPLA_CHANNELVALUE_SIZE);
          }

          if (validity_time_is_null) {
            validity_time_sec = 0;
          }

          channels->add_channel(id, number, type, func, param1, param2, param3,
                                param4, text_param1, text_param2, text_param3,
                                hidden > 0, flags, value, validity_time_sec,
                                user_config_size ? user_config : NULL,
                                properties_size ? properties : NULL);
        }
      }
    }

    mysql_stmt_close(stmt);
  }
}

void database::get_client_locations(int ClientID,
                                    supla_client_locations *locs) {
  MYSQL_STMT *stmt = NULL;

  const char sql[] =
      "SELECT `id`, `caption` FROM `supla_v_client_location`  WHERE "
      "`client_id` = ?";

  MYSQL_BIND pbind[1];
  memset(pbind, 0, sizeof(pbind));

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&ClientID;

  if (stmt_execute((void **)&stmt, sql, pbind, 1, true)) {
    my_bool is_null[2] = {false, false};

    MYSQL_BIND rbind[2];
    memset(rbind, 0, sizeof(rbind));

    int id = 0;
    unsigned long size = 0;
    char caption[SUPLA_LOCATION_CAPTION_MAXSIZE];  // utf8

    rbind[0].buffer_type = MYSQL_TYPE_LONG;
    rbind[0].buffer = (char *)&id;
    rbind[0].is_null = &is_null[0];

    rbind[1].buffer_type = MYSQL_TYPE_STRING;
    rbind[1].buffer = caption;
    rbind[1].buffer_length = SUPLA_LOCATION_CAPTION_MAXSIZE;
    rbind[1].length = &size;
    rbind[1].is_null = &is_null[1];

    if (mysql_stmt_bind_result(stmt, rbind)) {
      supla_log(LOG_ERR, "MySQL - stmt bind error - %s",
                mysql_stmt_error(stmt));
    } else {
      mysql_stmt_store_result(stmt);

      if (mysql_stmt_num_rows(stmt) > 0) {
        while (!mysql_stmt_fetch(stmt)) {
          if (size >= SUPLA_LOCATION_CAPTION_MAXSIZE) {
            size = SUPLA_LOCATION_CAPTION_MAXSIZE - 1;
          }
          caption[is_null[1] ? 0 : size] = 0;
          locs->add_location(id, caption);
        }
      }
    }

    mysql_stmt_close(stmt);
  }
}

void database::get_client_channels(int ClientID, int *DeviceID,
                                   supla_client_channels *channels) {
  MYSQL_STMT *stmt = NULL;
  const char sql1[] =
      "SELECT `id`, `type`, `func`, `param1`, `param2`, `param3`, `param4`, "
      "`text_param1`, "
      "`text_param2`, `text_param3`, `iodevice_id`, `location_id`, `caption`, "
      "`alt_icon`, `user_icon_id`, `manufacturer_id`, `product_id`, "
      "`protocol_version`, `flags`, `value`, `validity_time_sec` + 2 FROM "
      "`supla_v_client_channel` WHERE `client_id` = ? ORDER BY `iodevice_id`, "
      "`channel_number`";
  const char sql2[] =
      "SELECT `id`, `type`, `func`, `param1`, `param2`, `param3`, `param4`, "
      "`text_param1`, "
      "`text_param2`, `text_param3`, `iodevice_id`, `location_id`, `caption`, "
      "`alt_icon`, `user_icon_id`, `manufacturer_id`, `product_id`, "
      "`protocol_version`, `flags`, `value`, `validity_time_sec` + 2 FROM "
      "`supla_v_client_channel` WHERE `client_id` = ? AND `iodevice_id` = ? "
      "ORDER BY `channel_number`";

  MYSQL_BIND pbind[2];
  memset(pbind, 0, sizeof(pbind));

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&ClientID;

  pbind[1].buffer_type = MYSQL_TYPE_LONG;
  pbind[1].buffer = (char *)DeviceID;

  if (stmt_execute((void **)&stmt, DeviceID ? sql2 : sql1, pbind,
                   DeviceID ? 2 : 1, true)) {
    MYSQL_BIND rbind[21];
    memset(rbind, 0, sizeof(rbind));

    int id = 0, type = 0, func = 0, param1 = 0, param2 = 0, param3 = 0,
        param4 = 0, iodevice_id = 0, location_id = 0, alt_icon = 0,
        user_icon = 0, protocol_version = 0, flags = 0;
    short manufacturer_id = 0;
    short product_id = 0;
    char text_param1[256];
    char text_param2[256];
    char text_param3[256];

    unsigned long caption_size = 0;
    unsigned long text_param1_size = 0;
    unsigned long text_param2_size = 0;
    unsigned long text_param3_size = 0;

    my_bool caption_is_null = true;
    my_bool text_param1_is_null = true;
    my_bool text_param2_is_null = true;
    my_bool text_param3_is_null = true;

    char caption[SUPLA_CHANNEL_CAPTION_MAXSIZE];

    char value[SUPLA_CHANNELVALUE_SIZE];
    memset(value, 0, SUPLA_CHANNELVALUE_SIZE);
    my_bool value_is_null = true;

    unsigned _supla_int_t validity_time_sec = 0;
    my_bool validity_time_is_null = true;

    rbind[0].buffer_type = MYSQL_TYPE_LONG;
    rbind[0].buffer = (char *)&id;

    rbind[1].buffer_type = MYSQL_TYPE_LONG;
    rbind[1].buffer = (char *)&type;

    rbind[2].buffer_type = MYSQL_TYPE_LONG;
    rbind[2].buffer = (char *)&func;

    rbind[3].buffer_type = MYSQL_TYPE_LONG;
    rbind[3].buffer = (char *)&param1;

    rbind[4].buffer_type = MYSQL_TYPE_LONG;
    rbind[4].buffer = (char *)&param2;

    rbind[5].buffer_type = MYSQL_TYPE_LONG;
    rbind[5].buffer = (char *)&param3;

    rbind[6].buffer_type = MYSQL_TYPE_LONG;
    rbind[6].buffer = (char *)&param4;

    rbind[7].buffer_type = MYSQL_TYPE_STRING;
    rbind[7].buffer = text_param1;
    rbind[7].is_null = &text_param1_is_null;
    rbind[7].buffer_length = sizeof(text_param1) - 1;
    rbind[7].length = &text_param1_size;

    rbind[8].buffer_type = MYSQL_TYPE_STRING;
    rbind[8].buffer = text_param2;
    rbind[8].is_null = &text_param2_is_null;
    rbind[8].buffer_length = sizeof(text_param2) - 1;
    rbind[8].length = &text_param2_size;

    rbind[9].buffer_type = MYSQL_TYPE_STRING;
    rbind[9].buffer = text_param3;
    rbind[9].is_null = &text_param3_is_null;
    rbind[9].buffer_length = sizeof(text_param3) - 1;
    rbind[9].length = &text_param3_size;

    rbind[10].buffer_type = MYSQL_TYPE_LONG;
    rbind[10].buffer = (char *)&iodevice_id;

    rbind[11].buffer_type = MYSQL_TYPE_LONG;
    rbind[11].buffer = (char *)&location_id;

    rbind[12].buffer_type = MYSQL_TYPE_STRING;
    rbind[12].buffer = caption;
    rbind[12].is_null = &caption_is_null;
    rbind[12].buffer_length = SUPLA_CHANNEL_CAPTION_MAXSIZE - 1;
    rbind[12].length = &caption_size;

    rbind[13].buffer_type = MYSQL_TYPE_LONG;
    rbind[13].buffer = (char *)&alt_icon;

    rbind[14].buffer_type = MYSQL_TYPE_LONG;
    rbind[14].buffer = (char *)&user_icon;

    rbind[15].buffer_type = MYSQL_TYPE_SHORT;
    rbind[15].buffer = (char *)&manufacturer_id;

    rbind[16].buffer_type = MYSQL_TYPE_SHORT;
    rbind[16].buffer = (char *)&product_id;

    rbind[17].buffer_type = MYSQL_TYPE_LONG;
    rbind[17].buffer = (char *)&protocol_version;

    rbind[18].buffer_type = MYSQL_TYPE_LONG;
    rbind[18].buffer = (char *)&flags;

    rbind[19].buffer_type = MYSQL_TYPE_BLOB;
    rbind[19].buffer = value;
    rbind[19].buffer_length = SUPLA_CHANNELVALUE_SIZE;
    rbind[19].is_null = &value_is_null;

    rbind[20].buffer_type = MYSQL_TYPE_LONG;
    rbind[20].buffer = (char *)&validity_time_sec;
    rbind[20].buffer_length = sizeof(unsigned _supla_int_t);
    rbind[20].is_null = &validity_time_is_null;

    if (mysql_stmt_bind_result(stmt, rbind)) {
      supla_log(LOG_ERR, "MySQL - stmt bind error - %s",
                mysql_stmt_error(stmt));
    } else {
      mysql_stmt_store_result(stmt);

      if (mysql_stmt_num_rows(stmt) > 0) {
        while (!mysql_stmt_fetch(stmt)) {
          if (caption_size >= SUPLA_CHANNEL_CAPTION_MAXSIZE) {
            caption_size = SUPLA_CHANNEL_CAPTION_MAXSIZE - 1;
          }

          caption[caption_size] = 0;
          text_param1[text_param1_size] = 0;
          text_param2[text_param2_size] = 0;
          text_param3[text_param3_size] = 0;

          if (value_is_null) {
            memset(value, 0, SUPLA_CHANNELVALUE_SIZE);
          }

          if (validity_time_is_null) {
            validity_time_sec = 0;
          }

          supla_client_channel *channel = new supla_client_channel(
              channels, id, iodevice_id, location_id, type, func, param1,
              param2, param3, param4, text_param1_is_null ? NULL : text_param1,
              text_param2_is_null ? NULL : text_param2,
              text_param3_is_null ? NULL : text_param3,
              caption_is_null ? NULL : caption, alt_icon, user_icon,
              manufacturer_id, product_id, protocol_version, flags, value,
              validity_time_sec);

          if (!channels->add(channel)) {
            delete channel;
          }
        }
      }
    }

    mysql_stmt_close(stmt);
  }
}

void database::get_user_channel_groups(int UserID,
                                       supla_user_channelgroups *cgroups) {
  MYSQL_STMT *stmt = NULL;
  const char sql[] =
      "SELECT `id`, `channel_id`, `iodevice_id` FROM "
      "`supla_v_user_channel_group` "
      "WHERE `user_id` = ? ORDER BY `id`";

  MYSQL_BIND pbind[1];
  memset(pbind, 0, sizeof(pbind));

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&UserID;

  if (stmt_execute((void **)&stmt, sql, pbind, 1, true)) {
    MYSQL_BIND rbind[3];
    memset(rbind, 0, sizeof(rbind));

    int group_id, channel_id, iodevice_id;

    rbind[0].buffer_type = MYSQL_TYPE_LONG;
    rbind[0].buffer = (char *)&group_id;

    rbind[1].buffer_type = MYSQL_TYPE_LONG;
    rbind[1].buffer = (char *)&channel_id;

    rbind[2].buffer_type = MYSQL_TYPE_LONG;
    rbind[2].buffer = (char *)&iodevice_id;

    if (mysql_stmt_bind_result(stmt, rbind)) {
      supla_log(LOG_ERR, "MySQL - stmt bind error - %s",
                mysql_stmt_error(stmt));
    } else {
      mysql_stmt_store_result(stmt);

      if (mysql_stmt_num_rows(stmt) > 0) {
        while (!mysql_stmt_fetch(stmt)) {
          supla_user_channelgroup *cg = new supla_user_channelgroup(
              cgroups, group_id, channel_id, iodevice_id);
          if (!cgroups->add(cg, master)) {
            delete cg;
          }
        }
      }
    }

    mysql_stmt_close(stmt);
  }
}

void database::get_client_channel_groups(int ClientID,
                                         supla_client_channelgroups *cgroups) {
  MYSQL_STMT *stmt = NULL;
  const char sql[] =
      "SELECT `id`, `func`, `location_id`, `caption`, `alt_icon`, "
      "`user_icon_id`  FROM `supla_v_client_channel_group` WHERE `client_id` = "
      "? ORDER BY `id`";

  MYSQL_BIND pbind[1];
  memset(pbind, 0, sizeof(pbind));

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&ClientID;

  if (stmt_execute((void **)&stmt, sql, pbind, 1, true)) {
    my_bool is_null;

    MYSQL_BIND rbind[6];
    memset(rbind, 0, sizeof(rbind));

    int id = 0;
    int func = 0;
    int location_id = 0;
    int alt_icon = 0;
    int user_icon = 0;
    unsigned long size = 0;
    char caption[SUPLA_CHANNELGROUP_CAPTION_MAXSIZE];

    rbind[0].buffer_type = MYSQL_TYPE_LONG;
    rbind[0].buffer = (char *)&id;

    rbind[1].buffer_type = MYSQL_TYPE_LONG;
    rbind[1].buffer = (char *)&func;

    rbind[2].buffer_type = MYSQL_TYPE_LONG;
    rbind[2].buffer = (char *)&location_id;

    rbind[3].buffer_type = MYSQL_TYPE_STRING;
    rbind[3].buffer = caption;
    rbind[3].is_null = &is_null;
    rbind[3].buffer_length = SUPLA_CHANNELGROUP_CAPTION_MAXSIZE;
    rbind[3].length = &size;

    rbind[4].buffer_type = MYSQL_TYPE_LONG;
    rbind[4].buffer = (char *)&alt_icon;

    rbind[5].buffer_type = MYSQL_TYPE_LONG;
    rbind[5].buffer = (char *)&user_icon;

    if (mysql_stmt_bind_result(stmt, rbind)) {
      supla_log(LOG_ERR, "MySQL - stmt bind error - %s",
                mysql_stmt_error(stmt));
    } else {
      mysql_stmt_store_result(stmt);

      if (mysql_stmt_num_rows(stmt) > 0) {
        while (!mysql_stmt_fetch(stmt)) {
          if (size >= SUPLA_CHANNELGROUP_CAPTION_MAXSIZE) {
            size = SUPLA_CHANNELGROUP_CAPTION_MAXSIZE - 1;
          }
          caption[size] = 0;

          supla_client_channelgroup *cg = new supla_client_channelgroup(
              cgroups, id, location_id, func, is_null ? NULL : caption,
              alt_icon, user_icon);
          if (!cgroups->add(cg, master)) {
            delete cg;
          }
        }
      }
    }

    mysql_stmt_close(stmt);
  }
}

void database::get_client_channel_group_relations(
    int ClientID, supla_client_channelgroups *cgroups) {
  MYSQL_STMT *stmt = NULL;
  const char sql[] =
      "SELECT `channel_id`, `group_id`, `channel_hidden`, `iodevice_id` FROM "
      "`supla_v_rel_cg` "
      "WHERE `client_id` "
      "= ? ORDER BY `channel_id`";

  MYSQL_BIND pbind[1];
  memset(pbind, 0, sizeof(pbind));

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&ClientID;

  if (stmt_execute((void **)&stmt, sql, pbind, 1, true)) {
    MYSQL_BIND rbind[4];
    memset(rbind, 0, sizeof(rbind));

    int channel_id, group_id, hidden, iodevice_id;

    rbind[0].buffer_type = MYSQL_TYPE_LONG;
    rbind[0].buffer = (char *)&channel_id;

    rbind[1].buffer_type = MYSQL_TYPE_LONG;
    rbind[1].buffer = (char *)&group_id;

    rbind[2].buffer_type = MYSQL_TYPE_LONG;
    rbind[2].buffer = (char *)&hidden;

    rbind[3].buffer_type = MYSQL_TYPE_LONG;
    rbind[3].buffer = (char *)&iodevice_id;

    if (mysql_stmt_bind_result(stmt, rbind)) {
      supla_log(LOG_ERR, "MySQL - stmt bind error - %s",
                mysql_stmt_error(stmt));
    } else {
      mysql_stmt_store_result(stmt);

      if (mysql_stmt_num_rows(stmt) > 0) {
        while (!mysql_stmt_fetch(stmt)) {
          supla_client_channelgroup_relation *cg_rel =
              new supla_client_channelgroup_relation(cgroups, iodevice_id,
                                                     channel_id, group_id);
          if (!cgroups->add(cg_rel, detail1)) {
            delete cg_rel;
          }
          if (hidden > 0) {
            supla_client_channelgroup_value *cg_value =
                new supla_client_channelgroup_value(cgroups, channel_id,
                                                    iodevice_id);
            if (!cgroups->add(cg_value, detail2)) {
              delete cg_value;
            }
          }
        }
      }
    }

    mysql_stmt_close(stmt);
  }
}

bool database::superuser_authorization(
    int UserID, const char email[SUPLA_EMAIL_MAXSIZE],
    const char password[SUPLA_PASSWORD_MAXSIZE]) {
  MYSQL_STMT *stmt = NULL;

  MYSQL_BIND pbind[1];
  memset(pbind, 0, sizeof(pbind));

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&UserID;

  bool result = false;

  if (stmt_execute((void **)&stmt,
                   "SELECT email, password FROM supla_user WHERE id = ?", pbind,
                   1, true)) {
    MYSQL_BIND rbind[2];
    memset(rbind, 0, sizeof(rbind));

    char buffer_email[256];
    unsigned long email_size = 0;
    my_bool email_is_null = true;

    char buffer_password[65];
    unsigned long password_size = 0;
    my_bool password_is_null = true;

    rbind[0].buffer_type = MYSQL_TYPE_STRING;
    rbind[0].buffer = buffer_email;
    rbind[0].buffer_length = sizeof(buffer_email);
    rbind[0].length = &email_size;
    rbind[0].is_null = &email_is_null;

    rbind[1].buffer_type = MYSQL_TYPE_STRING;
    rbind[1].buffer = buffer_password;
    rbind[1].buffer_length = sizeof(buffer_password);
    rbind[1].length = &password_size;
    rbind[1].is_null = &password_is_null;

    if (mysql_stmt_bind_result(stmt, rbind)) {
      supla_log(LOG_ERR, "MySQL - stmt bind error - %s",
                mysql_stmt_error(stmt));
    } else {
      mysql_stmt_store_result(stmt);

      if (mysql_stmt_num_rows(stmt) > 0 && !mysql_stmt_fetch(stmt) &&
          !email_is_null && !password_is_null &&
          email_size < rbind[0].buffer_length &&
          password_size < rbind[1].buffer_length) {
        buffer_email[email_size] = 0;
        buffer_password[password_size] = 0;

        if (strncmp(email, buffer_email, email_size) == 0 &&
            st_bcrypt_check(password, buffer_password, password_size) == 1) {
          result = 1;
        }
      }
    }

    mysql_stmt_close(stmt);
  }

  return result;
}

bool database::amazon_alexa_load_credentials(
    supla_amazon_alexa_credentials *alexa) {
  bool result = false;
  char sql[] =
      "SELECT `access_token`, `refresh_token`, TIMESTAMPDIFF(SECOND, "
      "UTC_TIMESTAMP(), expires_at) `expires_in`, `region` "
      "FROM "
      "`supla_amazon_alexa` WHERE user_id = ? AND LENGTH(access_token) > 0";

  MYSQL_STMT *stmt = NULL;

  MYSQL_BIND pbind[1];
  memset(pbind, 0, sizeof(pbind));

  int UserID = alexa->getUserID();

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&UserID;

  if (stmt_execute((void **)&stmt, sql, pbind, 1, true)) {
    MYSQL_BIND rbind[4];
    memset(rbind, 0, sizeof(rbind));

    char buffer_token[ALEXA_TOKEN_MAXSIZE + 1];
    buffer_token[0] = 0;
    unsigned long token_size = 0;
    my_bool token_is_null = true;

    char buffer_refresh_token[ALEXA_TOKEN_MAXSIZE + 1];
    buffer_refresh_token[0] = 0;
    unsigned long refresh_token_size = 0;
    my_bool refresh_token_is_null = true;

    char buffer_region[ALEXA_REGION_MAXSIZE + 1];
    buffer_region[0] = 0;
    unsigned long region_size = 0;
    my_bool region_is_null = true;

    int expires_in = 0;

    rbind[0].buffer_type = MYSQL_TYPE_STRING;
    rbind[0].buffer = buffer_token;
    rbind[0].buffer_length = ALEXA_TOKEN_MAXSIZE;
    rbind[0].length = &token_size;
    rbind[0].is_null = &token_is_null;

    rbind[1].buffer_type = MYSQL_TYPE_STRING;
    rbind[1].buffer = buffer_refresh_token;
    rbind[1].buffer_length = ALEXA_TOKEN_MAXSIZE;
    rbind[1].length = &refresh_token_size;
    rbind[1].is_null = &refresh_token_is_null;

    rbind[2].buffer_type = MYSQL_TYPE_LONG;
    rbind[2].buffer = (char *)&expires_in;

    rbind[3].buffer_type = MYSQL_TYPE_STRING;
    rbind[3].buffer = buffer_region;
    rbind[3].buffer_length = ALEXA_REGION_MAXSIZE;
    rbind[3].length = &region_size;
    rbind[3].is_null = &region_is_null;

    if (mysql_stmt_bind_result(stmt, rbind)) {
      supla_log(LOG_ERR, "MySQL - stmt bind error - %s",
                mysql_stmt_error(stmt));
    } else {
      mysql_stmt_store_result(stmt);

      if (mysql_stmt_num_rows(stmt) > 0 && !mysql_stmt_fetch(stmt)) {
        buffer_token[token_is_null ? 0 : token_size] = 0;
        buffer_refresh_token[refresh_token_is_null ? 0 : refresh_token_size] =
            0;
        buffer_region[region_is_null ? 0 : region_size] = 0;

        alexa->set(buffer_token, buffer_refresh_token, expires_in,
                   region_is_null ? NULL : buffer_region);
        result = true;
      }
    }
    mysql_stmt_close(stmt);
  }

  return result;
}

void database::amazon_alexa_remove_token(
    supla_amazon_alexa_credentials *alexa) {
  MYSQL_BIND pbind[1];
  memset(pbind, 0, sizeof(pbind));

  int UserID = alexa->getUserID();

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&UserID;

  const char sql[] = "CALL `supla_update_amazon_alexa`('','',0,?)";

  MYSQL_STMT *stmt = NULL;
  stmt_execute((void **)&stmt, sql, pbind, 1, true);

  if (stmt != NULL) mysql_stmt_close(stmt);
}

void database::amazon_alexa_update_token(supla_amazon_alexa_credentials *alexa,
                                         const char *token,
                                         const char *refresh_token,
                                         int expires_in) {
  MYSQL_BIND pbind[4];
  memset(pbind, 0, sizeof(pbind));

  int UserID = alexa->getUserID();

  pbind[0].buffer_type = MYSQL_TYPE_STRING;
  pbind[0].buffer = (char *)token;
  pbind[0].buffer_length = strnlen((char *)token, ALEXA_TOKEN_MAXSIZE);

  pbind[1].buffer_type = MYSQL_TYPE_STRING;
  pbind[1].buffer = (char *)refresh_token;
  pbind[1].buffer_length = strnlen((char *)refresh_token, ALEXA_TOKEN_MAXSIZE);

  pbind[2].buffer_type = MYSQL_TYPE_LONG;
  pbind[2].buffer = (char *)&expires_in;

  pbind[3].buffer_type = MYSQL_TYPE_LONG;
  pbind[3].buffer = (char *)&UserID;

  const char sql[] = "CALL `supla_update_amazon_alexa`(?,?,?,?)";

  MYSQL_STMT *stmt = NULL;
  stmt_execute((void **)&stmt, sql, pbind, 4, true);

  if (stmt != NULL) mysql_stmt_close(stmt);
}

bool database::google_home_load_credentials(
    supla_google_home_credentials *google_home) {
  bool result = false;
  char sql[] =
      "SELECT `access_token` FROM `supla_google_home` WHERE user_id = ? AND "
      "LENGTH(access_token) > 0";

  MYSQL_STMT *stmt = NULL;

  MYSQL_BIND pbind[1];
  memset(pbind, 0, sizeof(pbind));

  int UserID = google_home->getUserID();

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&UserID;

  if (stmt_execute((void **)&stmt, sql, pbind, 1, true)) {
    MYSQL_BIND rbind[1];
    memset(rbind, 0, sizeof(rbind));

    char buffer_token[GH_TOKEN_MAXSIZE + 1];
    buffer_token[0] = 0;
    unsigned long token_size = 0;
    my_bool token_is_null = true;

    rbind[0].buffer_type = MYSQL_TYPE_STRING;
    rbind[0].buffer = buffer_token;
    rbind[0].buffer_length = GH_TOKEN_MAXSIZE;
    rbind[0].length = &token_size;
    rbind[0].is_null = &token_is_null;

    if (mysql_stmt_bind_result(stmt, rbind)) {
      supla_log(LOG_ERR, "MySQL - stmt bind error - %s",
                mysql_stmt_error(stmt));
    } else {
      mysql_stmt_store_result(stmt);

      if (mysql_stmt_num_rows(stmt) > 0 && !mysql_stmt_fetch(stmt)) {
        buffer_token[token_is_null ? 0 : token_size] = 0;

        google_home->set(buffer_token);
        result = true;
      }
    }
    mysql_stmt_close(stmt);
  }

  return result;
}

bool database::state_webhook_load_credentials(
    supla_state_webhook_credentials *webhook) {
  bool result = false;
  char sql[] =
      "SELECT `access_token`, `refresh_token`, TIMESTAMPDIFF(SECOND, "
      "UTC_TIMESTAMP(), expires_at) `expires_in`, `url`, `functions_ids` "
      "FROM "
      "`supla_state_webhooks` WHERE user_id = ? AND enabled = 1 AND "
      "LENGTH(access_token) > 0";

  MYSQL_STMT *stmt = NULL;

  MYSQL_BIND pbind[1];
  memset(pbind, 0, sizeof(pbind));

  int UserID = webhook->getUserID();

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&UserID;

  if (stmt_execute((void **)&stmt, sql, pbind, 1, true)) {
    MYSQL_BIND rbind[5];
    memset(rbind, 0, sizeof(rbind));

    char buffer_token[ALEXA_TOKEN_MAXSIZE + 1];
    buffer_token[0] = 0;
    unsigned long token_size = 0;

    char buffer_refresh_token[ALEXA_TOKEN_MAXSIZE + 1];
    buffer_refresh_token[0] = 0;
    unsigned long refresh_token_size = 0;

    char buffer_url[WEBHOOK_URL_MAXSIZE + 1];
    buffer_url[0] = 0;
    unsigned long url_size = 0;

    char buffer_functions[WEBHOOK_FUNCTIONS_IDS_MAXSIZE + 1];
    buffer_functions[0] = 0;
    unsigned long functions_size = 0;

    int expires_in = 0;

    rbind[0].buffer_type = MYSQL_TYPE_STRING;
    rbind[0].buffer = buffer_token;
    rbind[0].buffer_length = WEBHOOK_TOKEN_MAXSIZE;
    rbind[0].length = &token_size;

    rbind[1].buffer_type = MYSQL_TYPE_STRING;
    rbind[1].buffer = buffer_refresh_token;
    rbind[1].buffer_length = WEBHOOK_TOKEN_MAXSIZE;
    rbind[1].length = &refresh_token_size;

    rbind[2].buffer_type = MYSQL_TYPE_LONG;
    rbind[2].buffer = (char *)&expires_in;

    rbind[3].buffer_type = MYSQL_TYPE_STRING;
    rbind[3].buffer = buffer_url;
    rbind[3].buffer_length = WEBHOOK_URL_MAXSIZE;
    rbind[3].length = &url_size;

    rbind[4].buffer_type = MYSQL_TYPE_STRING;
    rbind[4].buffer = buffer_functions;
    rbind[4].buffer_length = WEBHOOK_FUNCTIONS_IDS_MAXSIZE;
    rbind[4].length = &functions_size;

    if (mysql_stmt_bind_result(stmt, rbind)) {
      supla_log(LOG_ERR, "MySQL - stmt bind error - %s",
                mysql_stmt_error(stmt));
    } else {
      mysql_stmt_store_result(stmt);

      if (mysql_stmt_num_rows(stmt) > 0 && !mysql_stmt_fetch(stmt)) {
        buffer_token[token_size] = 0;
        buffer_refresh_token[refresh_token_size] = 0;
        buffer_url[url_size] = 0;
        buffer_functions[functions_size] = 0;

        webhook->set(buffer_token, buffer_refresh_token, expires_in, buffer_url,
                     buffer_functions);
        result = true;
      }
    }
    mysql_stmt_close(stmt);
  }

  return result;
}

void database::state_webhook_update_token(int UserID, const char *token,
                                          const char *refresh_token,
                                          int expires_in) {
  MYSQL_BIND pbind[4];
  memset(pbind, 0, sizeof(pbind));

  pbind[0].buffer_type = MYSQL_TYPE_STRING;
  pbind[0].buffer = (char *)token;
  pbind[0].buffer_length = strnlen((char *)token, WEBHOOK_TOKEN_MAXSIZE);

  pbind[1].buffer_type = MYSQL_TYPE_STRING;
  pbind[1].buffer = (char *)refresh_token;
  pbind[1].buffer_length =
      strnlen((char *)refresh_token, WEBHOOK_TOKEN_MAXSIZE);

  pbind[2].buffer_type = MYSQL_TYPE_LONG;
  pbind[2].buffer = (char *)&expires_in;

  pbind[3].buffer_type = MYSQL_TYPE_LONG;
  pbind[3].buffer = (char *)&UserID;

  const char sql[] = "CALL `supla_update_state_webhook`(?,?,?,?)";

  MYSQL_STMT *stmt = NULL;
  stmt_execute((void **)&stmt, sql, pbind, 4, true);

  if (stmt != NULL) mysql_stmt_close(stmt);
}

void database::state_webhook_remove_token(int UserID) {
  MYSQL_BIND pbind[1];
  memset(pbind, 0, sizeof(pbind));

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&UserID;

  const char sql[] = "CALL `supla_update_state_webhook`('','',0,?)";

  MYSQL_STMT *stmt = NULL;
  stmt_execute((void **)&stmt, sql, pbind, 1, true);

  if (stmt != NULL) mysql_stmt_close(stmt);
}

bool database::get_channel_basic_cfg(int ChannelID, TSC_ChannelBasicCfg *cfg) {
  if (cfg == NULL) {
    return false;
  }
  bool result = false;
  char sql[] =
      "SELECT d.name, d.software_version, c.iodevice_id, d.flags, "
      "d.manufacturer_id, d.product_id, c.channel_number, "
      "c.type, c.func, c.flist, c.flags, c.caption FROM `supla_dev_channel` c, "
      "`supla_iodevice` d WHERE d.id = c.iodevice_id AND c.id = ?";

  memset(cfg, 0, sizeof(TSC_ChannelBasicCfg));

  MYSQL_STMT *stmt = NULL;
  MYSQL_BIND pbind[1];
  memset(pbind, 0, sizeof(pbind));

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&ChannelID;

  if (stmt_execute((void **)&stmt, sql, pbind, 1, true)) {
    MYSQL_BIND rbind[12];
    memset(rbind, 0, sizeof(rbind));

    unsigned long device_name_size = 0;
    my_bool device_name_is_null = true;

    unsigned long device_softver_size = 0;
    my_bool device_softver_is_null = true;

    unsigned long caption_size = 0;
    my_bool caption_is_null = true;

    rbind[0].buffer_type = MYSQL_TYPE_STRING;
    rbind[0].buffer = cfg->DeviceName;
    rbind[0].buffer_length = sizeof(cfg->DeviceName);
    rbind[0].length = &device_name_size;
    rbind[0].is_null = &device_name_is_null;

    rbind[1].buffer_type = MYSQL_TYPE_STRING;
    rbind[1].buffer = cfg->DeviceSoftVer;
    rbind[1].buffer_length = sizeof(cfg->DeviceSoftVer);
    rbind[1].length = &device_softver_size;
    rbind[1].is_null = &device_softver_is_null;

    rbind[2].buffer_type = MYSQL_TYPE_LONG;
    rbind[2].buffer = (char *)&cfg->DeviceID;

    rbind[3].buffer_type = MYSQL_TYPE_LONG;
    rbind[3].buffer = (char *)&cfg->DeviceFlags;

    rbind[4].buffer_type = MYSQL_TYPE_LONG;
    rbind[4].buffer = (char *)&cfg->ManufacturerID;

    rbind[5].buffer_type = MYSQL_TYPE_LONG;
    rbind[5].buffer = (char *)&cfg->ProductID;

    rbind[6].buffer_type = MYSQL_TYPE_LONG;
    rbind[6].buffer = (char *)&cfg->Number;

    rbind[7].buffer_type = MYSQL_TYPE_LONG;
    rbind[7].buffer = (char *)&cfg->Type;

    rbind[8].buffer_type = MYSQL_TYPE_LONG;
    rbind[8].buffer = (char *)&cfg->Func;

    rbind[9].buffer_type = MYSQL_TYPE_LONG;
    rbind[9].buffer = (char *)&cfg->FuncList;

    rbind[10].buffer_type = MYSQL_TYPE_LONG;
    rbind[10].buffer = (char *)&cfg->ChannelFlags;

    rbind[11].buffer_type = MYSQL_TYPE_STRING;
    rbind[11].buffer = cfg->Caption;
    rbind[11].buffer_length = SUPLA_CHANNEL_CAPTION_MAXSIZE;
    rbind[11].length = &caption_size;
    rbind[11].is_null = &caption_is_null;

    if (mysql_stmt_bind_result(stmt, rbind)) {
      supla_log(LOG_ERR, "MySQL - stmt bind error - %s",
                mysql_stmt_error(stmt));
    } else {
      mysql_stmt_store_result(stmt);

      if (mysql_stmt_num_rows(stmt) > 0 && !mysql_stmt_fetch(stmt)) {
        set_terminating_byte(cfg->DeviceName, sizeof(cfg->DeviceName),
                             device_name_size, device_name_is_null);

        set_terminating_byte(cfg->DeviceSoftVer, sizeof(cfg->DeviceSoftVer),
                             device_softver_size, device_softver_is_null);

        set_terminating_byte(cfg->Caption, SUPLA_CHANNEL_CAPTION_MAXSIZE,
                             caption_size, caption_is_null);

        cfg->CaptionSize =
            strnlen(cfg->Caption, SUPLA_CHANNEL_CAPTION_MAXSIZE) + 1;

        cfg->ID = ChannelID;
        result = true;
      }
    }
    mysql_stmt_close(stmt);
  }

  return result;
}

bool database::set_channel_function(int UserID, int ChannelID, int Func) {
  char sql[100];
  snprintf(sql, sizeof(sql), "CALL `supla_set_channel_function`(%i, %i, %i)",
           UserID, ChannelID, Func);

  return query(sql, true) == 0;
}

bool database::get_channel_type_funclist_and_device_id(int UserID,
                                                       int ChannelID, int *Type,
                                                       unsigned int *FuncList,
                                                       int *DeviceID) {
  if (Type == NULL || FuncList == NULL || DeviceID == NULL) {
    return false;
  }

  *Type = 0;
  *FuncList = 0;

  bool result = false;
  char sql[] =
      "SELECT type, flist, iodevice_id FROM `supla_dev_channel` WHERE user_id "
      "= ? AND id = ?";

  MYSQL_STMT *stmt = NULL;
  MYSQL_BIND pbind[2];
  memset(pbind, 0, sizeof(pbind));

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&UserID;

  pbind[1].buffer_type = MYSQL_TYPE_LONG;
  pbind[1].buffer = (char *)&ChannelID;

  if (stmt_execute((void **)&stmt, sql, pbind, 2, true)) {
    MYSQL_BIND rbind[3];
    memset(rbind, 0, sizeof(rbind));

    my_bool flist_is_null = true;

    rbind[0].buffer_type = MYSQL_TYPE_LONG;
    rbind[0].buffer = (char *)Type;

    rbind[1].buffer_type = MYSQL_TYPE_LONG;
    rbind[1].buffer = (char *)FuncList;
    rbind[1].is_null = &flist_is_null;

    rbind[2].buffer_type = MYSQL_TYPE_LONG;
    rbind[2].buffer = (char *)DeviceID;

    if (mysql_stmt_bind_result(stmt, rbind)) {
      supla_log(LOG_ERR, "MySQL - stmt bind error - %s",
                mysql_stmt_error(stmt));
    } else {
      mysql_stmt_store_result(stmt);

      if (mysql_stmt_num_rows(stmt) > 0 && !mysql_stmt_fetch(stmt)) {
        if (flist_is_null) {
          *FuncList = 0;
        }
        result = true;
      }
    }
  }

  if (stmt != NULL) mysql_stmt_close(stmt);

  return result;
}

bool database::set_caption(int UserID, int ID, char *Caption, int call_id) {
  MYSQL_BIND pbind[3];
  memset(pbind, 0, sizeof(pbind));

  my_bool caption_is_null = true;

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&UserID;

  pbind[1].buffer_type = MYSQL_TYPE_LONG;
  pbind[1].buffer = (char *)&ID;

  pbind[2].is_null = &caption_is_null;
  pbind[2].buffer_type = MYSQL_TYPE_STRING;
  pbind[2].buffer_length =
      Caption == NULL ? 0 : strnlen(Caption, SUPLA_CAPTION_MAXSIZE);

  if (pbind[2].buffer_length > 0) {
    pbind[2].buffer = Caption;
    caption_is_null = false;
  }

  bool result = false;
  MYSQL_STMT *stmt = NULL;

  char *sql = nullptr;

  char sql_c[] = "CALL `supla_set_channel_caption`(?,?,?)";
  char sql_l[] = "CALL `supla_set_location_caption`(?,?,?)";
  char sql_s[] = "CALL `supla_set_scene_caption`(?,?,?)";

  switch (call_id) {
    case SUPLA_CS_CALL_SET_CHANNEL_CAPTION:
      sql = sql_c;
      break;
    case SUPLA_CS_CALL_SET_LOCATION_CAPTION:
      sql = sql_l;
      break;
    case SUPLA_CS_CALL_SET_SCENE_CAPTION:
      sql = sql_s;
      break;
  }

  if (sql && stmt_execute((void **)&stmt, sql, pbind, 3, true)) {
    result = true;
  }

  if (stmt != nullptr) mysql_stmt_close(stmt);

  return result;
}

bool database::channel_belong_to_group(int channel_id) {
  const char sql[] =
      "SELECT group_id FROM supla_rel_cg WHERE channel_id = ? LIMIT 1";
  return get_int(channel_id, 0, sql) > 0;
}

bool database::channel_has_schedule(int channel_id) {
  const char sql[] =
      "SELECT id FROM supla_schedule WHERE channel_id = ? LIMIT 1";
  return get_int(channel_id, 0, sql) > 0;
}

bool database::channel_is_associated_with_scene(int channel_id) {
  const char sql[] =
      "SELECT id FROM supla_scene_operation WHERE channel_id = ? LIMIT 1";
  return get_int(channel_id, 0, sql) > 0;
}

bool database::channel_is_associated_with_action_trigger(int UserID,
                                                         int ChannelID) {
  bool result = false;

  if (UserID == 0 || ChannelID == 0) {
    return false;
  }

  MYSQL_STMT *stmt = NULL;
  const char sql[] =
      "SELECT user_config, properties FROM supla_dev_channel WHERE user_id = ? "
      "AND func = ?";

  int func = SUPLA_CHANNELFNC_ACTIONTRIGGER;

  MYSQL_BIND pbind[4];
  memset(pbind, 0, sizeof(pbind));

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&UserID;

  pbind[1].buffer_type = MYSQL_TYPE_LONG;
  pbind[1].buffer = (char *)&func;

  if (stmt_execute((void **)&stmt, sql, pbind, 2, true)) {
    MYSQL_BIND rbind[2] = {};

    char properties[2049] = {};
    char user_config[2049] = {};

    unsigned long user_config_size = 0;
    unsigned long properties_size = 0;

    my_bool is_null[2] = {true, true};

    rbind[0].buffer_type = MYSQL_TYPE_STRING;
    rbind[0].buffer = user_config;
    rbind[0].is_null = &is_null[0];
    rbind[0].buffer_length = sizeof(user_config) - 1;
    rbind[0].length = &user_config_size;

    rbind[1].buffer_type = MYSQL_TYPE_STRING;
    rbind[1].buffer = properties;
    rbind[1].is_null = &is_null[1];
    rbind[1].buffer_length = sizeof(properties) - 1;
    rbind[1].length = &properties_size;

    if (mysql_stmt_bind_result(stmt, rbind)) {
      supla_log(LOG_ERR, "MySQL - stmt bind error - %s",
                mysql_stmt_error(stmt));
    } else {
      mysql_stmt_store_result(stmt);

      if (mysql_stmt_num_rows(stmt) > 0) {
        action_trigger_config *config = new action_trigger_config();
        if (config) {
          while (!mysql_stmt_fetch(stmt)) {
            config->set_user_config(is_null[0] ? NULL : user_config);
            config->set_properties(is_null[1] ? NULL : properties);

            if (config->channel_exists(ChannelID)) {
              result = true;
              break;
            }
          }
          delete config;
        }
      }
    }

    mysql_stmt_close(stmt);
  }

  return result;
}

void database::update_channel_value(int channel_id, int user_id,
                                    const char value[SUPLA_CHANNELVALUE_SIZE],
                                    unsigned _supla_int_t validity_time_sec) {
  MYSQL_STMT *stmt = NULL;
  MYSQL_BIND pbind[4];
  memset(pbind, 0, sizeof(pbind));

  char value_hex[SUPLA_CHANNELVALUE_SIZE * 2 + 1];
  st_bin2hex(value_hex, value, SUPLA_CHANNELVALUE_SIZE);

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&channel_id;

  pbind[1].buffer_type = MYSQL_TYPE_LONG;
  pbind[1].buffer = (char *)&user_id;

  pbind[2].buffer_type = MYSQL_TYPE_STRING;
  pbind[2].buffer = (char *)value_hex;
  pbind[2].buffer_length = SUPLA_CHANNELVALUE_SIZE * 2;

  pbind[3].buffer_type = MYSQL_TYPE_LONG;
  pbind[3].buffer = (char *)&validity_time_sec;

  const char sql[] = "CALL `supla_update_channel_value`(?, ?, unhex(?), ?)";

  if (stmt_execute((void **)&stmt, sql, pbind, 4, true)) {
    if (stmt != NULL) mysql_stmt_close((MYSQL_STMT *)stmt);
  }
}

bool database::get_channel_value(int user_id, int channel_id,
                                 char value[SUPLA_CHANNELVALUE_SIZE],
                                 unsigned _supla_int_t *validity_time_sec) {
  if (channel_id == 0 || value == NULL || validity_time_sec == NULL) {
    return false;
  }

  bool result = false;
  const char sql[] =
      "SELECT `value`, TIME_TO_SEC(TIMEDIFF(`valid_to`, UTC_TIMESTAMP())) + 2 "
      "FROM `supla_dev_channel_value` WHERE `channel_id` = ? AND `user_id` = ? "
      ", `valid_to` >= UTC_TIMESTAMP()";

  MYSQL_STMT *stmt = NULL;
  MYSQL_BIND pbind[2];
  memset(pbind, 0, sizeof(pbind));

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&channel_id;

  pbind[1].buffer_type = MYSQL_TYPE_LONG;
  pbind[1].buffer = (char *)&user_id;

  if (stmt_execute((void **)&stmt, sql, pbind, 2, true)) {
    MYSQL_BIND rbind[2];
    memset(rbind, 0, sizeof(rbind));

    rbind[0].buffer_type = MYSQL_TYPE_BLOB;
    rbind[0].buffer = value;
    rbind[0].buffer_length = SUPLA_CHANNELVALUE_SIZE;

    rbind[1].buffer_type = MYSQL_TYPE_LONG;
    rbind[1].buffer = (char *)validity_time_sec;
    rbind[1].buffer_length = sizeof(unsigned _supla_int_t);

    if (mysql_stmt_bind_result(stmt, rbind)) {
      supla_log(LOG_ERR, "MySQL - stmt bind error - %s",
                mysql_stmt_error(stmt));
    } else {
      mysql_stmt_store_result(stmt);

      if (mysql_stmt_num_rows(stmt) > 0 && !mysql_stmt_fetch(stmt)) {
        result = true;
      }
    }
  }

  if (stmt != NULL) mysql_stmt_close(stmt);

  return result;
}

void database::update_channel_properties(int channel_id, int user_id,
                                         const char *properties) {
  MYSQL_STMT *stmt = NULL;
  MYSQL_BIND pbind[3];
  memset(pbind, 0, sizeof(pbind));

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&channel_id;

  pbind[1].buffer_type = MYSQL_TYPE_LONG;
  pbind[1].buffer = (char *)&user_id;

  pbind[2].buffer_type = MYSQL_TYPE_STRING;
  pbind[2].buffer = (char *)properties;
  pbind[2].buffer_length = strnlen(properties, 2049);

  const char sql[] = "CALL `supla_update_channel_properties`(?, ?, ?)";

  if (stmt_execute((void **)&stmt, sql, pbind, 3, true)) {
    if (stmt != NULL) mysql_stmt_close((MYSQL_STMT *)stmt);
  }
}

void database::update_channel_params(int channel_id, int user_id, int param1,
                                     int param2, int param3, int param4) {
  MYSQL_STMT *stmt = NULL;
  MYSQL_BIND pbind[6];
  memset(pbind, 0, sizeof(pbind));

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&channel_id;

  pbind[1].buffer_type = MYSQL_TYPE_LONG;
  pbind[1].buffer = (char *)&user_id;

  pbind[2].buffer_type = MYSQL_TYPE_LONG;
  pbind[2].buffer = (char *)&param1;

  pbind[3].buffer_type = MYSQL_TYPE_LONG;
  pbind[3].buffer = (char *)&param2;

  pbind[4].buffer_type = MYSQL_TYPE_LONG;
  pbind[4].buffer = (char *)&param3;

  pbind[5].buffer_type = MYSQL_TYPE_LONG;
  pbind[5].buffer = (char *)&param4;

  const char sql[] = "CALL `supla_update_channel_params`(?, ?, ?, ?, ?, ?)";

  if (stmt_execute((void **)&stmt, sql, pbind, 6, true)) {
    if (stmt != NULL) mysql_stmt_close((MYSQL_STMT *)stmt);
  }
}

void database::update_channel_flags(int channel_id, int user_id,
                                    unsigned int flags) {
  MYSQL_STMT *stmt = NULL;
  MYSQL_BIND pbind[6];
  memset(pbind, 0, sizeof(pbind));

  pbind[0].buffer_type = MYSQL_TYPE_LONG;
  pbind[0].buffer = (char *)&channel_id;

  pbind[1].buffer_type = MYSQL_TYPE_LONG;
  pbind[1].buffer = (char *)&user_id;

  pbind[2].buffer_type = MYSQL_TYPE_LONG;
  pbind[2].buffer = (char *)&flags;

  const char sql[] = "CALL `supla_update_channel_flags`(?, ?, ?)";

  if (stmt_execute((void **)&stmt, sql, pbind, 3, true)) {
    if (stmt != NULL) mysql_stmt_close((MYSQL_STMT *)stmt);
  }
}
