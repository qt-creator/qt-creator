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

#ifndef QDECLARATIVEDESIGNDEBUGSERVER_H
#define QDECLARATIVEDESIGNDEBUGSERVER_H

#include "qt_private/qdeclarativedebugservice_p.h"
#include "qmlinspectorconstants.h"
#include "qmljsdebugger_global.h"

#include <QHash>

QT_FORWARD_DECLARE_CLASS(QColor)
QT_FORWARD_DECLARE_CLASS(QDeclarativeEngine)
QT_FORWARD_DECLARE_CLASS(QDeclarativeContext)
QT_FORWARD_DECLARE_CLASS(QDeclarativeWatcher)
QT_FORWARD_DECLARE_CLASS(QDataStream)

namespace QmlJSDebugger {

class QMLJSDEBUGGER_EXPORT QDeclarativeInspectorService : public QDeclarativeDebugService
{
    Q_OBJECT
public:
    QDeclarativeInspectorService();
    static QDeclarativeInspectorService *instance();

    void setDesignModeBehavior(bool inDesignMode);
    void setCurrentObjects(QList<QObject*> items);
    void setAnimationSpeed(qreal slowDownFactor);
    void setAnimationPaused(bool paused);
    void setCurrentTool(QmlJSDebugger::Constants::DesignTool toolId);
    void reloaded();
    void setShowAppOnTop(bool showAppOnTop);

    QString idStringForObject(QObject *obj) const;

    void sendMessage(const QByteArray &message);

public Q_SLOTS:
    void selectedColorChanged(const QColor &color);

Q_SIGNALS:
    void debuggingClientChanged(bool hasDebuggingClient);

    void currentObjectsChanged(const QList<QObject*> &objects);
    void designModeBehaviorChanged(bool inDesignMode);
    void showAppOnTopChanged(bool showAppOnTop);
    void reloadRequested();
    void selectToolRequested();
    void selectMarqueeToolRequested();
    void zoomToolRequested();
    void colorPickerToolRequested();

    void objectCreationRequested(const QString &qml, QObject *parent,
                                 const QStringList &imports, const QString &filename = QString(), int order = -1);
    void objectReparentRequested(QObject *object, QObject *newParent);
    void objectDeletionRequested(QObject *object);

    // 1 = normal speed,
    // 1 < x < 16 = slowdown by some factor
    void animationSpeedChangeRequested(qreal speedFactor);
    void executionPauseChangeRequested(bool paused);

    void clearComponentCacheRequested();

protected:
    virtual void statusChanged(Status status);
    virtual void messageReceived(const QByteArray &);

private:
    QHash<int, QString> m_stringIdForObjectId;
};

} // namespace QmlJSDebugger

#endif // QDECLARATIVEDESIGNDEBUGSERVER_H
