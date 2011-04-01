/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef TRACEWINDOW_H
#define TRACEWINDOW_H

#include <QtCore/qpointer.h>
#include <QtGui/qwidget.h>

#include <QtDeclarative/private/qdeclarativedebugclient_p.h>


QT_BEGIN_NAMESPACE
class QTabWidget;
class QSlider;
class QGroupBox;
class QLabel;
class QSpinBox;
class QPushButton;
class QDeclarativeView;
QT_END_NAMESPACE

namespace QmlProfiler {
namespace Internal {

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
    void updateTimer();

signals:
    void viewUpdated();
    void gotoSourceLocation(const QString &fileUrl, int lineNumber);
    void timeChanged(qreal newTime);

private:
    TracePlugin *m_plugin;
    QSize m_sizeHint;
    bool m_recordAtStart;

    QDeclarativeView *m_view;
};

} // namespace Internal
} // namespace QmlProfiler

#endif // TRACEWINDOW_H

