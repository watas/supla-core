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

#include "ValueGetterStub.h"

#include <string.h>

namespace testing {
ValueGetterStub::ValueGetterStub(void) : supla_abstract_value_getter() {
  result = NULL;
}

ValueGetterStub::~ValueGetterStub(void) {}

supla_channel_value* ValueGetterStub::_get_value(int user_id, int device_id,
                                                 int channel_id) {
  return result;
}

void ValueGetterStub::setResult(supla_channel_value* result) {
  this->result = result;
}

}  // namespace testing
