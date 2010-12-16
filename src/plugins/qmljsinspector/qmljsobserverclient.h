/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMLJSDESIGNDEBUGCLIENT_H
#define QMLJSDESIGNDEBUGCLIENT_H

#include "qmljsprivateapi.h"

namespace QmlJSInspector {
namespace Internal {

class QmlJSObserverClient : public QDeclarativeDebugClient
{
    Q_OBJECT
public:
    explicit QmlJSObserverClient(QDeclarativeDebugConnection *client,
                                    QObject *parent = 0);

    void setCurrentObjects(const QList<int> &debugIds);
    void reloadViewer();
    void setDesignModeBehavior(bool inDesignMode);
    void setAnimationSpeed(qreal slowdownFactor);
    void changeToColorPickerTool();
    void changeToSelectTool();
    void changeToSelectMarqueeTool();
    void changeToZoomTool();
    void showAppOnTop(bool showOnTop);

    void createQmlObject(const QString &qmlText, int parentDebugId,
                         const QStringList &imports, const QString &filename);
    void destroyQmlObject(int debugId);
    void reparentQmlObject(int debugId, int newParent);

    void applyChangesToQmlFile();
    void applyChangesFromQmlFile();

    QList<int> currentObjects() const;

    // ### Qt 4.8: remove if we can have access to qdeclarativecontextdata or id's
    void setObjectIdList(const QList<QDeclarativeDebugObjectReference> &objectRoots);

    void setContextPathIndex(int contextPathIndex);
    void clearComponentCache();

signals:
    void connectedStatusChanged(QDeclarativeDebugClient::Status status);

    void currentObjectsChanged(const QList<int> &debugIds);
    void selectedColorChanged(const QColor &color);
    void colorPickerActivated();
    void selectToolActivated();
    void selectMarqueeToolActivated();
    void zoomToolActivated();
    void animationSpeedChanged(qreal slowdownFactor);
    void designModeBehaviorChanged(bool inDesignMode);
    void showAppOnTopChanged(bool showAppOnTop);
    void reloaded(); // the server has reloadetd he document
    void contextPathUpdated(const QStringList &path);

    void logActivity(QString client, QString message);

protected:
    virtual void statusChanged(Status);
    virtual void messageReceived(const QByteArray &);

private:
    enum LogDirection {
        LogSend,
        LogReceive
    };

    void log(LogDirection direction, const QString &str);

    QList<int> m_currentDebugIds;
    QDeclarativeDebugConnection *m_connection;
};

} // namespace Internal
} // namespace QmlJSInspector

#endif // QMLJSDESIGNDEBUGCLIENT_H
