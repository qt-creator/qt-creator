/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BASETOOLSCLIENT_H
#define BASETOOLSCLIENT_H

#include "qmldebugclient.h"
#include "baseenginedebugclient.h"

namespace QmlDebug {

class QMLDEBUG_EXPORT BaseToolsClient : public QmlDebugClient
{
    Q_OBJECT
public:
    BaseToolsClient(QmlDebugConnection *client, QLatin1String clientName);

    virtual void setCurrentObjects(const QList<int> &debugIds) = 0;
    virtual void reload(const QHash<QString, QByteArray> &changesHash) = 0;
    virtual bool supportReload() const = 0;
    virtual void setDesignModeBehavior(bool inDesignMode) = 0;
    virtual void setAnimationSpeed(qreal slowDownFactor) = 0;
    virtual void setAnimationPaused(bool paused) = 0;
    virtual void changeToSelectTool() = 0;
    virtual void changeToSelectMarqueeTool() = 0;
    virtual void changeToZoomTool() = 0;
    virtual void showAppOnTop(bool showOnTop) = 0;

    virtual void createQmlObject(const QString &qmlText, int parentDebugId,
                         const QStringList &imports, const QString &filename,
                         int order) = 0;
    virtual void destroyQmlObject(int debugId) = 0;
    virtual void reparentQmlObject(int debugId, int newParent) = 0;

    virtual void applyChangesToQmlFile() = 0;
    virtual void applyChangesFromQmlFile() = 0;

    virtual QList<int> currentObjects() const = 0;

    // ### Qt 4.8: remove if we can have access to qdeclarativecontextdata or id's
    virtual void setObjectIdList(
            const QList<ObjectReference> &objectRoots) = 0;

    virtual void clearComponentCache() = 0;

signals:
    void newStatus(QmlDebug::ClientStatus status);

    void currentObjectsChanged(const QList<int> &debugIds);
    void selectToolActivated();
    void selectMarqueeToolActivated();
    void zoomToolActivated();
    void animationSpeedChanged(qreal slowdownFactor);
    void animationPausedChanged(bool paused);
    void designModeBehaviorChanged(bool inDesignMode);
    void showAppOnTopChanged(bool showAppOnTop);
    void reloaded(); // the server has reloaded the document
    void destroyedObject(int);

    void logActivity(QString client, QString message);

protected:
    void statusChanged(ClientStatus status);

    void recurseObjectIdList(const ObjectReference &ref,
                             QList<int> &debugIds, QList<QString> &objectIds);
protected:
    enum LogDirection {
        LogSend,
        LogReceive
    };
};

} // namespace QmlDebug

#endif // BASETOOLSCLIENT_H
