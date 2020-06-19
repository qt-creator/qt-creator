/****************************************************************************
**
** Copyright (C) 2020 Uwe Kindler
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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or (at your option) any later version.
** The licenses are as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPLv21 included in the packaging
** of this file. Please review the following information to ensure
** the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "elidinglabel.h"

#include <QMouseEvent>

namespace ADS {
    /**
     * Private data of public ClickableLabel
     */
    struct ElidingLabelPrivate
    {
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
        if (str == QChar(0x2026))
            str = m_text.at(0);

        bool wasElided = m_isElided;
        m_isElided = str != m_text;
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

    bool ElidingLabel::hasPixmap() const
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        return !pixmap().isNull();
#else
        return pixmap() != nullptr;
#endif
    }

    QSize ElidingLabel::minimumSizeHint() const
    {
        if (hasPixmap() || d->isModeElideNone())
            return QLabel::minimumSizeHint();

        const QFontMetrics &fm = fontMetrics();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
        QSize size(fm.horizontalAdvance(d->m_text.left(2) + "…"), fm.height());
#else
        QSize size(fm.width(d->m_text.left(2) + "…"), fm.height());
#endif
        return size;
    }

    QSize ElidingLabel::sizeHint() const
    {
        if (hasPixmap() || d->isModeElideNone())
            return QLabel::sizeHint();

        const QFontMetrics &fm = fontMetrics();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
        QSize size(fm.horizontalAdvance(d->m_text), QLabel::sizeHint().height());
#else
        QSize size(fm.width(d->m_text), QLabel::sizeHint().height());
#endif
        return size;
    }

    void ElidingLabel::setText(const QString &text)
    {
        d->m_text = text;
        if (d->isModeElideNone()) {
            Super::setText(text);
        } else {
            internal::setToolTip(this, text);
            d->elideText(this->size().width());
        }
    }

    QString ElidingLabel::text() const
    {
        return d->m_text;
    }

} // namespace ADS
