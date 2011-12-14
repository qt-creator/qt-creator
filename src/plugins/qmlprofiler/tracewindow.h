/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef TRACEWINDOW_H
#define TRACEWINDOW_H

#include <qmljsdebugclient/qmlprofilertraceclient.h>
#include <qmljsdebugclient/qmlprofilereventlist.h>

#include <qmljsdebugclient/qv8profilerclient.h>

#include <QtCore/QPointer>
#include <QtGui/QWidget>
#include <QtGui/QToolButton>

#include <QtCore/QEvent>
#include <QtDeclarative/QDeclarativeView>

namespace QmlProfiler {
namespace Internal {

class MouseWheelResizer : public QObject {
    Q_OBJECT
public:
    MouseWheelResizer(QObject *parent=0):QObject(parent){}
protected:
    bool eventFilter(QObject *obj, QEvent *event);
signals:
    void mouseWheelMoved(int x, int y, int delta);
};

// centralized zoom control
class ZoomControl : public QObject {
    Q_OBJECT
public:
    ZoomControl(QObject *parent=0):QObject(parent),m_startTime(0),m_endTime(0) {}
    ~ZoomControl(){}

    Q_INVOKABLE void setRange(qint64 startTime, qint64 endTime);
    Q_INVOKABLE qint64 startTime() { return m_startTime; }
    Q_INVOKABLE qint64 endTime() { return m_endTime; }

signals:
    void rangeChanged();

private:
    qint64 m_startTime;
    qint64 m_endTime;
};

class ScrollableDeclarativeView : public QDeclarativeView
{
    Q_OBJECT
public:
    explicit ScrollableDeclarativeView(QWidget *parent = 0);
    ~ScrollableDeclarativeView();
protected:
    void scrollContentsBy(int dx, int dy);
};

class TraceWindow : public QWidget
{
    Q_OBJECT

public:
    TraceWindow(QWidget *parent = 0);
    ~TraceWindow();

    void reset(QmlJsDebugClient::QDeclarativeDebugConnection *conn);

    QmlJsDebugClient::QmlProfilerEventList *getEventList() const;
    ZoomControl *rangeTimes() const;

    void setRecording(bool recording);
    bool isRecording() const;
    void viewAll();

    bool hasValidSelection() const;
    qint64 selectionStart() const;
    qint64 selectionEnd() const;
    double profiledTime() const;

public slots:
    void updateCursorPosition();
    void updateTimer();
    void clearDisplay();
    void updateToolbar();
    void toggleRangeMode(bool);
    void toggleLockMode(bool);
    void updateRangeButton();
    void updateLockButton();
    void setZoomLevel(int zoomLevel);
    void updateRange();
    void mouseWheelMoved(int x, int y, int delta);

    void qmlComplete();
    void v8Complete();
    void selectNextEvent(int eventId);
    void updateProfilerState();

signals:
    void viewUpdated();
    void profilerStateChanged(bool qmlActive, bool v8active);
    void gotoSourceLocation(const QString &fileUrl, int lineNumber);
    void range(int type, qint64 startTime, qint64 length, const QStringList &data, const QString &fileName, int line);
    void v8range(int depth,const QString &function,const QString &filename,
               int lineNumber, double totalTime, double selfTime);
    void traceFinished(qint64);
    void traceStarted(qint64);
    void frameEvent(qint64, int, int);

    void internalClearDisplay();
    void jumpToPrev();
    void jumpToNext();
    void rangeModeChanged(bool);
    void lockModeChanged(bool);
    void enableToolbar(bool);
    void zoomLevelChanged(int);
    void updateViewZoom(QVariant zoomLevel);
    void wheelZoom(QVariant wheelCenter, QVariant wheelDelta);
    void globalZoom();

    void contextMenuRequested(const QPoint& position);
    void selectNextEventInDisplay(QVariant eventId);
    void selectedEventIdChanged(int eventId);

private:
    void contextMenuEvent(QContextMenuEvent *);
    QWidget *createToolbar();
    QWidget *createZoomToolbar();
    void connectClientSignals();
    void disconnectClientSignals();

protected:
    virtual void resizeEvent(QResizeEvent *event);

private:
    QWeakPointer<QmlJsDebugClient::QmlProfilerTraceClient> m_plugin;
    QWeakPointer<QmlJsDebugClient::QV8ProfilerClient> m_v8plugin;
    QSize m_sizeHint;

    ScrollableDeclarativeView *m_mainView;
    QDeclarativeView *m_timebar;
    QDeclarativeView *m_overview;
    QmlJsDebugClient::QmlProfilerEventList *m_eventList;
    bool m_qmlDataReady;
    bool m_v8DataReady;
    double m_profiledTime;

    QWeakPointer<ZoomControl> m_zoomControl;

    QToolButton *m_buttonRange;
    QToolButton *m_buttonLock;
    QWidget *m_zoomToolbar;
    int m_currentZoomLevel;
};

} // namespace Internal
} // namespace QmlProfiler

#endif // TRACEWINDOW_H

