// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldebugclient.h"
#include "baseenginedebugclient.h"

namespace QmlDebug {

class QMLDEBUG_EXPORT BaseToolsClient : public QmlDebugClient
{
    Q_OBJECT
public:
    BaseToolsClient(QmlDebugConnection *client, QLatin1String clientName);

    virtual void setDesignModeBehavior(bool inDesignMode) = 0;
    virtual void changeToSelectTool() = 0;
    virtual void changeToSelectMarqueeTool() = 0;
    virtual void changeToZoomTool() = 0;
    virtual void showAppOnTop(bool showOnTop) = 0;
    virtual void selectObjects(const QList<int> &debugIds) = 0;

signals:
    void newState(QmlDebug::QmlDebugClient::State status);

    void currentObjectsChanged(const QList<int> &debugIds);
    void selectToolActivated();
    void selectMarqueeToolActivated();
    void zoomToolActivated();
    void designModeBehaviorChanged(bool inDesignMode);
    void reloaded(); // the server has reloaded the document

    void logActivity(QString client, QString message);

protected:
    void stateChanged(State status) override;

    void recurseObjectIdList(const ObjectReference &ref,
                             QList<int> &debugIds, QList<QString> &objectIds);
protected:
    enum LogDirection {
        LogSend,
        LogReceive
    };
};

} // namespace QmlDebug
