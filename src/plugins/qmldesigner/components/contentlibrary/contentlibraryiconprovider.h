// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QQuickImageProvider>

namespace QmlDesigner::Internal {

class ContentLibraryIconProvider : public QQuickImageProvider
{
public:
    ContentLibraryIconProvider();
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};

} // namespace QmlDesigner::Internal
