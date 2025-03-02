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

#ifndef SUPLA_ABSTRACT_CONNECTION_OBJECT_H_
#define SUPLA_ABSTRACT_CONNECTION_OBJECT_H_

#include <memory>

#include "conn/connection.h"
#include "proto.h"

class supla_user;
class database;
class supla_abstract_srpc_call_handler_collection;
class supla_abstract_connection_object {
 private:
  struct timeval last_activity_time;
  char guid[SUPLA_GUID_SIZE];
  char authkey[SUPLA_AUTHKEY_SIZE];
  supla_connection *conn;
  int ID;
  supla_user *user;
  bool registered;
  void *lck;

 protected:
  // Thread safe start
  void lock(void);
  void unlock(void);
  void set_guid(const char GUID[SUPLA_GUID_SIZE]);
  void set_authkey(const char AuthKey[SUPLA_AUTHKEY_SIZE]);
  void set_id(int ID);
  void set_user(supla_user *user);
  void set_registered(bool registered);
  // Thread safe end

  virtual bool can_reconnect(void);

 public:
  explicit supla_abstract_connection_object(supla_connection *conn);
  virtual ~supla_abstract_connection_object();
  virtual unsigned _supla_int64_t wait_time_usec(void);
  supla_connection *get_connection(void);
  std::shared_ptr<supla_abstract_connection_object> get_shared_ptr(void);
  virtual supla_abstract_srpc_call_handler_collection *
  get_srpc_call_handler_collection(void) = 0;
  virtual bool is_sleeping_object(void) = 0;
  virtual unsigned int get_time_to_wakeup_msec(void) = 0;

  // Thread safe start
  bool is_registered(void);
  void terminate(void);
  bool reconnect(void);

  void get_guid(char guid[SUPLA_GUID_SIZE]);
  void get_authkey(char authkey[SUPLA_AUTHKEY_SIZE]);
  int get_id(void);
  int get_user_id(void);
  supla_user *get_user(void);
  bool guid_equal(const char guid1[SUPLA_GUID_SIZE]);
  void update_last_activity_time(void);
  int get_activity_delay(void);
  unsigned char get_protocol_version(void);
  // Thread safe end
};

#endif /* SUPLA_ABSTRACT_CONNECTION_OBJECT_H_ */
