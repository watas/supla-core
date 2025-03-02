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

#ifndef CLIENT_H_
#define CLIENT_H_

#include <conn/abstract_connection_object.h>

#include <memory>

#include "client_scene_dao.h"
#include "client_scene_remote_updater.h"
#include "client_scenes.h"
#include "clientchannelgroups.h"
#include "clientchannels.h"
#include "clientlocation.h"
#include "db/db_access_provider.h"

class supla_client_call_handler_collection;
class supla_user;
class supla_connection;
class supla_register_client;
class supla_client : public supla_abstract_connection_object {
 private:
  char name[SUPLA_CLIENT_NAME_MAXSIZE];
  bool superuser_authorized;
  int access_id;
  static supla_client_call_handler_collection call_handler_collection;
  supla_db_access_provider dba;
  supla_client_locations *locations;
  supla_client_channels *channels;
  supla_client_channelgroups *cgroups;
  supla_client_scenes *scenes;
  supla_client_scene_remote_updater *scene_remote_updater;
  supla_client_scene_dao *scene_dao;

 protected:
  friend class supla_register_client;

  void loadIODevices(void);
  void load_config(void);

  void remote_update_lists(void);
  void set_name(const char *name);
  void set_access_id(int access_id);

 public:
  explicit supla_client(supla_connection *connection);
  virtual supla_abstract_srpc_call_handler_collection *
  get_srpc_call_handler_collection(void);
  std::shared_ptr<supla_client> get_shared_ptr(void);
  supla_client_locations *get_locations(void);
  supla_client_channels *get_channels(void);
  supla_client_channelgroups *get_cgroups(void);
  supla_client_scenes *get_scenes(void);
  virtual bool is_sleeping_object(void);
  virtual unsigned int get_time_to_wakeup_msec(void);

  void iterate();
  unsigned _supla_int64_t wait_time_usec();

  void superuser_authorize(int UserID, const char Email[SUPLA_EMAIL_MAXSIZE],
                           const char Password[SUPLA_PASSWORD_MAXSIZE],
                           bool *connection_failed);
  void revoke_superuser_authorization(void);
  bool is_superuser_authorized(void);
  void update_device_channels(int LocationID, int DeviceID);
  void on_channel_value_changed(int DeviceId, int ChannelId = 0,
                                bool Extended = false);
  void get_next(void);
  int getName(char *buffer, int size);
  int getAccessID(void);

  void call_event(TSC_SuplaEvent *event);

  void on_device_calcfg_result(int ChannelID, TDS_DeviceCalCfgResult *result);
  void on_device_channel_state_result(int ChannelID, TDSC_ChannelState *state);

  void set_channel_function(int ChannelId, int Func);
  void set_channel_function_result(TSC_SetChannelFunctionResult *result);
  void set_channel_caption(int ChannelId, char *Caption);
  void set_location_caption(int LocationId, char *Caption);
  void set_scene_caption(int scene_id, char *caption);

  virtual ~supla_client();
};

#endif /* CLIENT_H_ */
