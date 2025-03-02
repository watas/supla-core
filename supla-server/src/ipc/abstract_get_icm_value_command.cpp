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

#include <ipc/abstract_get_icm_value_command.h>
#include <string.h>

#include "ipc/abstract_ipc_ctrl.h"
#include "tools.h"

using std::string;

supla_abstract_get_icm_value_command::supla_abstract_get_icm_value_command(
    supla_abstract_ipc_socket_adapter *socket_adapter)
    : supla_abstract_ipc_command(socket_adapter) {}

const string supla_abstract_get_icm_value_command::get_command_name(void) {
  return "GET-IC-VALUE:";
}

void supla_abstract_get_icm_value_command::on_command_match(
    const char *params) {
  process_parameters(
      params, [this](int user_id, int device_id, int channel_id) -> bool {
        supla_channel_ic_measurement *icm =
            get_ic_measurement(user_id, device_id, channel_id);
        if (!icm) {
          return false;
        }

        char *buffer = NULL;
        size_t size = 0;

        char *unit_b64 = st_openssl_base64_encode(
            (char *)icm->getCustomUnit(), strnlen(icm->getCustomUnit(), 9));

        for (char a = 0; a < 2; a++) {
          int n = snprintf(buffer, size, "VALUE:%i,%i,%i,%llu,%lld,%s,%s",
                           icm->getTotalCost(), icm->getPricePerUnit(),
                           icm->getImpulsesPerUnit(), icm->getCounter(),
                           icm->getCalculatedValue(), icm->getCurrency(),
                           unit_b64 ? unit_b64 : "");

          if (a == 0) {
            if (n > -1) { /* glibc 2.1 */
              size = n + 1;
            } else { /* glibc 2.0 */
              size = IPC_BUFFER_MAX_SIZE;
            }

            if (size) {
              buffer = (char *)malloc(size);
              memset(buffer, 0, size);
            }
          }
        }

        if (unit_b64) {
          free(unit_b64);
          unit_b64 = NULL;
        }

        delete icm;

        if (buffer) {
          send_result(buffer);
          free(buffer);
          return true;
        }

        return false;
      });
}
