// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "elidinglabel.h"

#include <QMouseEvent>

namespace ADS {

/**
 * Private data of public ElidingLabel
 */
class ElidingLabelPrivate
{
public:
    ElidingLabel *q;
    Qt::TextElideMode m_elideMode = Qt::ElideNone;
    QString m_text;
    bool m_isElided = false;

    ElidingLabelPrivate(ElidingLabel *parent)
        : q(parent)
    {}

    void elideText(int width);

    /**
     * Convenience function to check if the
     */
    bool isModeElideNone() const { return Qt::ElideNone == m_elideMode; }
};

void ElidingLabelPrivate::elideText(int width)
{
    if (isModeElideNone())
        return;

    QFontMetrics fm = q->fontMetrics();
    QString str = fm.elidedText(m_text, m_elideMode, width - q->margin() * 2 - q->indent());
    if (str == u'\u2026')
        str = m_text.at(0);

    bool wasElided = m_isElided;
    m_isElided = (str != m_text);
    if (m_isElided != wasElided)
        emit q->elidedChanged(m_isElided);

    q->QLabel::setText(str);
}

ElidingLabel::ElidingLabel(QWidget *parent, Qt::WindowFlags flags)
    : QLabel(parent, flags)
    , d(new ElidingLabelPrivate(this))
{}

ElidingLabel::ElidingLabel(const QString &text, QWidget *parent, Qt::WindowFlags flags)
    : QLabel(text, parent, flags)
    , d(new ElidingLabelPrivate(this))
{
    d->m_text = text;
    internal::setToolTip(this, text);
}

ElidingLabel::~ElidingLabel()
{
    delete d;
}

Qt::TextElideMode ElidingLabel::elideMode() const
{
    return d->m_elideMode;
}

void ElidingLabel::setElideMode(Qt::TextElideMode mode)
{
    d->m_elideMode = mode;
    d->elideText(size().width());
}

bool ElidingLabel::isElided() const
{
    return d->m_isElided;
}

void ElidingLabel::mouseReleaseEvent(QMouseEvent *event)
{
    Super::mouseReleaseEvent(event);
    if (event->button() != Qt::LeftButton)
        return;

    emit clicked();
}

void ElidingLabel::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    emit doubleClicked();
    Super::mouseDoubleClickEvent(event);
}

void ElidingLabel::resizeEvent(QResizeEvent *event)
{
    if (!d->isModeElideNone())
        d->elideText(event->size().width());

    Super::resizeEvent(event);
}

QSize ElidingLabel::minimumSizeHint() const
{
    if (hasPixmap() || d->isModeElideNone())
        return QLabel::minimumSizeHint();

    const QFontMetrics &fm = fontMetrics();
    QSize size(fm.horizontalAdvance(d->m_text.left(2) + "…"), fm.height());
    return size;
}

QSize ElidingLabel::sizeHint() const
{
    if (hasPixmap() || d->isModeElideNone())
        return QLabel::sizeHint();

    const QFontMetrics &fm = fontMetrics();
    QSize size(fm.horizontalAdvance(d->m_text), QLabel::sizeHint().height());
    return size;
}

void ElidingLabel::setText(const QString &text)
{
    d->m_text = text;
    if (d->isModeElideNone()) {
        Super::setText(text);
    } else {
        internal::setToolTip(this, text);
        d->elideText(size().width());
    }
}

QString ElidingLabel::text() const
{
    return d->m_text;
}

bool ElidingLabel::hasPixmap() const
{
    return !pixmap().isNull();
}

} // namespace ADS
