#pragma once
#include "Common.h"

#include "Shadertoy.h"

class ShadertoyFetcher : public QObject {
  Q_OBJECT

  QQueue<QString> shadersToFetch;
  QTimer timer;
  int currentNetworkRequests{ 0 };
  QDir destinationFolder;

  void fetchUrl(const QUrl & url, std::function<void(QByteArray)> f);
  void fetchFile(const QUrl & url, const QString & path);
  void fetchNextShader();

public:
  ShadertoyFetcher();
  void setDestination(const QDir & folder);
  void fetchShaders();
};
