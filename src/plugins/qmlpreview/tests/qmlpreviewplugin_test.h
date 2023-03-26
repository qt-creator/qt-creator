// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace QmlPreview {

typedef QByteArray (*TestFileLoader)(const QString &, bool *);
typedef void (*TestFpsHandler)(quint16[8]);

class QmlPreviewPluginTest : public QObject
{
    Q_OBJECT
public:
    explicit QmlPreviewPluginTest(QObject *parent = nullptr);

private slots:
    void testFileLoaderProperty();
    void testZoomFactorProperty();
    void testFpsHandlerProperty();
};

} // namespace QmlPreview
