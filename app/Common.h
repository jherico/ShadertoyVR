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

#pragma once

#define USE_RIFT 0

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cinttypes>
#include <cmath>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <thread>
#include <unordered_map>

#define OGLPLUS_USE_GLEW 1
#define OGLPLUS_USE_GLCOREARB_H 0
#define OGLPLUS_USE_BOOST_CONFIG 1
#define OGLPLUS_NO_SITE_CONFIG
#define GLEW_STATIC
#include <oglplus/gl.hpp>
#pragma warning(disable : 4068)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
#pragma warning( disable : 4244 4267 4065 4101)
#include <oglplus/all.hpp>
#include <oglplus/interop/glm.hpp>
#include <oglplus/bound/texture.hpp>
#include <oglplus/bound/framebuffer.hpp>
#include <oglplus/bound/renderbuffer.hpp>
#include <oglplus/shapes/wrapper.hpp>
#pragma warning( default : 4244 4267 4065 4101)
#pragma clang diagnostic pop

#include <QtCore>
#include <QUrl>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/norm.hpp>

using glm::ivec3;
using glm::ivec2;
using glm::uvec2;
using glm::mat3;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::quat;

inline float aspect(const glm::vec2 & v) {
  return (float)v.x / (float)v.y;
}

typedef std::function<void()> Lambda;
typedef std::list<Lambda> LambdaList;

class Finally {
private:
  Lambda function;

public:
  Finally(Lambda function)
    : function(function) {
  }

  virtual ~Finally() {
    function();
  }
};

#include "Utils.h"

#include "rendering/Lights.h"
#include "rendering/MatrixStack.h"
#include "rendering/State.h"
#include "rendering/Colors.h"
#include "rendering/Vectors.h"
#include "rendering/Interaction.h"

#include "opengl/Constants.h"
#include "opengl/Textures.h"
#include "opengl/Shaders.h"
#include "opengl/Framebuffer.h"
#include "opengl/GlUtils.h"

#if defined(OS_WIN)
#define OVR_OS_WIN32
#elif defined(OS_OSX)
#define OVR_OS_MAC
#elif defined(OS_LINUX)
#define OVR_OS_LINUX
#endif

#if USE_RIFT
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>
#include "ovr/OvrUtils.h"
#include "ovr/RiftRenderingApp.h"
#endif

#include "qt/QtUtils.h"
#include "qt/QRiftWindow.h"
#include "qt/GlslEditor.h"

#ifndef PI
#define PI 3.14159265f
#endif

#ifndef HALF_PI
#define HALF_PI (PI / 2.0f)
#endif

#ifndef TWO_PI
#define TWO_PI (PI * 2.0f)
#endif

#ifndef RADIANS_TO_DEGREES
#define RADIANS_TO_DEGREES (180.0f / PI)
#endif

#ifndef DEGREES_TO_RADIANS
#define DEGREES_TO_RADIANS (PI / 180.0f)
#endif

class Platform {
  Platform() {}
public:
  static void addShutdownHook(Lambda lambda);
  static void runShutdownHooks();
};

// Combine some macros together to create a single macro
// to run an example app
#define RUN_APP(AppClass) \
    MAIN_DECL { \
        try { \
            return AppClass().run(); \
        } catch (std::exception & error) { \
            SAY_ERR(error.what()); \
        } catch (const std::string & error) { \
            SAY_ERR(error.c_str()); \
        } \
        return -1; \
    }

