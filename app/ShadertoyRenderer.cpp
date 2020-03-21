#include "Shadertoy.h"
#include "ShadertoyRenderer.h"

void ShadertoyRenderer::setup() {
  QRiftWindow::setup();
  initTextureCache();

  setShaderSourceInternal(QtUtil::toString(":/shaders/default.fs"));
  assert(shadertoyProgram);
  if (!skybox) {
    skybox = oria::loadSkybox(shadertoyProgram);
  }

  Platform::addShutdownHook([&] {
    textureCache.clear();
    shadertoyProgram.reset();
    vertexShader.reset();
    fragmentShader.reset();
    skybox.reset();
  });
}

void ShadertoyRenderer::initTextureCache() {
  QRegExp re("(tex|cube)(\\d+)(_0)?\\.(png|jpg)");

  for (int i = 0; i < st::TEXTURES.size(); ++i) {
    QString path = st::TEXTURES.at(i);
    QString fileName = path.split("/").back();
    qDebug() << "Loading texture from " << path;
    TextureData & cacheEntry = textureCache[path];
    cacheEntry.tex = oria::load2dTexture(":" + path, cacheEntry.size);
    canonicalPathMap["qrc:" + path] = path;

    // Backward compatibility
    if (re.exactMatch(fileName)) {
      int textureId = re.cap(2).toInt();

      QString alias = QString("preset://tex/%1").arg(textureId);
      qDebug() << "Adding alias for " << path << " to " << alias;
      canonicalPathMap[alias] = path;

      alias = QString("preset://tex/%1").arg(textureId, 2, 10, QChar('0'));
      qDebug() << "Adding alias for " << path << " to " << alias;
      canonicalPathMap[alias] = path;
    }
  }

  for (int i = 0; i < st::CUBEMAPS.size(); ++i) {
    QString pathTemplate = st::CUBEMAPS.at(i);
    QString path = pathTemplate.arg(0);
    QString fileName = path.split("/").back();
    qDebug() << "Processing path " << path;
    TextureData & cacheEntry = textureCache[path];
    cacheEntry.tex = oria::loadCubemapTexture([&](int i){
      QString texturePath = pathTemplate.arg(i);
      ImagePtr image = oria::loadImage(":" + texturePath, false);
      if (image) {
        cacheEntry.size = uvec2(image->Width(), image->Height());
      }
      return image;
    });
    canonicalPathMap["qrc:" + path] = path;

    // Backward compatibility
    if (re.exactMatch(fileName)) {
      int textureId = re.cap(2).toInt();
      QString alias = QString("preset://cube/%1").arg(textureId);
      qDebug() << "Adding alias for " << path << " to " << alias;
      canonicalPathMap[alias] = path;

      alias = QString("preset://cube/%1").arg(textureId, 2, 10, QChar('0'));
      qDebug() << "Adding alias for " << path << " to " << alias;
      canonicalPathMap[alias] = path;
    }
  }

  std::for_each(canonicalPathMap.begin(), canonicalPathMap.end(), [&](CanonicalPathMap::const_reference & entry) {
    qDebug() << entry.second << "\t" << entry.first;
  });
}

void ShadertoyRenderer::renderShadertoy() {
  using namespace oglplus;
  Context::Clear().ColorBuffer();
  if (!shadertoyProgram) {
    return;
  }
  MatrixStack & mv = Stacks::modelview();
  mv.withPush([&] {
    mv.untranslate();
    oria::renderGeometry(skybox, shadertoyProgram, uniformLambdas);
  });
  for (int i = 0; i < 4; ++i) {
    oglplus::DefaultTexture().Active(0);
    DefaultTexture().Bind(Texture::Target::_2D);
    DefaultTexture().Bind(Texture::Target::CubeMap);
  }
  oglplus::Texture::Active(0);
}

void ShadertoyRenderer::updateUniforms() {
  using namespace oglplus;
  typedef std::map<std::string, GLuint> Map;
  Map activeUniforms = oria::getActiveUniforms(shadertoyProgram);
  shadertoyProgram->Bind();
  //    UNIFORM_DATE;
  for (int i = 0; i < 4; ++i) {
    const char * uniformName = st::UNIFORM_CHANNELS[i];
    if (activeUniforms.count(uniformName)) {
      context()->functions()->glUniform1i(activeUniforms[uniformName], i);
    }
    if (channels[i].texture) {
      if (activeUniforms.count(st::UNIFORM_CHANNEL_RESOLUTIONS[i])) {
        Uniform<vec3>(*shadertoyProgram, st::UNIFORM_CHANNEL_RESOLUTIONS[i]).Set(channels[i].resolution);
      }

    }
  }
  NoProgram().Bind();

  uniformLambdas.clear();
  if (activeUniforms.count(st::UNIFORM_GLOBALTIME)) {
    uniformLambdas.push_back([&] {
      Uniform<GLfloat>(*shadertoyProgram, st::UNIFORM_GLOBALTIME).Set((float)shaderTime.elapsed() / 1000.0f);
    });
  }

  if (activeUniforms.count(st::UNIFORM_RESOLUTION)) {
    uniformLambdas.push_back([&] {
      vec3 res = vec3(renderSize(), 0);
      Uniform<vec3>(*shadertoyProgram, st::UNIFORM_RESOLUTION).Set(res);
    });
  }

#if USE_RIFT
  if (activeUniforms.count(st::UNIFORM_POSITION)) {
    uniformLambdas.push_back([&] {
      Uniform<vec3>(*shadertoyProgram, st::UNIFORM_POSITION).Set(
            (ovr::toGlm(getEyePose().Position) + position) * eyePosScale);
    });
  }
#endif

  for (int i = 0; i < 4; ++i) {
    if (activeUniforms.count(st::UNIFORM_CHANNELS[i]) && channels[i].texture) {
      uniformLambdas.push_back([=] {
        if (this->channels[i].texture) {
          Texture::Active(i);
          this->channels[i].texture->Bind(channels[i].target);
        }
      });
    }
  }
}

vec2 ShadertoyRenderer::textureSize() {
#if USE_RIFT
  return vec2(ovr::toGlm(eyeTextures[0].Header.TextureSize));
#else
  return vec2(size().width(), size().height());
#endif
}

uvec2 ShadertoyRenderer::renderSize() {
  return uvec2(texRes * textureSize());
}



bool ShadertoyRenderer::setShaderSourceInternal(QString source) {
  using namespace oglplus;
  try {
    position = vec3();
    if (!vertexShader) {
      QString vertexShaderSource = QtUtil::toString(":/shaders/default.vs");
      vertexShader = VertexShaderPtr(new VertexShader());
      vertexShader->Source(vertexShaderSource.toLocal8Bit().constData());
      vertexShader->Compile();
    }

    QString header = st::SHADER_HEADER;
    for (int i = 0; i < 4; ++i) {
      const Channel & channel = channels[i];
      QString line; line.sprintf("uniform sampler%s iChannel%d;\n",
                                 channel.target == Texture::Target::CubeMap ? "Cube" : "2D", i);
      header += line;
    }
    header += st::LINE_NUMBER_HEADER;
    FragmentShaderPtr newFragmentShader(new FragmentShader());
    vrMode = source.contains("#pragma vr");
    source.
        replace(QRegExp("\\t"), "  ").
        replace(QRegExp("\\bgl_FragColor\\b"), "FragColor").
        replace(QRegExp("\\btexture2D\\b"), "texture").
        replace(QRegExp("\\btextureCube\\b"), "texture");
    source.insert(0, header);
    QByteArray qb = source.toLocal8Bit();
    GLchar * fragmentSource = (GLchar*)qb.data();
    StrCRef src(fragmentSource);
    newFragmentShader->Source(GLSLSource(src));
    newFragmentShader->Compile();
    ProgramPtr result(new Program());
    result->AttachShader(*vertexShader);
    result->AttachShader(*newFragmentShader);

    result->Link();
    shadertoyProgram.swap(result);
    if (!skybox) {
      skybox = oria::loadSkybox(shadertoyProgram);
    }
    fragmentShader.swap(newFragmentShader);
    updateUniforms();
    shaderTime.start();
    emit compileSuccess();
  } catch (ProgramBuildError & err) {
    emit compileError(QString(err.Log().c_str()));
    return false;
  }
  return true;
}

ShadertoyRenderer::TextureData ShadertoyRenderer::loadTexture(QString source) {
  qDebug() << "Looking for texture " << source;
  while (canonicalPathMap.count(source)) {
    source = canonicalPathMap[source];
  }

  if (!textureCache.count(source)) {
    TextureData & textureData = textureCache[source];
    qWarning() << "Texture " << source << " not found, loading";
    textureData.tex = oria::load2dTexture(source, textureData.size);
  }
  return textureCache[source];
}

void ShadertoyRenderer::setChannelTextureInternal(int channel, st::ChannelInputType type, const QString & textureSource) {
  using namespace oglplus;
  if (textureSource == channelSources[channel]) {
    return;
  }

  channelSources[channel] = textureSource;

  if (QUrl() == textureSource) {
    channels[channel].texture.reset();
    channels[channel].target = Texture::Target::_2D;
    return;
  }

  Channel newChannel;
  uvec2 size;
  auto texData = loadTexture(textureSource);
  newChannel.texture = texData.tex;
  switch (type) {
  case st::ChannelInputType::TEXTURE:
    newChannel.target = Texture::Target::_2D;
    newChannel.resolution = vec3(texData.size, 0);
    break;

  case st::ChannelInputType::CUBEMAP:
    newChannel.target = Texture::Target::CubeMap;
    newChannel.resolution = vec3(texData.size.x);
    break;

  case st::ChannelInputType::VIDEO:
    // FIXME, not supported
    break;

  case st::ChannelInputType::AUDIO:
    // FIXME, not supported
    break;
  }

  channels[channel] = newChannel;
}

void ShadertoyRenderer::setShaderInternal(const st::Shader & shader) {
  for (int i = 0; i < st::MAX_CHANNELS; ++i) {
    setChannelTextureInternal(i, shader.channelTypes[i], shader.channelTextures[i]);
  }
  setShaderSourceInternal(shader.fragmentSource);
}

