// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlpreview_global.h"
#include <qmldebug/qmldebugclient.h>

namespace QmlPreview {

class QMLPREVIEW_EXPORT QmlDebugTranslationClient : public QmlDebug::QmlDebugClient
{
    Q_OBJECT
public:
    explicit QmlDebugTranslationClient(QmlDebug::QmlDebugConnection *connection);

    void changeLanguage(const QUrl &url, const QString &localeIsoCode);
    void stateChanged(State state) override;

signals:
    void debugServiceUnavailable();
};

} // namespace QmlPreview
