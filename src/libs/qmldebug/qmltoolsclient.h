/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLTOOLSCLIENT_H
#define QMLTOOLSCLIENT_H

#include "basetoolsclient.h"

namespace QmlDebug {

class QMLDEBUG_EXPORT QmlToolsClient : public BaseToolsClient
{
    Q_OBJECT
public:
    explicit QmlToolsClient(QmlDebugConnection *client);

    void setCurrentObjects(const QList<int> &debugIds);
    void reload(const QHash<QString, QByteArray> &changesHash);
    bool supportReload() const { return true; }
    void setDesignModeBehavior(bool inDesignMode);
    void setAnimationSpeed(qreal slowDownFactor);
    void setAnimationPaused(bool paused);
    void changeToSelectTool();
    void changeToSelectMarqueeTool();
    void changeToZoomTool();
    void showAppOnTop(bool showOnTop);

    void createQmlObject(const QString &qmlText, int parentDebugId,
                         const QStringList &imports, const QString &filename,
                         int order);
    void destroyQmlObject(int debugId);
    void reparentQmlObject(int debugId, int newParent);

    void applyChangesToQmlFile();
    void applyChangesFromQmlFile();

    QList<int> currentObjects() const;

    // ### Qt 4.8: remove if we can have access to qdeclarativecontextdata or id's
    void setObjectIdList(const QList<ObjectReference> &objectRoots);

    void clearComponentCache();

protected:
    void messageReceived(const QByteArray &);

private:
    void log(LogDirection direction,
             const QByteArray &message,
             const QString &extra = QString());

private:
    QList<int> m_currentDebugIds;
    QmlDebugConnection *m_connection;
    int m_requestId;
    qreal m_slowDownFactor;
    int m_reloadQueryId;
    int m_destroyObjectQueryId;
};

} // namespace QmlDebug

#endif // QMLTOOLSCLIENT_H
