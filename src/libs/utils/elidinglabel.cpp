/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "elidinglabel.h"
#include <QFontMetrics>
#include <QPainter>
#include <QStyle>

/*!
    \class Utils::ElidingLabel

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
    QRect contents = contentsRect().adjusted(m, m, -m, -m);
    QFontMetrics fm = fontMetrics();
    QString txt = text();
    if (txt.length() > 4 && fm.horizontalAdvance(txt) > contents.width()) {
        updateToolTip(txt);
        txt = fm.elidedText(txt, m_elideMode, contents.width());
    } else {
        updateToolTip(QString());
    }
    int flags = QStyle::visualAlignment(layoutDirection(), alignment()) | Qt::TextSingleLine;

    QPainter painter(this);
    drawFrame(&painter);
    painter.drawText(contents, flags, txt);
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

} // namespace Utils
