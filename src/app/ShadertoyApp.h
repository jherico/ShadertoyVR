#pragma once
#include <QGuiApplication>
#include <QQuickView>

#include "shadertoy/Shadertoy.h"
#include "shadertoy/Renderer.h"
#include "display/DisplayPlugins.h"
#include "MainWindow.h"
#include "CodeEditor.h"
#include "qt/OffscreenGlSurface.h"
#include "qt/OffscreenQmlSurface.h"

class QOpenGLDebugLogger;

#define SHADER_PROPERTY(type, name) \
    Q_PROPERTY(type name READ name WRITE set##name NOTIFY name##Changed) \
public: \
    type name() { return _##name; }; \
    void set##name(const type& new##name) { if (_##name != new##name) { _##name = new##name;  emit name##Changed(); } } \
private: \
    type _##name;


class ShadertoyShader : public QObject {
    Q_OBJECT
    SHADER_PROPERTY(QString, shader);
    SHADER_PROPERTY(QString, channel0);
    SHADER_PROPERTY(QString, channel1);
    SHADER_PROPERTY(QString, channel2);
    SHADER_PROPERTY(QString, channel3);
signals:
    void shaderChanged();
    void channel0Changed();
    void channel1Changed();
    void channel2Changed();
    void channel3Changed();
};

class ShadertoyApp : public QGuiApplication {
    Q_OBJECT
public:
    ShadertoyApp(int argc, char ** argv);
    virtual ~ShadertoyApp();
    
    void activatePlugin(int index);

    virtual bool eventFilter(QObject* receiver, QEvent* event) override;
private:
    void initTracker();
    void initAppInformation();
    void initResources();
    void initSettings();
    void initLogging();
    void initTypes();
    void initUi();
    void initDisplayPlugins();

    void shutdownTracker();
    void shutdownLogging();

    void setupGlContext();
    void setupOffscreenUi();
    void setupRenderer();

public slots:
    void onFrameRequested();
    void onUiTextureReady(GLuint texture, GLsync sync);
    void onSizeChanged();
    //void onSixDofMotion(const vec3 & tr, const vec3 & mo);
    //void onTimer();
    //void onFontSizeChanged(int newSize);
    //void onShaderTextureReady(GLuint texture, GLsync sync);
    //void onNewShaderFilepath(const QString & shaderPath);
    //void onNewShaderHighlighted(const QString & shaderPath);
    //void onNewPresetHighlighted(int presetId);

    void loadShader(const shadertoy::Shader & shader);
    void loadShaderFile(const QString & shaderPath);
    void loadPreset(int index);
    void loadNextPreset();
    void loadPreviousPreset();

    void recenterPose();
    void toggleUi();
    void buildShader();


    //void onSaveShaderXml(const QString & shaderPath);
    void onChannelTextureChanged(const int & channelIndex, const int & channelType, const QString & texturePath);
    //void onModifyTextureResolution(double scale);
    //void onModifyPositionScale(double scale);
    //void onResetPositionScale();
    void toggleEyePerFrame();
    void restartShader();

    void queueRenderThreadTask(std::function<void()> f);
private:
    Renderer _renderer;
    OffscreenGlSurface _surface;
    OffscreenQmlSurface _uiSurface;
    CodeEditor* _codeEditor{ nullptr };
    QOpenGLDebugLogger* _logger{ nullptr };
    Plugins::Display::Plugin* _activePlugin{ nullptr };
    QVariantAnimation animation;
    float animationValue;
    float eyeOffsetScale{ 1.0f };
    float savedEyeOffsetScale{ 1.0f };

    // We actually render the shader to one FBO for dynamic framebuffer scaling,
    // while leaving the actual texture we pass to the Oculus SDK fixed.
    // This allows us to have a clear UI regardless of the shader performance
    FramebufferWrapperPtr shaderFramebuffer;

    ShadertoyShader _activeShader;
};

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<ShadertoyApp*>(QGuiApplication::instance()))



/*
using AtomicGlTexture = std::atomic<GLuint>;
using SyncPair = std::pair<GLuint, GLsync>;
using TextureTrashcan = std::queue<SyncPair>;
using TextureDeleteQueue = std::vector<GLuint>;
using Mutex = std::mutex;
using Lock = std::unique_lock<Mutex>;

virtual void setup();
virtual void resize();
virtual void drawFrame();


// A cache of all the input textures available
QDir configPath;
QSettings settings;

//////////////////////////////////////////////////////////////////////////////
//
// Offscreen UI
//
//  QOffscreenUi * uiWindow{ new QOffscreenUi() };
//  GlslHighlighter highlighter;

int activePresetIndex{ 0 };
float savedEyePosScale{ 1.0f };

//////////////////////////////////////////////////////////////////////////////
//
// Shader Rendering information
//

shadertoy::Shader activeShader;


// The current mouse position as reported by the main thread
bool uiVisible{ false };
QVariantAnimation animation;
float animationValue;


// A wrapper for passing the UI texture from the app to the widget
AtomicGlTexture uiTexture{ 0 };
TextureTrashcan textureTrash;
TextureDeleteQueue textureDeleteQueue;
Mutex textureLock;
QTimer timer;

// GLSL and geometry for the UI
ProgramPtr uiProgram;
ShapeWrapperPtr uiShape;
TexturePtr mouseTexture;
ShapeWrapperPtr mouseShape;

// For easy compositing the UI texture and the mouse texture
FramebufferWrapperPtr uiFramebuffer;

// Geometry and shader for rendering the possibly low res shader to the main framebuffer
ProgramPtr planeProgram;
ShapeWrapperPtr plane;
*/
/*
#include "Fetcher.h"

class MainWindow : public QRiftWindow {
  Q_OBJECT


  // Measure the FPS for use in dynamic scaling
  GLuint exchangeUiTexture(GLuint newUiTexture) {
    return uiTexture.exchange(newUiTexture);
  }

  Fetcher fetcher;

public:
  MainWindow();
  virtual void stop();

private:
  virtual void setup();
  void setupOffscreenUi();

  QVariant getItemProperty(const QString & itemName, const QString & property);
  void setItemProperty(const QString & itemName, const QString & property, const QVariant & value);

  QString getItemText(const QString & itemName);
  void setItemText(const QString & itemName, const QString & text);

private:
  void loadShader(const shadertoy::Shader & shader);
  void loadFile(const QString & file);
  void updateFps(float fps);

  ///////////////////////////////////////////////////////
  //
  // Event handling customization
  //
  void mouseMoveEvent(QMouseEvent * me);
  bool event(QEvent * e);
  void resizeEvent(QResizeEvent *e);
  QPointF mapWindowToUi(const QPointF & p);

  ///////////////////////////////////////////////////////
  //
  // Rendering functionality
  //
  void perFrameRender();
  void perEyeRender();

signals:
  void fpsUpdated(float);
};
*/
