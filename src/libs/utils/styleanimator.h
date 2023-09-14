// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QAbstractAnimation>
#include <QBasicTimer>
#include <QPointer>
#include <QTime>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOption;
class QTimerEvent;
QT_END_NAMESPACE

namespace Utils {
/*
 * This is a set of helper classes to allow for widget animations in
 * the style. Its mostly taken from Vista style so it should be fully documented
 * there.
 *
 */

class QTCREATOR_UTILS_EXPORT Animation
{
public :
    Animation() = default;
    virtual ~Animation() = default;
    QWidget * widget() const { return m_widget; }
    bool running() const { return m_running; }
    const QTime &startTime() const { return m_startTime; }
    void setRunning(bool val) { m_running = val; }
    void setWidget(QWidget *widget) { m_widget = widget; }
    void setStartTime(const QTime &startTime) { m_startTime = startTime; }
    virtual void paint(QPainter *painter, const QStyleOption *option);

protected:
    void drawBlendedImage(QPainter *painter, const QRect &rect, float value);
    QTime m_startTime;
    QPointer<QWidget> m_widget;
    QImage m_primaryImage;
    QImage m_secondaryImage;
    QImage m_tempImage;
    bool m_running = true;
};

// Handles state transition animations
class QTCREATOR_UTILS_EXPORT Transition : public Animation
{
public :
    Transition() = default;
    ~Transition() override = default;
    void setDuration(int duration) { m_duration = duration; }
    void setStartImage(const QImage &image) { m_primaryImage = image; }
    void setEndImage(const QImage &image) { m_secondaryImage = image; }
    void paint(QPainter *painter, const QStyleOption *option) override;
    int duration() const { return m_duration; }
    int m_duration = 100; //set time in ms to complete a state transition
};

class QTCREATOR_UTILS_EXPORT StyleAnimator : public QObject
{
public:
    StyleAnimator(QObject *parent = nullptr) : QObject(parent) {}

    void timerEvent(QTimerEvent *) override;
    void startAnimation(Animation *);
    void stopAnimation(const QWidget *);
    Animation* widgetAnimation(const QWidget *) const;

private:
    QBasicTimer animationTimer;
    QList <Animation*> animations;
};

class QTCREATOR_UTILS_EXPORT QStyleAnimation : public QAbstractAnimation
{
    Q_OBJECT
public:
    QStyleAnimation(QObject *target);
    virtual ~QStyleAnimation();
    QObject *target() const;
    int duration() const override;
    void setDuration(int duration);
    int delay() const;
    void setDelay(int delay);
    QTime startTime() const;
    void setStartTime(const QTime &time);
    enum FrameRate { DefaultFps, SixtyFps, ThirtyFps, TwentyFps, FifteenFps };
    FrameRate frameRate() const;
    void setFrameRate(FrameRate fps);
    void updateTarget();
public Q_SLOTS:
    void start();

protected:
    virtual bool isUpdateNeeded() const;
    virtual void updateCurrentTime(int time) override;

private:
    int m_delay;
    int m_duration;
    QTime m_startTime;
    FrameRate m_fps;
    int m_skip;
};

class QTCREATOR_UTILS_EXPORT QNumberStyleAnimation : public QStyleAnimation
{
    Q_OBJECT
public:
    QNumberStyleAnimation(QObject *target);
    qreal startValue() const;
    void setStartValue(qreal value);
    qreal endValue() const;
    void setEndValue(qreal value);
    qreal currentValue() const;

protected:
    bool isUpdateNeeded() const override;

private:
    qreal m_start;
    qreal m_end;
    mutable qreal m_prev;
};

class QTCREATOR_UTILS_EXPORT QScrollbarStyleAnimation : public QNumberStyleAnimation
{
    Q_OBJECT
public:
    enum Mode { Activating, Deactivating };
    QScrollbarStyleAnimation(Mode mode, QObject *target);
    Mode mode() const;
    bool wasActive() const;
    bool wasAdjacent() const;
    void setActive(bool active);
    void setAdjacent(bool adjacent);

private slots:
    void updateCurrentTime(int time) override;

private:
    Mode m_mode;
    bool m_active;
    bool m_adjacent;
};

} // namespace Utils
