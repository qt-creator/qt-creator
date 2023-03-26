// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "styleanimator.h"

#include <utils/algorithm.h>

#include <QStyleOption>

Animation * StyleAnimator::widgetAnimation(const QWidget *widget) const
{
    if (!widget)
        return nullptr;
    return Utils::findOrDefault(animations, Utils::equal(&Animation::widget, widget));
}

void Animation::paint(QPainter *painter, const QStyleOption *option)
{
    Q_UNUSED(option)
    Q_UNUSED(painter)
}

void Animation::drawBlendedImage(QPainter *painter, const QRect &rect, float alpha)
{
    if (m_secondaryImage.isNull() || m_primaryImage.isNull())
        return;

    if (m_tempImage.isNull())
        m_tempImage = m_secondaryImage;

    const int a = qRound(alpha*256);
    const int ia = 256 - a;
    const int sw = m_primaryImage.width();
    const int sh = m_primaryImage.height();
    const int bpl = m_primaryImage.bytesPerLine();
    switch (m_primaryImage.depth()) {
    case 32:
        {
            uchar *mixed_data = m_tempImage.bits();
            const uchar *back_data = m_primaryImage.constBits();
            const uchar *front_data = m_secondaryImage.constBits();
            for (int sy = 0; sy < sh; sy++) {
                quint32 *mixed = (quint32*)mixed_data;
                const quint32* back = (const quint32*)back_data;
                const quint32* front = (const quint32*)front_data;
                for (int sx = 0; sx < sw; sx++) {
                    quint32 bp = back[sx];
                    quint32 fp = front[sx];
                    mixed[sx] =  qRgba ((qRed(bp)*ia + qRed(fp)*a)>>8,
                                        (qGreen(bp)*ia + qGreen(fp)*a)>>8,
                                        (qBlue(bp)*ia + qBlue(fp)*a)>>8,
                                        (qAlpha(bp)*ia + qAlpha(fp)*a)>>8);
                }
                mixed_data += bpl;
                back_data += bpl;
                front_data += bpl;
            }
        }
    default:
        break;
    }
    painter->drawImage(rect, m_tempImage);
}

void Transition::paint(QPainter *painter, const QStyleOption *option)
{
    float alpha = 1.0;
    if (m_duration > 0) {
        QTime current = QTime::currentTime();

        if (m_startTime > current)
            m_startTime = current;

        int timeDiff = m_startTime.msecsTo(current);
        alpha = timeDiff/(float)m_duration;
        if (timeDiff > m_duration) {
            m_running = false;
            alpha = 1.0;
        }
    } else {
        m_running = false;
    }
    drawBlendedImage(painter, option->rect, alpha);
}

void StyleAnimator::timerEvent(QTimerEvent *)
{
    for (int i = animations.size() - 1 ; i >= 0 ; --i) {
        if (animations[i]->widget())
            animations[i]->widget()->update();

        if (!animations[i]->widget() ||
            !animations[i]->widget()->isEnabled() ||
            !animations[i]->widget()->isVisible() ||
            animations[i]->widget()->window()->isMinimized() ||
            !animations[i]->running())
        {
            Animation *a = animations.takeAt(i);
            delete a;
        }
    }
    if (animations.size() == 0 && animationTimer.isActive())
        animationTimer.stop();
}

void StyleAnimator::stopAnimation(const QWidget *w)
{
    for (int i = animations.size() - 1 ; i >= 0 ; --i) {
        if (animations[i]->widget() == w) {
            Animation *a = animations.takeAt(i);
            delete a;
            break;
        }
    }
}

void StyleAnimator::startAnimation(Animation *t)
{
    stopAnimation(t->widget());
    animations.append(t);
    if (animations.size() > 0 && !animationTimer.isActive())
        animationTimer.start(35, this);
}
