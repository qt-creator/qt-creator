// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include "imagecontainer.h"

namespace QmlDesigner {

class PixmapChangedCommand
{
    friend QDataStream &operator>>(QDataStream &in, PixmapChangedCommand &command);
    friend bool operator ==(const PixmapChangedCommand &first, const PixmapChangedCommand &second);

public:
    PixmapChangedCommand();
    explicit PixmapChangedCommand(const QVector<ImageContainer> &imageVector);

    QVector<ImageContainer> images() const;

    void sort();

private:
    QVector<ImageContainer> m_imageVector;
};

QDataStream &operator<<(QDataStream &out, const PixmapChangedCommand &command);
QDataStream &operator>>(QDataStream &in, PixmapChangedCommand &command);

bool operator ==(const PixmapChangedCommand &first, const PixmapChangedCommand &second);
QDebug operator <<(QDebug debug, const PixmapChangedCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::PixmapChangedCommand)
