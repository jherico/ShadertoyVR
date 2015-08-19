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
#include <glm/glm.hpp>
#include <QOpenGLContext>
#include <QWindow>
namespace Plugins {

class Plugin {
    public:
        virtual bool supported() const = 0;
        virtual const char* name() const = 0;
        /**
         *  Called after supported has returned true.  Will only be called once per
         *  app run
         */
        virtual bool init() = 0;
        /**
         *  Called before the app shuts down, if init returned true.  Will only be
         *  called once per app run
         */
        virtual void destroy() = 0;
        /**
         * Called whenever the application activates the plugin.  May be called
         * multiple times, but shutdown should be called before startup is called
         * again.
         */
        virtual bool start() = 0;
        /**
         * Called whenever the application deactivates the plugin.  May be called
         * multiple times, but will only be called after a successful start() call
         */
        virtual void stop() = 0;
    };

    namespace Display {
        enum class Type {
            _2D,
            STEREO,
            HMD,
        };

        enum Eye {
            LEFT,
            RIGHT,
            MONO
        };

        class Plugin : public QObject, public Plugins::Plugin {
            Q_OBJECT
        public:
            virtual Type type() const = 0;
            virtual glm::uvec2 preferredSurfaceSize() const = 0;

            virtual glm::uvec2 preferredUiSize() const {
                return preferredSurfaceSize();
            }
            virtual void setShareContext(QOpenGLContext* context) {};
            virtual QOpenGLContext* context() { return nullptr; }
            virtual QWindow* window() = 0;
            virtual void preRender() = 0;
            virtual void render(
                    uint32_t sceneTexture, const glm::uvec2& textureSize,
                    uint32_t uiTexture = 0, const glm::uvec2& uiSize = glm::uvec2(), 
                    const glm::mat4& uiView = glm::mat4()) = 0;
            virtual void postRender() = 0;

            virtual glm::mat4 projection(Eye eye, const glm::mat4& baseProjection) const {
                return baseProjection;
            }

            virtual glm::mat4 pose(Eye eye) const {
                return glm::mat4();
            }

            virtual void resetPose() {
            }

        signals:
            void requestFrame();
            void sizeChanged();
        };

        /// returns a null terminated array of display plugins supported on this system
        size_t  list(Plugin**);
    }
}
