// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QPointer>
#include <QTime>
#include <QBasicTimer>
#include <QStyle>
#include <QPainter>
#include <QWidget>

/*
 * This is a set of helper classes to allow for widget animations in
 * the style. Its mostly taken from Vista style so it should be fully documented
 * there.
 *
 */

class Animation
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
class Transition : public Animation
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

class StyleAnimator : public QObject
{
    Q_OBJECT

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
