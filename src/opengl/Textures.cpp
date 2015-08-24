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
struct TextureInfo {
    uvec2 size;
    TexturePtr tex;
};

typedef std::map<QUrl, TextureInfo> TextureMap;
typedef TextureMap::iterator TextureMapItr;

namespace oria {

    ImagePtr loadImage(const QString& path, bool flip) {
        QImage image = QImage(path).mirrored(false, true);
        using namespace oglplus;
        size_t width = image.width();
        size_t height = image.height();
        PixelDataFormat format = PixelDataFormat::RGB;
        if (image.hasAlphaChannel()) {
            format = PixelDataFormat::RGBA;
        }
        return ImagePtr(new oglplus::images::Image(width, height, 1, 3, 
            image.constBits(), format, PixelDataInternalFormat::RGBA8));
    }

    TextureMap & getTextureMap() {
        static TextureMap map;
        static bool registeredShutdown = false;
        if (!registeredShutdown) {
            Platform::addShutdownHook([&] {
                map.clear();
            });
            registeredShutdown = true;
        }

        return map;
    }

    template <typename T, typename F>
    T loadOrPopulate(std::map<QString, T> & map, const QString& resource, F loader) {
        if (!map.count(resource)) {
            map[resource] = loader();
        }
        return map[resource];
    }

    //TextureInfo load2dTextureInternal(const std::vector<uint8_t> & data) {
    //  using namespace oglplus;
    //  TextureInfo result;
    //  result.tex = TexturePtr(new Texture());
    //  Context::Bound(TextureTarget::_2D, *result.tex)
    //    .MagFilter(TextureMagFilter::Linear)
    //    .MinFilter(TextureMinFilter::Linear);
    //  ImagePtr image = loadImage(data);
    //  result.size.x = image->Width();
    //  result.size.y = image->Height();
    //  // FIXME detect alignment properly, test on both OpenCV and LibPNG
    //  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    //  Texture::Image2D(TextureTarget::_2D, *image);
    //  return result;
    //}
    TexturePtr load2dTexture(const QString& path) {
        uvec2 outSize;
        return load2dTexture(path, outSize);
    }

    TexturePtr load2dTexture(const QString& path, uvec2 & outSize) {
        QImage image = QImage(path).mirrored(false, true);
        using namespace oglplus;
        size_t width = image.width();
        size_t height = image.height();
        PixelDataFormat format = PixelDataFormat::RGB;
        if (image.hasAlphaChannel()) {
            format = PixelDataFormat::RGBA;
        }
        TexturePtr result(new oglplus::Texture());
        Context::Bound(Texture::Target::_2D, *result).Image2D(0, PixelDataInternalFormat::RGBA8, width, height, 0,
            format, PixelDataType::UnsignedByte, image.constBits());
        return result;
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
