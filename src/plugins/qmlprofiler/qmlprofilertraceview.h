/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QMLPROFILERTRACEVIEW_H
#define QMLPROFILERTRACEVIEW_H

#include <QQuickView>
#include <QWidget>

namespace Analyzer { class IAnalyzerTool; }

namespace QmlProfiler {

class QmlProfilerModelManager;
namespace Internal {

class QmlProfilerStateManager;
class QmlProfilerViewManager;

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

class QmlProfilerTraceView : public QWidget
{
    Q_OBJECT

public:
    explicit QmlProfilerTraceView(QWidget *parent, Analyzer::IAnalyzerTool *profilerTool, QmlProfilerViewManager *container, QmlProfilerModelManager *modelManager, QmlProfilerStateManager *profilerState);
    ~QmlProfilerTraceView();

    void reset();

    bool hasValidSelection() const;
    qint64 selectionStart() const;
    qint64 selectionEnd() const;
    void showContextMenu(QPoint position);

public slots:
    void clearDisplay();
    void selectNextEventByHash(const QString &eventHash);
    void selectNextEventByLocation(const QString &filename, const int line, const int column);

private slots:
    void updateCursorPosition();
    void toggleRangeMode(bool);
    void updateRangeButton();
    void toggleLockMode(bool);
    void updateLockButton();

    void updateRange();

    void updateToolTip(const QString &text);
    void profilerDataModelStateChanged();

protected:
    virtual void resizeEvent(QResizeEvent *event);
    virtual void contextMenuEvent(QContextMenuEvent *event);

private slots:
    void setZoomSliderEnabled(bool enabled);
    void setZoomSliderVisible(bool visible);

signals:
    void gotoSourceLocation(const QString &fileUrl, int lineNumber, int columNumber);
    void selectedEventChanged(int eventId);

    void jumpToPrev();
    void jumpToNext();
    void rangeModeChanged(bool);
    void lockModeChanged(bool);
    void enableToolbar(bool);
    void showZoomSlider(bool);

    void resized();

private:
    QWidget *createToolbar();

private:
    class QmlProfilerTraceViewPrivate;
    QmlProfilerTraceViewPrivate *d;
};

class QmlProfilerQuickView : public QQuickView {
public:
    QmlProfilerQuickView(QmlProfilerTraceView *parent) : parent(parent) {}
protected:
    QmlProfilerTraceView *parent;
    bool event(QEvent *ev);
};

} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILERTRACEVIEW_H

