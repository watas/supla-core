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

#include "clientchannel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <memory>

#include "client.h"
#include "db/database.h"
#include "log.h"
#include "proto.h"
#include "safearray.h"
#include "srpc/srpc.h"
#include "user.h"

using std::shared_ptr;

supla_client_channel::supla_client_channel(
    supla_client_channels *Container, int Id, int DeviceId, int LocationID,
    int Type, int Func, int Param1, int Param2, int Param3, int Param4,
    char *TextParam1, char *TextParam2, char *TextParam3, const char *Caption,
    int AltIcon, int UserIcon, short ManufacturerID, short ProductID,
    unsigned char ProtocolVersion, int Flags,
    const char value[SUPLA_CHANNELVALUE_SIZE],
    unsigned _supla_int_t validity_time_sec)
    : supla_client_objcontainer_item(Container, Id, Caption) {
  this->DeviceId = DeviceId;
  this->LocationId = LocationID;
  this->Type = Type;
  this->Func = Func;
  this->Param1 = Param1;
  this->Param2 = Param2;
  this->Param3 = Param3;
  this->Param4 = Param4;
  this->TextParam1 = TextParam1 ? strndup(TextParam1, 255) : NULL;
  this->TextParam2 = TextParam2 ? strndup(TextParam2, 255) : NULL;
  this->TextParam3 = TextParam3 ? strndup(TextParam3, 255) : NULL;
  this->AltIcon = AltIcon;
  this->UserIcon = UserIcon;
  this->ManufacturerID = ManufacturerID;
  this->ProductID = ProductID;
  this->ProtocolVersion = ProtocolVersion;
  this->Flags = Flags;
  setValueValidityTimeSec(validity_time_sec);

  memcpy(this->value, value, SUPLA_CHANNELVALUE_SIZE);
}

supla_client_channel::~supla_client_channel(void) {
  if (this->TextParam1) {
    free(this->TextParam1);
    this->TextParam1 = NULL;
  }

  if (this->TextParam2) {
    free(this->TextParam2);
    this->TextParam2 = NULL;
  }

  if (this->TextParam3) {
    free(this->TextParam3);
    this->TextParam3 = NULL;
  }
}

int supla_client_channel::getDeviceId() { return DeviceId; }

int supla_client_channel::getExtraId() { return DeviceId; }

int supla_client_channel::getType() { return Type; }

int supla_client_channel::getFunc() { return Func; }

void supla_client_channel::setFunc(int Func) {
  if (Func != this->Func) {
    this->Func = Func;
    mark_for_remote_update(OI_REMOTEUPDATE_DATA1);
  }
}

void supla_client_channel::setCaption(const char *Caption) {
  if ((Caption == NULL && getCaption() != NULL) ||
      (Caption != NULL && getCaption() == NULL) ||
      strncmp(Caption, getCaption(), SUPLA_CHANNEL_CAPTION_MAXSIZE) != 0) {
    supla_client_objcontainer_item::setCaption(Caption);
    mark_for_remote_update(OI_REMOTEUPDATE_DATA1);
  }
}

short supla_client_channel::getManufacturerID() { return ManufacturerID; }

short supla_client_channel::getProductID() { return ProductID; }

int supla_client_channel::getFlags() { return Flags; }

void supla_client_channel::setValueValidityTimeSec(
    unsigned _supla_int_t validity_time_sec) {
  resetValueValidityTime();

  if (validity_time_sec > 0) {
    gettimeofday(&value_valid_to, NULL);
    value_valid_to.tv_sec += validity_time_sec;
  }
}

bool supla_client_channel::isValueValidityTimeSet() {
  return value_valid_to.tv_sec || value_valid_to.tv_usec;
}

unsigned _supla_int64_t supla_client_channel::getValueValidityTimeUSec(void) {
  if (isValueValidityTimeSet()) {
    struct timeval now;
    gettimeofday(&now, NULL);

    _supla_int64_t result =
        (value_valid_to.tv_sec * 1000000 + value_valid_to.tv_usec) -
        (now.tv_sec * 1000000 + now.tv_usec);
    if (result > 0) {
      return result;
    }
  }

  return 0;
}

void supla_client_channel::resetValueValidityTime(void) {
  value_valid_to.tv_sec = 0;
  value_valid_to.tv_usec = 0;
}

bool supla_client_channel::remote_update_is_possible(void) {
  switch (Func) {
    case SUPLA_CHANNELFNC_CONTROLLINGTHEDOORLOCK:
    case SUPLA_CHANNELFNC_CONTROLLINGTHEGATEWAYLOCK:
    case SUPLA_CHANNELFNC_CONTROLLINGTHEGATE:
    case SUPLA_CHANNELFNC_CONTROLLINGTHEGARAGEDOOR:
    case SUPLA_CHANNELFNC_THERMOMETER:
    case SUPLA_CHANNELFNC_HUMIDITY:
    case SUPLA_CHANNELFNC_HUMIDITYANDTEMPERATURE:
    case SUPLA_CHANNELFNC_NOLIQUIDSENSOR:
    case SUPLA_CHANNELFNC_CONTROLLINGTHEROLLERSHUTTER:
    case SUPLA_CHANNELFNC_CONTROLLINGTHEROOFWINDOW:
    case SUPLA_CHANNELFNC_POWERSWITCH:
    case SUPLA_CHANNELFNC_LIGHTSWITCH:
    case SUPLA_CHANNELFNC_DIMMER:
    case SUPLA_CHANNELFNC_RGBLIGHTING:
    case SUPLA_CHANNELFNC_DIMMERANDRGBLIGHTING:
    case SUPLA_CHANNELFNC_DEPTHSENSOR:
    case SUPLA_CHANNELFNC_DISTANCESENSOR:
    case SUPLA_CHANNELFNC_MAILSENSOR:
    case SUPLA_CHANNELFNC_WINDSENSOR:
    case SUPLA_CHANNELFNC_PRESSURESENSOR:
    case SUPLA_CHANNELFNC_RAINSENSOR:
    case SUPLA_CHANNELFNC_WEIGHTSENSOR:
    case SUPLA_CHANNELFNC_WEATHER_STATION:
    case SUPLA_CHANNELFNC_STAIRCASETIMER:
    case SUPLA_CHANNELFNC_THERMOSTAT:
    case SUPLA_CHANNELFNC_THERMOSTAT_HEATPOL_HOMEPLUS:
    case SUPLA_CHANNELFNC_VALVE_OPENCLOSE:
    case SUPLA_CHANNELFNC_VALVE_PERCENTAGE:
    case SUPLA_CHANNELFNC_DIGIGLASS_HORIZONTAL:
    case SUPLA_CHANNELFNC_DIGIGLASS_VERTICAL:
      return true;

    case SUPLA_CHANNELFNC_OPENINGSENSOR_GATEWAY:
    case SUPLA_CHANNELFNC_OPENINGSENSOR_GATE:
    case SUPLA_CHANNELFNC_OPENINGSENSOR_GARAGEDOOR:
    case SUPLA_CHANNELFNC_OPENINGSENSOR_DOOR:
    case SUPLA_CHANNELFNC_OPENINGSENSOR_ROLLERSHUTTER:
    case SUPLA_CHANNELFNC_OPENINGSENSOR_ROOFWINDOW:
    case SUPLA_CHANNELFNC_OPENINGSENSOR_WINDOW:

      if (Param1 == 0 && Param2 == 0) {
        return true;
      }
      break;

    case SUPLA_CHANNELFNC_ELECTRICITY_METER:
    case SUPLA_CHANNELFNC_IC_ELECTRICITY_METER:
    case SUPLA_CHANNELFNC_IC_GAS_METER:
    case SUPLA_CHANNELFNC_IC_WATER_METER:
    case SUPLA_CHANNELFNC_IC_HEAT_METER:

      if (Param4 == 0) {
        return true;
      }
      break;
  }

  return Type == SUPLA_CHANNELTYPE_BRIDGE && Func == 0;
}

void supla_client_channel::proto_get_value(TSuplaChannelValue_B *value,
                                           char *online, supla_client *client) {
  bool result = false;

  if (client && client->get_user()) {
    unsigned _supla_int_t validity_time_sec = 0;
    result = client->get_user()->get_channel_value(
        DeviceId, getId(), value->value, value->sub_value,
        &value->sub_value_type, nullptr, nullptr, online, &validity_time_sec,
        true);
    if (result) {
      setValueValidityTimeSec(validity_time_sec);
    }
  }

  if ((!result || (online && !(*online))) && isValueValidityTimeSet() &&
      getValueValidityTimeUSec() > 0) {
    result = true;
    if (online) {
      *online = true;
    }
    memcpy(value->value, this->value, SUPLA_CHANNELVALUE_SIZE);
  }
}

void supla_client_channel::proto_get_value(TSuplaChannelValue *value,
                                           char *online, supla_client *client) {
  TSuplaChannelValue_B value_b = {};
  proto_get_value(&value_b, online, client);

  memcpy(value->value, value_b.value, SUPLA_CHANNELVALUE_SIZE);
  memcpy(value->sub_value, value_b.sub_value, SUPLA_CHANNELVALUE_SIZE);
}

void supla_client_channel::proto_get(TSC_SuplaChannel *channel,
                                     supla_client *client) {
  memset(channel, 0, sizeof(TSC_SuplaChannel));

  channel->Id = getId();
  channel->Func = Func;
  channel->LocationID = this->LocationId;

  proto_get_value(&channel->value, &channel->online, client);
  sproto_set_null_terminated_string(getCaption(), channel->Caption,
                                    &channel->CaptionSize,
                                    SUPLA_CHANNEL_CAPTION_MAXSIZE);
}

void supla_client_channel::proto_get(TSC_SuplaChannel_B *channel,
                                     supla_client *client) {
  memset(channel, 0, sizeof(TSC_SuplaChannel_B));

  channel->Id = getId();
  channel->Func = Func;
  channel->LocationID = this->LocationId;
  channel->AltIcon = this->AltIcon;
  channel->ProtocolVersion = this->ProtocolVersion;
  channel->Flags = this->Flags;

  proto_get_value(&channel->value, &channel->online, client);
  sproto_set_null_terminated_string(getCaption(), channel->Caption,
                                    &channel->CaptionSize,
                                    SUPLA_CHANNEL_CAPTION_MAXSIZE);
}

void supla_client_channel::proto_get(TSC_SuplaChannel_C *channel,
                                     supla_client *client) {
  memset(channel, 0, sizeof(TSC_SuplaChannel_C));

  channel->Id = getId();
  channel->DeviceID = getDeviceId();
  channel->Type = this->Type;
  channel->Func = Func;
  channel->LocationID = this->LocationId;
  channel->AltIcon = this->AltIcon;
  channel->UserIcon = this->UserIcon;
  channel->ManufacturerID = this->ManufacturerID;
  channel->ProductID = this->ProductID;
  channel->ProtocolVersion = this->ProtocolVersion;
  channel->Flags = this->Flags;

  proto_get_value(&channel->value, &channel->online, client);
  sproto_set_null_terminated_string(getCaption(), channel->Caption,
                                    &channel->CaptionSize,
                                    SUPLA_CHANNEL_CAPTION_MAXSIZE);
}

void supla_client_channel::proto_get(TSC_SuplaChannel_D *channel,
                                     supla_client *client) {
  memset(channel, 0, sizeof(TSC_SuplaChannel_D));

  channel->Id = getId();
  channel->DeviceID = getDeviceId();
  channel->Type = this->Type;
  channel->Func = Func;
  channel->LocationID = this->LocationId;
  channel->AltIcon = this->AltIcon;
  channel->UserIcon = this->UserIcon;
  channel->ManufacturerID = this->ManufacturerID;
  channel->ProductID = this->ProductID;
  channel->ProtocolVersion = this->ProtocolVersion;
  channel->Flags = this->Flags;

  proto_get_value(&channel->value, &channel->online, client);
  sproto_set_null_terminated_string(getCaption(), channel->Caption,
                                    &channel->CaptionSize,
                                    SUPLA_CHANNEL_CAPTION_MAXSIZE);
}

void supla_client_channel::proto_get(TSC_SuplaChannelValue *channel_value,
                                     supla_client *client) {
  memset(channel_value, 0, sizeof(TSC_SuplaChannelValue));
  channel_value->Id = getId();
  proto_get_value(&channel_value->value, &channel_value->online, client);
}

void supla_client_channel::proto_get(TSC_SuplaChannelValue_B *channel_value,
                                     supla_client *client) {
  memset(channel_value, 0, sizeof(TSC_SuplaChannelValue_B));
  channel_value->Id = getId();
  proto_get_value(&channel_value->value, &channel_value->online, client);
}

bool supla_client_channel::proto_get(TSC_SuplaChannelExtendedValue *cev,
                                     supla_client *client) {
  if (cev == NULL) {
    return false;
  }

  memset(cev, 0, sizeof(TSC_SuplaChannelExtendedValue));

  if (client && client->get_user()) {
    bool cev_exists = false;

    int ChannelId = getId();
    shared_ptr<supla_device> device =
        client->get_user()->get_devices()->get(DeviceId);

    if (device != nullptr) {
      cev_exists =
          device->get_channels()->get_channel_extendedvalue(ChannelId, cev);
    }

    device = nullptr;
    ChannelId = 0;

    switch (getFunc()) {
      case SUPLA_CHANNELFNC_POWERSWITCH:
      case SUPLA_CHANNELFNC_LIGHTSWITCH:
        ChannelId = Param1;  // Associated measurement channel
        break;

      case SUPLA_CHANNELFNC_STAIRCASETIMER:
        ChannelId = Param2;  // Associated measurement channel
        break;
    }

    if (ChannelId) {
      device = client->get_user()->get_devices()->get(0, ChannelId);

      if (device != nullptr) {
        TSC_SuplaChannelExtendedValue second_cev = {};
        if (device->get_channels()->get_channel_extendedvalue(ChannelId,
                                                              &second_cev)) {
          if (client->get_protocol_version() >= 17) {
            srpc_evtool_value_add(&cev->value, &second_cev.value);
          } else {
            // For backward compatibility, overwrite cev->value
            memcpy(&cev->value, &second_cev.value,
                   sizeof(TSuplaChannelExtendedValue));
          }

          cev_exists = true;
        }
      }
    }

    if (cev_exists) {
      cev->Id = getId();
      return true;
    }
  }

  return false;
}

void supla_client_channel::mark_for_remote_update(int mark) {
  supla_client_objcontainer_item::mark_for_remote_update(mark);
  mark = marked_for_remote_update();
  if ((mark & OI_REMOTEUPDATE_DATA1) && (mark & OI_REMOTEUPDATE_DATA2)) {
    unmark_for_remote_update(OI_REMOTEUPDATE_DATA2);
  }
}

bool supla_client_channel::get_basic_cfg(TSC_ChannelBasicCfg *basic_cfg) {
  if (basic_cfg == NULL) return false;

  bool result = false;
  database *db = new database();
  result = db->connect() && db->get_channel_basic_cfg(getId(), basic_cfg);
  delete db;

  return result;
}
