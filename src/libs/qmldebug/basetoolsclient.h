/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

    // ### Qt 4.8: remove if we can have access to qdeclarativecontextdata or id's
    virtual void setObjectIdList(
            const QList<ObjectReference> &objectRoots) = 0;

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
