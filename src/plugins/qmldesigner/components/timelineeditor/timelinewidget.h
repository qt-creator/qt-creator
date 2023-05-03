// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelineutils.h"
#include <coreplugin/icontext.h>

#include <QWidget>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QGraphicsView)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QResizeEvent)
QT_FORWARD_DECLARE_CLASS(QShowEvent)
QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QVariantAnimation)
QT_FORWARD_DECLARE_CLASS(QScrollBar)
QT_FORWARD_DECLARE_CLASS(QAbstractScrollArea)

namespace Utils {
QT_FORWARD_DECLARE_CLASS(ScrollBar)
}

namespace QmlDesigner {

class TimelineToolBar;
class TimelineView;
class TimelineGraphicsScene;
class TimelineWidget;
class QmlTimeline;
class Navigation2dScrollBar;

namespace TimeLineNS {

class TimelineScrollAreaPrivate;
class ScrollBarPrivate;

class TimelineScrollAreaSupport : public QObject
{
    Q_OBJECT
public:
    static void support(QAbstractScrollArea *scrollArea, Utils::ScrollBar *scrollbar);
    virtual ~TimelineScrollAreaSupport();

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

private:
    explicit TimelineScrollAreaSupport(QAbstractScrollArea *scrollArea, Utils::ScrollBar *scrollbar);

    TimelineScrollAreaPrivate *d = nullptr;
};

} // namespace TimeLineNS

class TimelineWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineWidget(TimelineView *view);
    void contextHelp(const Core::IContext::HelpCallback &callback) const;

    TimelineGraphicsScene *graphicsScene() const;
    TimelineView *timelineView() const;
    TimelineToolBar *toolBar() const;

    void init(int zoom = 0);
    void reset();

    void invalidateTimelineDuration(const QmlTimeline &timeline);
    void invalidateTimelinePosition(const QmlTimeline &timeline);
    void setupScrollbar(int min, int max, int current);
    void setTimelineId(const QString &id);

    void setTimelineActive(bool b);
    void setFocus();

public slots:
    void selectionChanged();
    void openEasingCurveEditor();
    void toggleAnimationPlayback();
    void setTimelineRecording(bool value);
    void changeScaleFactor(int factor);
    void scroll(const TimelineUtils::Side &side);
    void updatePlaybackValues();

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void connectToolbar();

    int adjacentFrame(const std::function<qreal(const QVector<qreal> &, qreal)> &fun) const;

    TimelineToolBar *m_toolbar = nullptr;

    QGraphicsView *m_rulerView = nullptr;

    QGraphicsView *m_graphicsView = nullptr;

    Utils::ScrollBar *m_scrollbar = nullptr;

    QLabel *m_statusBar = nullptr;

    TimelineView *m_timelineView = nullptr;

    TimelineGraphicsScene *m_graphicsScene;

    QPushButton *m_addButton = nullptr;

    QWidget *m_onboardingContainer = nullptr;

    bool m_loopPlayback;
    qreal m_playbackSpeed;
    QVariantAnimation *m_playbackAnimation;
};

} // namespace QmlDesigner
