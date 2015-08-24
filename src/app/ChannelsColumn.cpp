#include "Common.h"

#include "ChannelsColumn.h"

ChannelsColumn::ChannelsColumn(QQuickItem* parent) : QQuickItem(parent) {
    for (int i = 0; i < 4; ++i) {
        QString name = QString("channel") + i;
        _sources[i] = findChild<QQuickItem*>(name);
    }
}

ChannelsColumn::~ChannelsColumn() {
}

void ChannelsColumn::channelSelect(int index) {
    qDebug() << index;
    for (int i = 0; i < 4; ++i) {
        QString name = QString("channel") + i;
        _sources[i] = findChild<QQuickItem*>(name);
    }
}
