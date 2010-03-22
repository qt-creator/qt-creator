/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#ifndef CANVASFRAMERATE_H
#define CANVASFRAMERATE_H

#include <private/qdeclarativedebugclient_p.h>

#include <QtCore/qpointer.h>
#include <QtGui/qwidget.h>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QSlider;
class QGroupBox;
class QLabel;
class QSpinBox;
class QPushButton;
QT_END_NAMESPACE


namespace Qml {
namespace Internal {


class CanvasFrameRatePlugin;

class CanvasFrameRate : public QWidget
{
    Q_OBJECT
public:
    CanvasFrameRate(QWidget *parent = 0);

    void reset(QDeclarativeDebugConnection *conn);

    void setSizeHint(const QSize &);
    virtual QSize sizeHint() const;

signals:
    void contextHelpIdChanged(const QString &helpId);

private slots:
    void clearGraph();
    void newTab();
    void enabledToggled(bool);
    void connectionStateChanged(QAbstractSocket::SocketState state);

private:
    void handleConnected(QDeclarativeDebugConnection *conn);

    QGroupBox *m_group;
    QTabWidget *m_tabs;
    QSpinBox *m_res;
    QPushButton *m_clearButton;
    CanvasFrameRatePlugin *m_plugin;
    QSize m_sizeHint;
};

} // Internal
} // Qml

#endif // CANVASFRAMERATE_H

