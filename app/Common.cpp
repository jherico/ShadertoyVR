#include "Common.h"
#include "Resources.h"

static QList<Lambda> shutdownHooks;
void Platform::addShutdownHook(Lambda f) {
  shutdownHooks.push_back(f);
}

void Platform::runShutdownHooks() {
  foreach(Lambda f, shutdownHooks) {
    f();
  }
}

const QString Resource::SHADERS_TEXTURED_VS = ":/shaders/Textured.vs";
const QString Resource::SHADERS_TEXTURED_FS = ":/shaders/Textured.fs";
