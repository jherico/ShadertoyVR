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

typedef std::shared_ptr<oglplus::Texture> TexturePtr;
typedef std::shared_ptr<oglplus::images::Image> ImagePtr;

namespace oria {
  ImagePtr loadImage(const QByteArray & data, bool flip = true);
  ImagePtr loadImage(const QString & url, bool flip = true);

  TexturePtr load2dTexture(const QByteArray & data, uvec2 & outSize);
  TexturePtr load2dTexture(const QString & url, uvec2 & outSize);

  TexturePtr loadCubemapTexture(std::function<ImagePtr(int)> dataLoader);
  TexturePtr loadCubemapTexture(const QString & url, int resourceOrder[6], bool flip = true);
  TexturePtr loadCubemapTexture(const QString & url, bool flip = true);
}
