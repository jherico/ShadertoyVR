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

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cinttypes>
#include <cmath>
#include <functional>
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
#include <vector>

#include <QtCore/QtGlobal>

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMap>
#include <QtCore/QMutex>
#include <QtCore/QQueue>
#include <QtCore/QSharedPointer>
#include <QtCore/QStandardPaths>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtCore/QVariantAnimation>

#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QWindow>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>

#include <QtXml/QDomDocument>

#include <QtXmlPatterns/QXmlQuery>

#pragma warning(disable : 4244)
#include <QtGui/qevent.h>
#pragma warning(default : 4244)

#include <GL/glew.h>
#define OGLPLUS_USE_GLEW 1
#define OGLPLUS_USE_GLCOREARB_H 0
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

#ifndef NDEBUG
#define DEBUG
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/norm.hpp>

using glm::ivec2;
using glm::ivec3;
using glm::ivec4;
using glm::uvec2;
using glm::uvec3;
using glm::uvec4;
using glm::mat3;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::quat;

inline float aspect(const glm::vec2 & v) {
  return (float)v.x / (float)v.y;
}


namespace Resource {
  extern const QString IMAGES_CUBE_TEXTURE_PNG;
  extern const QString IMAGES_FLOOR_PNG;

  extern const QString SHADERS_SIMPLE_VS;
  extern const QString SHADERS_COLORED_FS;
  extern const QString SHADERS_TEXTURED_FS;
  extern const QString SHADERS_CUBEMAP_VS;
  extern const QString SHADERS_CUBEMAP_FS;
  extern const QString SHADERS_COLORCUBE_VS;
  extern const QString SHADERS_COLORCUBE_FS;
  extern const QString MISC_GLSL_XML;
  extern const QString& SHADERS_TEXTURED_VS;
};
//using Resource = std::string;

#include "Lambdas.h"
#include "Platform.h"
#include "Utils.h"

#include "rendering/MatrixStack.h"
#include "rendering/State.h"
#include "rendering/Colors.h"
#include "rendering/Vectors.h"

#include "opengl/Constants.h"
#include "opengl/Textures.h"
#include "opengl/Shaders.h"
#include "opengl/Framebuffer.h"
#include "opengl/GlUtils.h"

#ifndef TAU
#define TAU 6.28318530718f
#endif

#ifndef HALF_TAU
#define HALF_TAU (TAU / 2.0f)
#endif

#ifndef QUARTER_TAU
#define QUARTER_TAU (TAU / 4.0f)
#endif

// Windows has a non-standard main function prototype
#ifdef Q_OS_WIN
#define MAIN_DECL int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
#define MAIN_DECL int main(int argc, char ** argv)
#endif

// Combine some macros together to create a single macro
// to run an example app
#define RUN_APP(AppClass) \
    MAIN_DECL { \
        try { \
            return AppClass().run(); \
        } catch (std::exception & error) { \
            qWarning() << error.what(); \
        } catch (const std::string & error) { \
            qWarning() << error.c_str(); \
        } \
        return -1; \
    }
