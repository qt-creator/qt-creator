// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QQuickImageProvider>

namespace QmlDesigner {

namespace Internal {

class ItemLibraryImageProvider : public QQuickImageProvider
{
public:
    ItemLibraryImageProvider();

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};

}

}
