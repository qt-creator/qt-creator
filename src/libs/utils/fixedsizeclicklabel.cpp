// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fixedsizeclicklabel.h"

#include <QMouseEvent>

/*!
    \class Utils::FixedSizeClickLabel

    \brief The FixedSizeClickLabel class is a label with a size hint derived from a sample text
    that can be different to the text that is shown.

    For convenience it also has a clicked signal that is emitted whenever the label receives a mouse
    click.

    \inmodule Qt Creator
*/

/*!
    \fn Utils::FixedSizeClickLabel::clicked()

    This signal is emitted when the label is clicked with the left mouse button.
*/

/*!
    \property Utils::FixedSizeClickLabel::maxText

    This property holds the text that is used to calculate the label's size hint.
*/

namespace Utils {

/*!
    Constructs a FixedSizeClickLabel with the parent \a parent.
*/
FixedSizeClickLabel::FixedSizeClickLabel(QWidget *parent)
    : QLabel(parent)
{
}

/*!
    Sets the label's text to \a text, and changes the size hint of the label to the size of
    \a maxText.

    \sa maxText
    \sa setMaxText
*/
void FixedSizeClickLabel::setText(const QString &text, const QString &maxText)
{
    QLabel::setText(text);
    m_maxText = maxText;
}

/*!
    \reimp
*/
QSize FixedSizeClickLabel::sizeHint() const
{
    return fontMetrics().boundingRect(m_maxText).size();
}

QString FixedSizeClickLabel::maxText() const
{
    return m_maxText;
}

void FixedSizeClickLabel::setMaxText(const QString &maxText)
{
    m_maxText = maxText;
}

/*!
    \reimp
*/
void FixedSizeClickLabel::mousePressEvent(QMouseEvent *ev)
{
    QLabel::mousePressEvent(ev);
    if (ev->button() == Qt::LeftButton)
        m_pressed = true;
}

/*!
    \reimp
*/
void FixedSizeClickLabel::mouseReleaseEvent(QMouseEvent *ev)
{
    QLabel::mouseReleaseEvent(ev);
    if (ev->button() != Qt::LeftButton)
        return;
    if (m_pressed && rect().contains(ev->pos()))
        emit clicked();
    m_pressed = false;
}

} // namespace Utils
