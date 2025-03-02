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

#include "amazon/alexaresponserequest.h"

#include <unistd.h>

#include "http/httprequestqueue.h"
#include "http/httprequestvoiceassistantextraparams.h"
#include "log.h"
#include "sthread.h"
#include "svrcfg.h"
#include "user/user.h"

supla_alexa_response_request::supla_alexa_response_request(
    supla_user *user, int ClassID, int DeviceId, int ChannelId,
    event_type EventType, const supla_caller &Caller)
    : supla_alexa_request(user, ClassID, DeviceId, ChannelId, EventType,
                          Caller) {
  setDelay(1000000);
  setTimeout(scfg_int(CFG_ALEXA_RESPONSE_TIMEOUT) * 1000);
}

supla_alexa_response_request::~supla_alexa_response_request() {}

bool supla_alexa_response_request::verifyExisting(
    supla_http_request *existing) {
  if (getCaller() == ctDevice) {
    existing->setDelay(0);
    return true;
  }
  return false;
}

bool supla_alexa_response_request::queueUp(void) {
  return getCaller() == ctAmazonAlexa && supla_alexa_request::queueUp();
}

bool supla_alexa_response_request::isCallerAccepted(const supla_caller &caller,
                                                    bool verification) {
  if (!supla_alexa_request::isCallerAccepted(caller, verification)) {
    return false;
  }

  channel_complex_value summary =
      getUser()->get_channel_complex_value(getChannelId());

  switch (summary.function) {
    case SUPLA_CHANNELFNC_POWERSWITCH:
    case SUPLA_CHANNELFNC_LIGHTSWITCH:
    case SUPLA_CHANNELFNC_STAIRCASETIMER:
    case SUPLA_CHANNELFNC_DIMMER:
    case SUPLA_CHANNELFNC_RGBLIGHTING:
    case SUPLA_CHANNELFNC_DIMMERANDRGBLIGHTING:
    case SUPLA_CHANNELFNC_CONTROLLINGTHEROLLERSHUTTER:
      if (verification) {
        if (caller == ctDevice || caller == ctAmazonAlexa) {
          return !summary.hidden_channel;
        }
      } else if (caller == ctAmazonAlexa) {
        return !summary.hidden_channel;
      }
      break;
    default:
      return false;
  }

  return false;
}

void supla_alexa_response_request::execute(void *sthread) {
  channel_complex_value value =
      getUser()->get_channel_complex_value(getChannelId());

  char *correlationToken = NULL;
  int subChannel = 0;

  accessExtraParams([this, &correlationToken, &subChannel](
                        supla_http_request_extra_params *_params) -> void {
    supla_http_request_voice_assistant_extra_params *params =
        dynamic_cast<supla_http_request_voice_assistant_extra_params *>(
            _params);
    if (params) {
      if (params->getCorrelationTokenPtr()) {
        correlationToken =
            strndup(params->getCorrelationTokenPtr(), CORRELATIONTOKEN_MAXSIZE);
      }
      subChannel = params->getSubChannel();
    }
  });

  switch (value.function) {
    case SUPLA_CHANNELFNC_POWERSWITCH:
    case SUPLA_CHANNELFNC_LIGHTSWITCH:
    case SUPLA_CHANNELFNC_STAIRCASETIMER:
      getClient()->powerControllerSendResponse(correlationToken, getChannelId(),
                                               value.hi, value.online);
      break;
    case SUPLA_CHANNELFNC_DIMMER:
      getClient()->brightnessControllerSendResponse(
          correlationToken, getChannelId(), value.brightness, value.online, 0);
      break;
    case SUPLA_CHANNELFNC_RGBLIGHTING:
      getClient()->colorControllerSendResponse(
          correlationToken, getChannelId(), value.color, value.color_brightness,
          value.online, 0);
      break;
    case SUPLA_CHANNELFNC_DIMMERANDRGBLIGHTING:
      if (subChannel == 1) {
        getClient()->colorControllerSendResponse(
            correlationToken, getChannelId(), value.color,
            value.color_brightness, value.online, 1);
      } else if (subChannel == 2) {
        getClient()->brightnessControllerSendResponse(
            correlationToken, getChannelId(), value.brightness, value.online,
            2);
      }
      break;
    case SUPLA_CHANNELFNC_CONTROLLINGTHEROLLERSHUTTER:
      getClient()->percentageControllerSendResponse(
          correlationToken, getChannelId(), value.shut, value.online);
      break;
  }

  if (correlationToken) {
    free(correlationToken);
  }
}

REGISTER_HTTP_REQUEST_CLASS(supla_alexa_response_request);
