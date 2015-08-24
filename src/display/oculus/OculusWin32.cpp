#include "Common.h"

#include "OculusWin32.h"

#include "qt/QtUtils.h"
#include <OVR_CAPI_GL.h>
#include <GL/wglew.h>

using namespace Plugins::Display;

// A basic wrapper for constructing a framebuffer with a renderbuffer
// for the depth attachment and an undefined type for the color attachement
// This allows us to reuse the basic framebuffer code for both the Mirror
// FBO as well as the Oculus swap textures we will use to render the scene
// Though we don't really need depth at all for the mirror FBO, or even an
// FBO, but using one means I can just use a glBlitFramebuffer to get it onto
// the screen.
template <
    typename C,
    typename D
>
struct OvrFramebufferWrapper {
    uvec2       size;
    oglplus::Framebuffer fbo;
    C           color;
    D           depth;

    OvrFramebufferWrapper() {}

    virtual ~OvrFramebufferWrapper() {
    }

    virtual void Init(const uvec2 & size) {
        this->size = size;
        initColor();
        initDepth();
        initDone();
    }

    template <typename F>
    void Bound(F f) {
        Bound(oglplus::Framebuffer::Target::Draw, f);
    }

    template <typename F>
    void Bound(oglplus::Framebuffer::Target target, F f) {
        fbo.Bind(target);
        onBind(target);
        f();
        onUnbind(target);
        oglplus::DefaultFramebuffer().Bind(target);
    }

    void Viewport() {
        oglplus::Context::Viewport(size.x, size.y);
    }

protected:
    virtual void onBind(oglplus::Framebuffer::Target target) {}
    virtual void onUnbind(oglplus::Framebuffer::Target target) {}

    static GLenum toEnum(oglplus::Framebuffer::Target target) {
        switch (target) {
        case oglplus::Framebuffer::Target::Draw:
            return GL_DRAW_FRAMEBUFFER;
        case oglplus::Framebuffer::Target::Read:
            return GL_READ_FRAMEBUFFER;
        default:
            Q_ASSERT(false);
            return GL_FRAMEBUFFER;
        }
    }

    virtual void initDepth() {}

    virtual void initColor() {}

    virtual void initDone() = 0;
};


// A base class for FBO wrappers that need to use the Oculus C
// API to manage textures via ovr_CreateSwapTextureSetGL,
// ovr_CreateMirrorTextureGL, etc
template <typename C>
struct RiftFramebufferWrapper : public OvrFramebufferWrapper<C, char> {
    ovrHmd hmd;
    RiftFramebufferWrapper(const ovrHmd & hmd) : hmd(hmd) {
        color = 0;
        depth = 0;
    };

    void Resize(const uvec2 & size) {
        QOpenGLContext::currentContext()->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oglplus::GetName(fbo));
        QOpenGLContext::currentContext()->functions()->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        QOpenGLContext::currentContext()->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        this->size = size;
        initColor();
        initDone();
    }

protected:
    virtual void initDepth() override final {
    }
};

// A wrapper for constructing and using a swap texture set,
// where each frame you draw to a texture via the FBO,
// then submit it and increment to the next texture.
// The Oculus SDK manages the creation and destruction of
// the textures
struct SwapFramebufferWrapper : public RiftFramebufferWrapper<ovrSwapTextureSet*> {
    SwapFramebufferWrapper(const ovrHmd & hmd)
        : RiftFramebufferWrapper(hmd) {
    }

    ~SwapFramebufferWrapper() {
        if (color) {
            ovr_DestroySwapTextureSet(hmd, color);
            color = nullptr;
        }
    }

    void Increment() {
        ++color->CurrentIndex;
        color->CurrentIndex %= color->TextureCount;
    }

protected:
    virtual void initColor() override {
        if (color) {
            ovr_DestroySwapTextureSet(hmd, color);
            color = nullptr;
        }

        ovrResult result = ovr_CreateSwapTextureSetGL(hmd, GL_RGBA, size.x, size.y, &color);
        Q_ASSERT(OVR_SUCCESS(result));

        for (int i = 0; i < color->TextureCount; ++i) {
            ovrGLTexture& ovrTex = (ovrGLTexture&)color->Textures[i];
            glBindTexture(GL_TEXTURE_2D, ovrTex.OGL.TexId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    virtual void initDone() override {
    }

    virtual void onBind(oglplus::Framebuffer::Target target) override {
        ovrGLTexture& tex = (ovrGLTexture&)(color->Textures[color->CurrentIndex]);
        GLenum glTarget = toEnum(target);
        QOpenGLContext::currentContext()->functions()->glFramebufferTexture2D(glTarget, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.OGL.TexId, 0);
    }

    virtual void onUnbind(oglplus::Framebuffer::Target target) override {
        GLenum glTarget = toEnum(target);
        QOpenGLContext::currentContext()->functions()->glFramebufferTexture2D(toEnum(target), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    }
};


// We use a FBO to wrap the mirror texture because it makes it easier to
// render to the screen via glBlitFramebuffer
struct MirrorFramebufferWrapper : public RiftFramebufferWrapper<ovrGLTexture*> {
    MirrorFramebufferWrapper(const ovrHmd & hmd)
        : RiftFramebufferWrapper(hmd) {
    }

    virtual ~MirrorFramebufferWrapper() {
        if (color) {
            ovr_DestroyMirrorTexture(hmd, (ovrTexture*)color);
            color = nullptr;
        }
    }

private:
    void initColor() override {
        if (color) {
            ovr_DestroyMirrorTexture(hmd, (ovrTexture*)color);
            color = nullptr;
        }
        ovrResult result = ovr_CreateMirrorTextureGL(hmd, GL_RGBA, size.x, size.y, (ovrTexture**)&color);
        Q_ASSERT(OVR_SUCCESS(result));
    }

    void initDone() override {
        QOpenGLContext::currentContext()->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oglplus::GetName(fbo));
        QOpenGLContext::currentContext()->functions()->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color->OGL.TexId, 0);
        QOpenGLContext::currentContext()->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
};


template <typename Function>
void for_each_eye(Function function) {
    for (ovrEyeType eye = ovrEyeType::ovrEye_Left;
        eye < ovrEyeType::ovrEye_Count;
        eye = static_cast<ovrEyeType>(eye + 1)) {
        function(eye);
    }
}

template <typename Function>
void for_each_eye(const ovrHmd & hmd, Function function) {
    for (int i = 0; i < ovrEye_Count; ++i) {
        ovrEyeType eye = hmd->EyeRenderOrder[i];
        function(eye);
    }
}


inline glm::mat4 toGlm(const ovrMatrix4f & om) {
    return glm::transpose(glm::make_mat4(&om.M[0][0]));
}

inline glm::mat4 toGlm(const ovrFovPort & fovport, float nearPlane = 0.01f, float farPlane = 10000.0f) {
    return toGlm(ovrMatrix4f_Projection(fovport, nearPlane, farPlane, true));
}

inline glm::vec3 toGlm(const ovrVector3f & ov) {
    return glm::make_vec3(&ov.x);
}

inline glm::vec2 toGlm(const ovrVector2f & ov) {
    return glm::make_vec2(&ov.x);
}

inline glm::ivec2 toGlm(const ovrVector2i & ov) {
    return glm::ivec2(ov.x, ov.y);
}

inline glm::uvec2 toGlm(const ovrSizei & ov) {
    return glm::uvec2(ov.w, ov.h);
}

inline glm::quat toGlm(const ovrQuatf & oq) {
    return glm::make_quat(&oq.x);
}

inline glm::mat4 toGlm(const ovrPosef & op) {
    glm::mat4 orientation = glm::mat4_cast(toGlm(op.Orientation));
    glm::mat4 translation = glm::translate(glm::mat4(), toGlm(op.Position));
    return translation * orientation;
}

class OculusWin32DisplayPlugin : public Plugins::Display::Plugin {
    virtual bool supported() const override {
        return true;
    }
    virtual const char* name() const override {
        return "Oculus SDK 0.6.x";
    }
    virtual Plugins::Display::Type type() const override {
        return Type::HMD;
    }

    virtual bool init() override { 
        connect(&_timer, &QTimer::timeout, this, &OculusWin32DisplayPlugin::requestFrame);
        _layers[0] = &_sceneLayer.Header;
        _layers[1] = &_uiLayer.Header;
        return true;
    }
    virtual void destroy() override {}
    virtual bool start() override;
    virtual void stop() override;

    virtual void setShareContext(QOpenGLContext* context) override { _shareContext = context; };
    virtual glm::uvec2 preferredSurfaceSize() const override;
    virtual glm::uvec2 preferredUiSize() const override;
    virtual glm::mat4 projection(Plugins::Display::Eye eye, const glm::mat4& baseProjection) const override;
    virtual glm::mat4 pose(Plugins::Display::Eye eye) const override;
    virtual QWindow* window() override { return nullptr; }

    virtual void preRender() override;
    virtual void render(
            uint32_t sceneTexture, const glm::uvec2& textureSize,
            uint32_t uiTexture, const glm::uvec2& uiSize, const glm::mat4& uiView) override;
    virtual void postRender() override;

    virtual void resetPose() {
        ovr_RecenterPose(_hmd);
    }

private:
    void resizedMirror();

    QOpenGLContext* _shareContext{ nullptr };
    QWindow* _window{ nullptr };
    QOpenGLContext* _context{ nullptr };
    QTimer _timer;

    ProgramPtr _program;
    ShapeWrapperPtr _quad;

    // Oculus specific code
    ovrHmd _hmd{ nullptr };
	ovrGraphicsLuid _luid;
	ovrHmdDesc _hmdDesc;
    SwapFramebufferWrapper* _sceneSwapFbo{ nullptr };
    SwapFramebufferWrapper* _uiSwapFbo{ nullptr };
    MirrorFramebufferWrapper* _mirrorFbo{ nullptr };

    ovrLayerEyeFov _sceneLayer;
    ovrLayerQuad _uiLayer;
    ovrLayerHeader* _layers[2];


    mat4 _eyeProjections[ovrEye_Count];
    ovrVector3f _eyeOffsets[ovrEye_Count];
    ovrSizei _renderTargetSize;
    ovrSizei _eyeTextureSize;
    ovrRecti _eyeViewports[ovrEye_Count];
    ovrFovPort _eyeFovs[ovrEye_Count];
    ovrPosef _eyePoses[ovrEye_Count];
    const uvec2 _uiSize{ 1280, 720 };
};

bool OculusWin32DisplayPlugin::start() {
    {
        ovrInitParams initParams; memset(&initParams, 0, sizeof(initParams));
#ifdef DEBUG
        initParams.Flags |= ovrInit_Debug;
#endif
        ovrResult result = ovr_Initialize(&initParams);
        Q_ASSERT(OVR_SUCCESS(result));
        result = ovr_Create(&_hmd, &_luid);
		_hmdDesc = ovr_GetHmdDesc(_hmd);

        for_each_eye([&](ovrEyeType eye) {
            _eyeFovs[eye] = _hmdDesc.DefaultEyeFov[eye];
            _eyeProjections[eye] = toGlm(ovrMatrix4f_Projection(_eyeFovs[eye],
                0.01f, 100.0f, ovrProjection_RightHanded));
            ovrEyeRenderDesc erd = ovr_GetRenderDesc(_hmd, eye, _eyeFovs[eye]);
            _eyeOffsets[eye] = erd.HmdToEyeViewOffset;
        });
        _eyeTextureSize = ovr_GetFovTextureSize(_hmd, ovrEye_Left, _eyeFovs[ovrEye_Left], 1.0f);
        _renderTargetSize = { _eyeTextureSize.w * 2, _eyeTextureSize.h };

        ovr_ConfigureTracking(_hmd,
            ovrTrackingCap_Orientation | ovrTrackingCap_Position | ovrTrackingCap_MagYawCorrection,
            ovrTrackingCap_Orientation);

    }



    Q_ASSERT(!_window);
    Q_ASSERT(_shareContext);
    _window = new QWindow;
    _window->setSurfaceType(QSurface::OpenGLSurface);
    _window->setFormat(getDesiredSurfaceFormat());
    _context = new QOpenGLContext;
    _context->setFormat(getDesiredSurfaceFormat());
    _context->setShareContext(_shareContext);
    _context->create();

    _window->show();
    _window->setGeometry(QRect(100, -800, 1280, 720));

    bool result = _context->makeCurrent(_window);

#if defined(Q_OS_WIN)
    if (wglewGetExtension("WGL_EXT_swap_control")) {
        wglSwapIntervalEXT(0);
        int swapInterval = wglGetSwapIntervalEXT();
        qDebug("V-Sync is %s\n", (swapInterval > 0 ? "ON" : "OFF"));
    } 
#elif defined(Q_OS_LINUX)
#else
    qCDebug(interfaceapp, "V-Sync is FORCED ON on this system\n");
#endif

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

        _mirrorFbo = new MirrorFramebufferWrapper(_hmd);
        _sceneSwapFbo = new SwapFramebufferWrapper(_hmd);
        _sceneSwapFbo->Init(toGlm(_renderTargetSize));
        _uiSwapFbo = new SwapFramebufferWrapper(_hmd);
        _uiSwapFbo->Init(_uiSize);
        _mirrorFbo->Init(uvec2(100, 100));
        _context->doneCurrent();
    }

    _sceneLayer.ColorTexture[0] = _sceneSwapFbo->color;
    _sceneLayer.ColorTexture[1] = nullptr;
    _sceneLayer.Viewport[0].Pos = { 0, 0 };
    _sceneLayer.Viewport[0].Size = _eyeTextureSize;
    _sceneLayer.Viewport[1].Pos = { _eyeTextureSize.w, 0 };
    _sceneLayer.Viewport[1].Size = _eyeTextureSize;
    _sceneLayer.Header.Type = ovrLayerType_EyeFov;
    _sceneLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
    for_each_eye([&](ovrEyeType eye) {
        _eyeViewports[eye] = _sceneLayer.Viewport[eye];
        _sceneLayer.Fov[eye] = _eyeFovs[eye];
    });

    _uiLayer.ColorTexture = _uiSwapFbo->color;
    _uiLayer.Header.Type = ovrLayerType_QuadInWorld;
    _uiLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
    _uiLayer.QuadPoseCenter.Orientation = { 0, 0, 0, 1 };
    _uiLayer.QuadPoseCenter.Position = { 0, 0, -1 }; 
    _uiLayer.QuadSize = { aspect(_uiSize), 1.0f };
    _uiLayer.Viewport = { { 0, 0 }, { (int)_uiSize.x, (int)_uiSize.y } };

    _timer.start(0);

    connect(_window, &QWindow::widthChanged, this, &OculusWin32DisplayPlugin::resizedMirror);
    connect(_window, &QWindow::heightChanged, this, &OculusWin32DisplayPlugin::resizedMirror);
    resizedMirror();
    return true;
}

void OculusWin32DisplayPlugin::stop() {
    _context->makeCurrent(_window);

    if (_sceneSwapFbo) {
        delete _sceneSwapFbo;
        _sceneSwapFbo = nullptr;
    }

    if (_mirrorFbo) {
        delete _mirrorFbo;
        _mirrorFbo = nullptr;
    }

    _context->doneCurrent();

    _timer.stop();
    _window->deleteLater();
    _window = nullptr;
    _context->deleteLater();
    _context = nullptr;

    if (_hmd) {
        ovr_Destroy(_hmd);
        _hmd = nullptr;
    }
    ovr_Shutdown();

}


void OculusWin32DisplayPlugin::resizedMirror() {
    auto newSize = qt::toGlm(_window->size());
    _context->makeCurrent(_window);
    if (newSize != _mirrorFbo->size) {
        _mirrorFbo->Resize(newSize);
    }
    _context->doneCurrent();
}

glm::uvec2 OculusWin32DisplayPlugin::preferredSurfaceSize() const {
    return toGlm(_renderTargetSize);
}

glm::uvec2 OculusWin32DisplayPlugin::preferredUiSize() const {
    return _uiSize;
}

glm::mat4 OculusWin32DisplayPlugin::projection(Plugins::Display::Eye eye, const glm::mat4& baseProjection) const {
    return _eyeProjections[eye];
}

glm::mat4 OculusWin32DisplayPlugin::pose(Plugins::Display::Eye eye) const {
    return toGlm(_eyePoses[eye]);
}

void OculusWin32DisplayPlugin::preRender() {
    ovr_GetEyePoses(_hmd, 0, _eyeOffsets, _eyePoses, nullptr);
    bool result = _context->makeCurrent(_window);
    Q_ASSERT(result);
}

void OculusWin32DisplayPlugin::render(
    uint32_t sceneTexture, const glm::uvec2& textureSize,
    uint32_t uiTexture, const glm::uvec2& uiSize, const glm::mat4& uiView) {

    //ovrPerfHud_RenderTiming
    //ovrPerfHud_Off
    //ovrPerfHud_LatencyTiming
    //ovr_SetInt(_hmd, "PerfHudMode", (int)ovrPerfHud_RenderTiming);
    if (uiTexture) {
        _uiSwapFbo->Bound(oglplus::Framebuffer::Target::Draw, [&] {
            glViewport(0, 0, _uiSize.x, _uiSize.y);
            glBindTexture(GL_TEXTURE_2D, uiTexture);
            oria::renderGeometry(_quad, _program);
        });
    }

    // Blit to the oculus provided texture
    _sceneSwapFbo->Bound(oglplus::Framebuffer::Target::Draw, [&] {
        glViewport(0, 0, _renderTargetSize.w, _renderTargetSize.h);
        if (sceneTexture) {
            glBindTexture(GL_TEXTURE_2D, sceneTexture);
            oria::renderGeometry(_quad, _program);
        }
    });

    // Submit the frame to the Oculus SDK for timewarp and distortion
    for_each_eye([&](ovrEyeType eye) {
        _sceneLayer.RenderPose[eye] = _eyePoses[eye];
    });
    ovrResult res = ovr_SubmitFrame(_hmd, 0, nullptr, _layers, uiTexture ? 2 : 1);

    // Blit to the onscreen window
    _mirrorFbo->Bound(oglplus::Framebuffer::Target::Read, [&] {
        auto sourceSize = _mirrorFbo->size;
        auto destWindowSize = _window->size();
        glBlitFramebuffer(
            0, sourceSize.y, sourceSize.x, 0,
            0, 0, destWindowSize.width(), destWindowSize.height(),
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
    });




    Q_ASSERT(OVR_SUCCESS(res));
    _sceneSwapFbo->Increment();
    _uiSwapFbo->Increment();
}

void OculusWin32DisplayPlugin::postRender() {
    _context->swapBuffers(_window);
}

Plugins::Display::Plugin* buildOculusPlugin() {
    return new OculusWin32DisplayPlugin();
}
