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

#include "fadingindicator.h"

#include "stylehelper.h"

#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QPropertyAnimation>
#include <QTimer>

namespace Utils {
namespace Internal {

class FadingIndicatorPrivate : public QWidget
{
    Q_OBJECT

public:
    FadingIndicatorPrivate(QWidget *parent, FadingIndicator::TextSize size)
        : QWidget(parent)
    {
        m_effect = new QGraphicsOpacityEffect(this);
        setGraphicsEffect(m_effect);
        m_effect->setOpacity(1.);

        m_label = new QLabel;
        QFont font = m_label->font();
        font.setPixelSize(size == FadingIndicator::LargeText ? 45 : 22);
        m_label->setFont(font);
        QPalette pal = palette();
        pal.setColor(QPalette::Foreground, pal.color(QPalette::Background));
        m_label->setPalette(pal);
        auto layout = new QHBoxLayout;
        setLayout(layout);
        layout->addWidget(m_label);
    }

    void setText(const QString &text)
    {
        m_pixmap = QPixmap();
        m_label->setText(text);
        layout()->setSizeConstraint(QLayout::SetFixedSize);
        adjustSize();
        if (QWidget *parent = parentWidget())
            move(parent->rect().center() - rect().center());
    }

    void setPixmap(const QString &uri)
    {
        m_label->hide();
        m_pixmap.load(Utils::StyleHelper::dpiSpecificImageFile(uri));
        layout()->setSizeConstraint(QLayout::SetNoConstraint);
        resize(m_pixmap.size() / m_pixmap.devicePixelRatio());
        if (QWidget *parent = parentWidget())
            move(parent->rect().center() - rect().center());
    }

    void run(int ms)
    {
        show();
        raise();
        QTimer::singleShot(ms, this, &FadingIndicatorPrivate::runInternal);
    }

protected:
    void paintEvent(QPaintEvent *)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        if (!m_pixmap.isNull()) {
            p.drawPixmap(rect(), m_pixmap);
        } else {
            p.setBrush(palette().color(QPalette::Foreground));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(rect(), 15, 15);
        }
    }

private:
    void runInternal()
    {
        QPropertyAnimation *anim = new QPropertyAnimation(m_effect, "opacity", this);
        anim->setDuration(200);
        anim->setEndValue(0.);
        connect(anim, &QAbstractAnimation::finished, this, &QObject::deleteLater);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    QGraphicsOpacityEffect *m_effect;
    QLabel *m_label;
    QPixmap m_pixmap;
};

} // Internal

namespace FadingIndicator {

void showText(QWidget *parent, const QString &text, TextSize size)
{
    static QPointer<Internal::FadingIndicatorPrivate> indicator;
    if (indicator)
        delete indicator;
    indicator = new Internal::FadingIndicatorPrivate(parent, size);
    indicator->setText(text);
    indicator->run(2500); // deletes itself
}

void showPixmap(QWidget *parent, const QString &pixmap)
{
    static QPointer<Internal::FadingIndicatorPrivate> indicator;
    if (indicator)
        delete indicator;
    indicator = new Internal::FadingIndicatorPrivate(parent, LargeText);
    indicator->setPixmap(pixmap);
    indicator->run(300); // deletes itself
}

} // FadingIndicator
} // Utils

#include "fadingindicator.moc"
