/************************************************************************************

Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
Copyright   :   Copyright Bradley Austin Davis. All Rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

************************************************************************************/
#pragma once
#include "Common.h"

class st {
  st(){}
public:
  static const int MAX_CHANNELS = 4;
  enum class ChannelInputType {
    TEXTURE, CUBEMAP, AUDIO, VIDEO,
  };

  static const char * UNIFORM_RESOLUTION;
  static const char * UNIFORM_GLOBALTIME;
  static const char * UNIFORM_CHANNEL_TIME;
  static const char * UNIFORM_MOUSE_COORDS;
  static const char * UNIFORM_DATE;
  static const char * UNIFORM_SAMPLE_RATE;
  static const char * UNIFORM_POSITION;
  static const char * UNIFORM_CHANNEL_RESOLUTIONS[4];
  static const char * UNIFORM_CHANNELS[4];
  static const char * SHADER_HEADER;
  static const char * LINE_NUMBER_HEADER;
  static const QStringList TEXTURES;
  static const QStringList CUBEMAPS;

  struct Shader {
    QString id;
    QString url;
    QString name;
    QString fragmentSource;
    ChannelInputType channelTypes[MAX_CHANNELS];
    QString channelTextures[MAX_CHANNELS];
  };
};

