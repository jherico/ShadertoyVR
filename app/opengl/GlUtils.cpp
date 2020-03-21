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

#pragma warning( disable : 4068 4244 4267 4065 4101 4244)
#include <oglplus/bound/buffer.hpp>
#include <oglplus/shapes/cube.hpp>
#include <oglplus/shapes/sky_box.hpp>
#include <oglplus/shapes/plane.hpp>
#include <oglplus/shapes/vector.hpp>
#include <oglplus/opt/list_init.hpp>
#include <oglplus/opt/list_init.hpp>
#include <oglplus/math/matrix.hpp>

#pragma warning( default : 4068 4244 4267 4065 4101)

typedef oglplus::Uniform<oglplus::Mat4f> Mat44Uniform;

oglplus::Mat4f fromGlm(const mat4 & m) {
  return oglplus::Mat4f(&(glm::transpose(m)[0][0]), 16);
}

namespace oria {

template <typename Iter>
  void renderGeometryWithLambdas(ShapeWrapperPtr & shape, ProgramPtr & program, Iter begin, const Iter & end) {
    program->Use();
    Mat44Uniform(*program, "ModelView").Set(fromGlm(Stacks::modelview().top()));
    Mat44Uniform(*program, "Projection").Set(fromGlm(Stacks::projection().top()));

    std::for_each(begin, end, [&](const Lambda & f){ f(); });

    shape->Use();
    shape->Draw();

    oglplus::NoProgram().Bind();
    oglplus::NoVertexArray().Bind();
  }

  void renderGeometry(ShapeWrapperPtr & shape, ProgramPtr & program, const Lambda & lambda) {
    renderGeometry(shape, program, LambdaList({ lambda }) );
  }

  void renderGeometry(ShapeWrapperPtr & shape, ProgramPtr & program, const LambdaList & list) {
    renderGeometryWithLambdas(shape, program, list.begin(), list.end());
  }

  void renderGeometry(ShapeWrapperPtr & shape, ProgramPtr & program) {
    static const std::list<std::function<void()>> EMPTY_LIST;
    renderGeometryWithLambdas(shape, program, EMPTY_LIST.begin(), EMPTY_LIST.end());
  }


  ShapeWrapperPtr loadSkybox(ProgramPtr program) {
    using namespace oglplus;
    ShapeWrapperPtr shape = ShapeWrapperPtr(
      new shapes::ShapeWrapper(List("Position").Get(),
        shapes::SkyBox(), *program));
    return shape;
  }


  ShapeWrapperPtr loadPlane(ProgramPtr program, float aspect) {
    using namespace oglplus;
    Vec3f a(1, 0, 0);
    Vec3f b(0, 1, 0);
    if (aspect > 1) {
      b[1] /= aspect;
    } else {
      a[0] *= aspect;
    }
    ShapeWrapperPtr shape = ShapeWrapperPtr(
      new shapes::ShapeWrapper(List("Position")("TexCoord").Get(),
        shapes::Plane(a, b), *program));
    return shape;
  }

  void GL_CALLBACK debugCallback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar * message,
    void * userParam) {
    const char * typeStr = "?";
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
      typeStr = "ERROR";
      break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      typeStr = "DEPRECATED_BEHAVIOR";
      break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      typeStr = "UNDEFINED_BEHAVIOR";
      break;
    case GL_DEBUG_TYPE_PORTABILITY:
      typeStr = "PORTABILITY";
      break;
    case GL_DEBUG_TYPE_PERFORMANCE:
      typeStr = "PERFORMANCE";
      break;
    case GL_DEBUG_TYPE_OTHER:
      typeStr = "OTHER";
      break;
    }

    const char * severityStr = "?";
    auto logger = qDebug();
    switch (severity) {
    case GL_DEBUG_SEVERITY_LOW:
      severityStr = "LOW";
      break;

    case GL_DEBUG_SEVERITY_MEDIUM:
      severityStr = "MEDIUM";
      logger = qWarning();
      break;

    case GL_DEBUG_SEVERITY_HIGH:
      severityStr = "HIGH";
      logger = qWarning();
      break;
    }

    logger << "--- OpenGL Callback Message ---";
    logger << QString().sprintf("type: %s\nseverity: %-8s\nid: %d\nmsg: %s", typeStr, severityStr, id,
      message);
    logger << "--- OpenGL Callback Message ---";
  }

}

