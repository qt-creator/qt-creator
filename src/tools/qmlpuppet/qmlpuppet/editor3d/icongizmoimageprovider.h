// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtQuick/qquickimageprovider.h>

namespace QmlDesigner {
namespace Internal {

class IconGizmoImageProvider : public QQuickImageProvider
{
public:
    IconGizmoImageProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
};
}
}
