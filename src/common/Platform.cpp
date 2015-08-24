/************************************************************************************
 
 Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
 Copyright   :   Copyright Brad Davis. All Rights reserved.
 
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

#include "Common.h"

#ifdef Q_OS_WIN
#pragma warning (disable : 4996)
#include <Windows.h>
#define snprintf _snprintf
#else
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <cstdarg>
#endif

void Platform::sleepMillis(int millis) {
#ifdef Q_OS_WIN
  Sleep(millis);
#else
  usleep(millis * 1000);
#endif
}

long Platform::elapsedMillis() {
#ifdef Q_OS_WIN
  static long start = GetTickCount();
  return GetTickCount() - start;
#else
  timeval time;
  gettimeofday(&time, NULL);
  long millis = (time.tv_sec * 1000) + (time.tv_usec / 1000);
  static long start = millis;
  return millis - start;
#endif
}

float Platform::elapsedSeconds() {
  return (float)elapsedMillis() / 1000.0f;
}

typedef std::vector<std::function<void()>> VecLambda;
VecLambda & getShutdownHooks() {
  static VecLambda hooks;
  return hooks;
}

void Platform::addShutdownHook(std::function<void()> f) {
  getShutdownHooks().push_back(f);
}

void Platform::runShutdownHooks() {
  VecLambda & hooks = getShutdownHooks();
  std::for_each(hooks.begin(), hooks.end(), [&](std::function<void()> f){
    f();
  });
}

const QString Resource::IMAGES_CUBE_TEXTURE_PNG{ ":/images/valve_hmd_cube_texture.png" };
const QString Resource::IMAGES_FLOOR_PNG{ ":/images/floor.png" };
const QString Resource::SHADERS_SIMPLE_VS{ ":/shaders/Simple.vs" };
const QString Resource::SHADERS_COLORED_FS{ ":/shaders/SimpleColored.fs" };
const QString Resource::SHADERS_TEXTURED_FS{ ":/shaders/SimpleTextured.fs" };
const QString Resource::SHADERS_CUBEMAP_VS{ ":/shaders/CubeMap.vs" };
const QString Resource::SHADERS_CUBEMAP_FS{ ":/shaders/CubeMap.fs" };
const QString Resource::SHADERS_COLORCUBE_VS{ ":/shaders/ColorCube.vs" };
const QString Resource::SHADERS_COLORCUBE_FS{ ":/shaders/ColorCube.fs" };
const QString Resource::MISC_GLSL_XML{ ":/misc/glsl.xml" };
const QString& Resource::SHADERS_TEXTURED_VS = Resource::SHADERS_SIMPLE_VS;
