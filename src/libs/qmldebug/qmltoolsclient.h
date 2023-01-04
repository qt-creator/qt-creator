// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "basetoolsclient.h"

namespace QmlDebug {

class QMLDEBUG_EXPORT QmlToolsClient : public BaseToolsClient
{
    Q_OBJECT
public:
    explicit QmlToolsClient(QmlDebugConnection *client);

    void setDesignModeBehavior(bool inDesignMode) override;
    void changeToSelectTool() override;
    void changeToSelectMarqueeTool() override;
    void changeToZoomTool() override;
    void showAppOnTop(bool showOnTop) override;
    void selectObjects(const QList<int> &debugIds) override;
    void messageReceived(const QByteArray &) override;

private:
    void log(LogDirection direction,
             const QByteArray &message,
             const QString &extra = QString());

private:
    QmlDebugConnection *m_connection;
    int m_requestId;
};

} // namespace QmlDebug
