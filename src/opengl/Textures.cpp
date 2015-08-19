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
#include <QUrl>
struct TextureInfo {
  uvec2 size;
  TexturePtr tex;
};

typedef std::map<QUrl, TextureInfo> TextureMap;
typedef TextureMap::iterator TextureMapItr;

namespace oria {

  ImagePtr loadImage(const std::vector<uint8_t> & data, bool flip) {
    using namespace oglplus;
    std::stringstream stream(std::string((const char*)&data[0], data.size()));
    return ImagePtr();
  }

  TextureMap & getTextureMap() {
    static TextureMap map;
    static bool registeredShutdown = false;
    if (!registeredShutdown) {
      Platform::addShutdownHook([&]{
        map.clear();
      });
      registeredShutdown = true;
    }

    return map;
  }

  template <typename T, typename F>
  T loadOrPopulate(std::map<Resource, T> & map, Resource resource, F loader) {
    if (!map.count(resource)) {
      map[resource] = loader();
    }
    return map[resource];
  }

  TexturePtr load2dTextureFromPngData(std::vector<uint8_t> & data) {
    using namespace oglplus;
    TexturePtr texture(new Texture());
    Context::Bound(TextureTarget::_2D, *texture)
      .MagFilter(TextureMagFilter::Linear)
      .MinFilter(TextureMinFilter::Linear);
    ImagePtr image = loadImage(data);
    // FIXME detect alignment properly, test on both OpenCV and LibPNG
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    Texture::Image2D(TextureTarget::_2D, *image);
    return texture;
  }

  TextureInfo load2dTextureInternal(const std::vector<uint8_t> & data) {
    using namespace oglplus;
    TextureInfo result;
    result.tex = TexturePtr(new Texture());
    Context::Bound(TextureTarget::_2D, *result.tex)
      .MagFilter(TextureMagFilter::Linear)
      .MinFilter(TextureMinFilter::Linear);
    ImagePtr image = loadImage(data);
    result.size.x = image->Width();
    result.size.y = image->Height();
    // FIXME detect alignment properly, test on both OpenCV and LibPNG
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    Texture::Image2D(TextureTarget::_2D, *image);
    return result;
  }

  TexturePtr load2dTexture(const std::vector<uint8_t> & data, uvec2 & outSize) {
    TextureInfo texInfo = load2dTextureInternal(data);
    outSize = texInfo.size;
    return texInfo.tex;
  }

  TexturePtr load2dTexture(const std::vector<uint8_t> & data) {
    uvec2 size;
    return load2dTexture(data, size);
  }

  TexturePtr loadCubemapTexture(std::function<ImagePtr(int)> dataLoader) {
    using namespace oglplus;
    TexturePtr result = TexturePtr(new Texture());
    Context::Bound(TextureTarget::CubeMap, *result)
      .MagFilter(TextureMagFilter::Linear)
      .MinFilter(TextureMinFilter::Linear)
      .WrapS(TextureWrap::ClampToEdge)
      .WrapT(TextureWrap::ClampToEdge)
      .WrapR(TextureWrap::ClampToEdge);

    glm::uvec2 size;
    for (int i = 0; i < 6; ++i) {
      ImagePtr image = dataLoader(i);
      if (!image) {
        continue;
      }
      Texture::Image2D(Texture::CubeMapFace(i), *image);
    }
    return result;
  }
}
