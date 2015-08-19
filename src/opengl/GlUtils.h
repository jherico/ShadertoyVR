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

typedef std::shared_ptr<oglplus::shapes::ShapeWrapper> ShapeWrapperPtr;
typedef std::shared_ptr<oglplus::Buffer> BufferPtr;
typedef std::shared_ptr<oglplus::VertexArray> VertexArrayPtr;

namespace oria {
  inline void viewport(const uvec2 & size, const ivec2& position = ivec2(0)) {
    oglplus::Context::Viewport(position.x, position.y, size.x, size.y);
  }

  ShapeWrapperPtr loadSphere(const std::initializer_list<const GLchar*>& names, ProgramPtr program);
  ShapeWrapperPtr loadSkybox(ProgramPtr program);
  ShapeWrapperPtr loadPlane(ProgramPtr program, float aspect);
  void renderGeometry(ShapeWrapperPtr & shape, ProgramPtr & program);
  void renderGeometry(ShapeWrapperPtr & shape, ProgramPtr & program, const std::list<std::function<void()>> & list);
  void renderGeometry(ShapeWrapperPtr & shape, ProgramPtr & program, std::function<void()> lambda);
  void renderCube(const glm::vec3 & color = Colors::white);
  void renderColorCube();
}

