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

class QTabWidget;
class QSlider;
class QGroupBox;
class QLabel;
class QSpinBox;
class QPushButton;
class QDeclarativeView;

class TracePlugin;

class TraceWindow : public QWidget
{
    Q_OBJECT
public:
    TraceWindow(QWidget *parent = 0);
    ~TraceWindow();

    void reset(QDeclarativeDebugConnection *conn);
    void setRecordAtStart(bool record);

    void setRecording(bool recording);

public slots:
    void updateCursorPosition();

signals:
    void viewUpdated();
    void gotoSourceLocation(const QString &fileName, int lineNumber);

private:
    TracePlugin *m_plugin;
    QSize m_sizeHint;
    bool m_recordAtStart;

    QDeclarativeView *m_view;
};

#endif // CANVASFRAMERATE_H

