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

#ifndef SUPLA_GET_ICM_VALUE_COMMAND_H_
#define SUPLA_GET_ICM_VALUE_COMMAND_H_

#include <string>

#include "ipc/abstract_get_icm_value_command.h"

class supla_get_icm_value_command
    : public supla_abstract_get_icm_value_command {
 protected:
  virtual supla_channel_ic_measurement *get_ic_measurement(int user_id,
                                                           int device_id,
                                                           int channel_id);

 public:
  explicit supla_get_icm_value_command(
      supla_abstract_ipc_socket_adapter *socket_adapter);
};

#endif /* SUPLA_GET_ICM_VALUE_COMMAND_H_ */
