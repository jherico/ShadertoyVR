#include "Common.h"
#include "ShadertoyConfig.h"
#include "ShadertoyFetcher.h"
#include <QNetworkAccessManager>
#include <QtNetwork>

static QNetworkAccessManager qnam;
static const QString SHADERTOY_API_URL = "https://www.shadertoy.com/api/v1/shaders";
static const QString SHADERTOY_MEDIA_URL = "https://www.shadertoy.com/media/shaders/";

void ShadertoyFetcher::fetchUrl(const QUrl & url, std::function<void(QByteArray)> f) {
  QNetworkRequest request(url);
  qDebug() << "Requesting url " << url;
  request.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, "ShadertoyVR/1.0");
  ++currentNetworkRequests;
  QNetworkReply * netReply = qnam.get(request);
  connect(netReply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
          this, [&, url](QNetworkReply::NetworkError code) {
    qWarning() << "Got error " << code << " fetching url " << url;
  });
  connect(netReply, &QNetworkReply::finished, this, [&, f, netReply, url] {
    --currentNetworkRequests;
    qDebug() << "Got response for url " << url;
    QByteArray replyBuffer = netReply->readAll();
    f(replyBuffer);
  });
}

void ShadertoyFetcher::fetchFile(const QUrl & url, const QString & path) {
  fetchUrl(url, [&, path](const QByteArray & replyBuffer) {
    QFile outputFile(path);
    outputFile.open(QIODevice::WriteOnly);
    outputFile.write(replyBuffer);
    outputFile.close();
  });
}

void ShadertoyFetcher::fetchNextShader() {
#ifdef SHADERTOY_API_KEY
  while (!shadersToFetch.empty() && currentNetworkRequests <= 4) {
    QString nextShaderId = shadersToFetch.front();
    shadersToFetch.pop_front();
    QString shaderFile = destinationFolder.absoluteFilePath(nextShaderId + ".json");
    QString shaderPreviewFile = destinationFolder.absoluteFilePath(nextShaderId + ".jpg");
    if (QFile(shaderFile).exists() && QFile(shaderPreviewFile).exists()) {
      continue;
    }

    if (!QFile(shaderFile).exists()) {
      qDebug() << "Fetching shader " << nextShaderId;
      QUrl url(SHADERTOY_API_URL + QString().sprintf("/%s?key=%s", nextShaderId.toLocal8Bit().constData(), SHADERTOY_API_KEY));
      fetchUrl(url, [&, shaderFile](const QByteArray & replyBuffer) {
        QFile outputFile(shaderFile);
        outputFile.open(QIODevice::WriteOnly);
        outputFile.write(replyBuffer);
        outputFile.close();
      });
    }

    if (!QFile(shaderPreviewFile).exists()) {
      fetchFile(QUrl(SHADERTOY_MEDIA_URL + nextShaderId + ".jpg"), shaderPreviewFile);
    }
  }

  if (shadersToFetch.isEmpty()) {
    timer.stop();
    return;
  }
#endif
}

ShadertoyFetcher::ShadertoyFetcher()  {
  connect(&timer, &QTimer::timeout, this, [&] {
    fetchNextShader();
  });
}

void ShadertoyFetcher::setDestination(const QDir & folder) {
  destinationFolder = folder;
}

void ShadertoyFetcher::fetchShaders() {
#ifdef SHADERTOY_API_KEY
  qDebug() << "Fetching shader list";
  QUrl url(SHADERTOY_API_URL + QString().sprintf("?key=%s", SHADERTOY_API_KEY));
  fetchUrl(url, [&](const QByteArray & replyBuffer) {
    QJsonDocument jsonResponse = QJsonDocument::fromJson(replyBuffer);
    QJsonObject jsonObject = jsonResponse.object();
    QJsonArray shaders = jsonObject["Results"].toArray();
    for (int i = 0; i < shaders.count(); ++i) {
      QString shaderId = shaders.at(i).toString();
      shadersToFetch.push_back(shaderId);
    }
    timer.start(1000);
  });
#endif
}

