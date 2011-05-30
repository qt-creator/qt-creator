/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef QMLJSDESIGNDEBUGCLIENT_H
#define QMLJSDESIGNDEBUGCLIENT_H

#include "qmljsprivateapi.h"

#include <inspectorprotocol.h>

namespace QmlJSInspector {
namespace Internal {

class QmlJSInspectorClient : public QDeclarativeDebugClient
{
    Q_OBJECT
public:
    explicit QmlJSInspectorClient(QDeclarativeDebugConnection *client,
                                    QObject *parent = 0);

    void setCurrentObjects(const QList<int> &debugIds);
    void reloadViewer();
    void setDesignModeBehavior(bool inDesignMode);
    void setAnimationSpeed(qreal slowDownFactor);
    void setAnimationPaused(bool paused);
    void changeToColorPickerTool();
    void changeToSelectTool();
    void changeToSelectMarqueeTool();
    void changeToZoomTool();
    void showAppOnTop(bool showOnTop);

    void createQmlObject(const QString &qmlText, int parentDebugId,
                         const QStringList &imports, const QString &filename, int order);
    void destroyQmlObject(int debugId);
    void reparentQmlObject(int debugId, int newParent);

    void applyChangesToQmlFile();
    void applyChangesFromQmlFile();

    QList<int> currentObjects() const;

    // ### Qt 4.8: remove if we can have access to qdeclarativecontextdata or id's
    void setObjectIdList(const QList<QDeclarativeDebugObjectReference> &objectRoots);

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
    void animationPausedChanged(bool paused);
    void designModeBehaviorChanged(bool inDesignMode);
    void showAppOnTopChanged(bool showAppOnTop);
    void reloaded(); // the server has reloadetd he document

    void logActivity(QString client, QString message);

protected:
    void statusChanged(Status);
    void messageReceived(const QByteArray &);

private:
    enum LogDirection {
        LogSend,
        LogReceive
    };

    void log(LogDirection direction,
             QmlJSDebugger::InspectorProtocol::Message message,
             const QString &extra = QString());

    QList<int> m_currentDebugIds;
    QDeclarativeDebugConnection *m_connection;
};

} // namespace Internal
} // namespace QmlJSInspector

#endif // QMLJSDESIGNDEBUGCLIENT_H
