/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
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

#ifndef TRACEWINDOW_H
#define TRACEWINDOW_H

#include "qmlprofilertraceclient.h"

#include <QtCore/QPointer>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QDeclarativeView;
QT_END_NAMESPACE

namespace QmlProfiler {
namespace Internal {

class TraceWindow : public QWidget
{
    Q_OBJECT

public:
    TraceWindow(QWidget *parent = 0);
    ~TraceWindow();

    void reset(QmlJsDebugClient::QDeclarativeDebugConnection *conn);

    void setRecording(bool recording);
    bool isRecording() const;


public slots:
    void updateCursorPosition();
    void updateTimer();
    void clearDisplay();
    void updateToolbar();

signals:
    void viewUpdated();
    void gotoSourceLocation(const QString &fileUrl, int lineNumber);
    void timeChanged(qreal newTime);
    void range(int type, int nestingLevel, int nestingInType, qint64 startTime, qint64 length, const QStringList &data, const QString &fileName, int line);

    void internalClearDisplay();
    void jumpToPrev();
    void jumpToNext();
    void zoomIn();
    void zoomOut();
    void enableToolbar(bool);

private:
    QWeakPointer<QmlProfilerTraceClient> m_plugin;
    QSize m_sizeHint;

    QDeclarativeView *m_view;
};

} // namespace Internal
} // namespace QmlProfiler

#endif // TRACEWINDOW_H

