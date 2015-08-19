#include "qt/QtCommon.h"
#include "Window.h"
#include <QWindow>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QCoreApplication>
#include <QTimer>

using namespace Plugins::Display;


class WindowDisplayPlugin : public Plugins::Display::Plugin {
public:
    WindowDisplayPlugin();
    virtual bool supported() const override;
    virtual const char* name() const override;
    virtual bool init() override;
    virtual void destroy() override;
    virtual bool start() override;
    virtual void stop() override;

    virtual void setShareContext(QOpenGLContext* context) override { _shareContext = context; };
    virtual QOpenGLContext* context() override { return _context; };
    virtual Plugins::Display::Type type() const override { return Type::_2D; }
    virtual QWindow* window() override { return _window; }
    virtual glm::uvec2 preferredSurfaceSize() const override;
    virtual glm::uvec2 preferredUiSize() const override;

    virtual void preRender() override;
    virtual void render(
            uint32_t sceneTexture, const glm::uvec2& textureSize,
            uint32_t uiTexture, const glm::uvec2& uiSize, const glm::mat4& uiView) override;
    virtual void postRender() override;
private:
    QOpenGLContext* _shareContext{ nullptr };
    QWindow* _window{ nullptr };
    QOpenGLContext* _context{ nullptr };
    QTimer _timer;

    // GLSL and geometry for the UI
    ProgramPtr _program;
    ShapeWrapperPtr _quad;
};

WindowDisplayPlugin::WindowDisplayPlugin() {
    connect(&_timer, &QTimer::timeout, this, &WindowDisplayPlugin::requestFrame);
}

bool WindowDisplayPlugin::supported() const {
    return true;
}

const char* WindowDisplayPlugin::name() const {
    return "Window";
}

bool WindowDisplayPlugin::init() {
    return true;
}

void WindowDisplayPlugin::destroy() {
}

bool WindowDisplayPlugin::start() {
    Q_ASSERT(!_window);
    Q_ASSERT(_shareContext);
    _window = new QWindow;
    _window->setSurfaceType(QSurface::OpenGLSurface);
    _window->setFormat(getDesiredSurfaceFormat());
    _context = new QOpenGLContext;
    _context->setFormat(getDesiredSurfaceFormat());
    _context->setShareContext(_shareContext);
    _context->create();
   
    connect(_window, &QWindow::widthChanged, this, &WindowDisplayPlugin::sizeChanged);
    connect(_window, &QWindow::heightChanged, this, &WindowDisplayPlugin::sizeChanged);

    _window->show();
    _window->setGeometry(QRect(100, -800, 1280, 720));

    bool result = _context->makeCurrent(_window);
    Q_ASSERT(result);
    {
        // The geometry and shader for rendering the 2D UI surface when needed
        _program = oria::loadProgram(
            Resource::SHADERS_TEXTURED_VS,
            Resource::SHADERS_TEXTURED_FS);
        _quad = oria::loadPlane(_program, 1.0f);

        glClearColor(0, 1, 1, 1);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    _context->doneCurrent();

    _timer.start(0);
    return true;
}

void WindowDisplayPlugin::stop() {
    _context->makeCurrent(_window);
    _quad.reset();
    _program.reset();
    _context->doneCurrent();

    Q_ASSERT(_window);
    _timer.stop();
    _window->deleteLater();
    _window = nullptr;
    _context->deleteLater();
    _context = nullptr;
}

glm::uvec2 WindowDisplayPlugin::preferredSurfaceSize() const {
    auto size = _window->size();
    return glm::uvec2(size.width(), size.height());
}

glm::uvec2 WindowDisplayPlugin::preferredUiSize() const {
    return preferredSurfaceSize();
}

void WindowDisplayPlugin::preRender() {
    bool result = _context->makeCurrent(_window);
    Q_ASSERT(result);
}

void WindowDisplayPlugin::render(
    uint32_t sceneTexture, const glm::uvec2& textureSize,
    uint32_t uiTexture, const glm::uvec2& uiSize, const glm::mat4& uiView) {
    glClear(GL_COLOR_BUFFER_BIT);
    auto size = _window->size();
    glViewport(0, 0, size.width(), size.height());
    if (sceneTexture) {
        glBindTexture(GL_TEXTURE_2D, sceneTexture);
        oria::renderGeometry(_quad, _program);
    }
    if (uiTexture) {
        glBindTexture(GL_TEXTURE_2D, uiTexture);
        oria::renderGeometry(_quad, _program);
    }
}

void WindowDisplayPlugin::postRender() {
    _context->swapBuffers(_window);
}


Plugins::Display::Plugin* buildWindowPlugin() {
    return new WindowDisplayPlugin();
}

