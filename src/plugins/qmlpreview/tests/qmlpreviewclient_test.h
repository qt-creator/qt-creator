// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QObject>

namespace QmlPreview {

class QmlPreviewClientTest : public QObject
{
    Q_OBJECT

private slots:
    void testLoadFile();
    void testAnnounceFile();
    void testZoom();
    void testMessageReceived();
};

} // namespace QmlPreview
