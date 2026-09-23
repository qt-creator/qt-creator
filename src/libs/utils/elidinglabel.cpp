// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "elidinglabel.h"

#include <QFontMetrics>
#include <QPainter>
#include <QStyle>

/*!
    \class Utils::ElidingLabel
    \inmodule QtCreator

    \brief The ElidingLabel class is a label suitable for displaying elided
    text.
*/

namespace Utils {

ElidingLabel::ElidingLabel(QWidget *parent)
    : ElidingLabel({}, parent)
{
}

ElidingLabel::ElidingLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent)
{
    setElideMode(Qt::ElideRight);
}

Qt::TextElideMode ElidingLabel::elideMode() const
{
    return m_elideMode;
}

void ElidingLabel::setElideMode(const Qt::TextElideMode &elideMode)
{
    m_elideMode = elideMode;
    if (elideMode == Qt::ElideNone)
        updateToolTip({});

    setSizePolicy(QSizePolicy(
                      m_elideMode == Qt::ElideNone ? QSizePolicy::Preferred : QSizePolicy::Ignored,
                      QSizePolicy::Preferred,
                      QSizePolicy::Label));
    update();
}

QString ElidingLabel::additionalToolTip()
{
    return m_additionalToolTip;
}

void ElidingLabel::setAdditionalToolTip(const QString &additionalToolTip)
{
    m_additionalToolTip = additionalToolTip;
}

void ElidingLabel::paintEvent(QPaintEvent *)
{
    if (m_elideMode == Qt::ElideNone) {
        QLabel::paintEvent(nullptr);
        updateToolTip({});
        return;
    }

    const int m = margin();
    const QRect contents = contentsRect().adjusted(m, m, -m, -m);
    const QFontMetrics fm = fontMetrics();
    const QString txt = text();
    const bool elide = txt.size() > 4 && fm.horizontalAdvance(txt) > contents.width();

    // QLabelPrivate::layoutRect() clamps the vertical offset at 0, so a label
    // shorter than its text loses Qt::AlignBottom, and the descender with it.
    // Equality is the normal case: it is what sizeHint() asks for.
    const bool fitsVertically = fm.height() <= contents.height();

    if (!elide && fitsVertically) {
        // Nothing to truncate: let QLabel paint so that features handled by the
        // base class, such as text selection, are rendered normally.
        QLabel::paintEvent(nullptr);
        updateToolTip({});
        return;
    }

    updateToolTip(elide ? txt : QString());
    const QString shown = elide ? fm.elidedText(txt, m_elideMode, contents.width()) : txt;
    const int flags = QStyle::visualAlignment(layoutDirection(), alignment()) | Qt::TextSingleLine;

    QPainter painter(this);
    drawFrame(&painter);
    painter.drawText(contents, flags, shown);
}

void ElidingLabel::updateToolTip(const QString &elidedText)
{
    if (m_additionalToolTip.isEmpty()) {
        setToolTip(elidedText);
        return;
    }

    if (elidedText.isEmpty()) {
        setToolTip(m_additionalToolTip);
        return;
    }

    setToolTip(elidedText + m_additionalToolTipSeparator + m_additionalToolTip);
}

QString ElidingLabel::additionalToolTipSeparator() const
{
    return m_additionalToolTipSeparator;
}

void ElidingLabel::setAdditionalToolTipSeparator(const QString &newAdditionalToolTipSeparator)
{
    m_additionalToolTipSeparator = newAdditionalToolTipSeparator;
}

static int singleLineHeight(const QLabel *label)
{
    const QMargins margins = label->contentsMargins();
    return label->fontMetrics().height() + 2 * label->margin() + margins.top() + margins.bottom();
}

QSize ElidingLabel::sizeHint() const
{
    if (m_elideMode == Qt::ElideNone)
        return QLabel::sizeHint();
    return {QLabel::sizeHint().width(), singleLineHeight(this)};
}

QSize ElidingLabel::minimumSizeHint() const
{
    if (m_elideMode == Qt::ElideNone)
        return QLabel::minimumSizeHint();
    return {QLabel::minimumSizeHint().width(), singleLineHeight(this)};
}

} // namespace Utils
