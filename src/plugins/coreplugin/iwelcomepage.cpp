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

#include "iwelcomepage.h"

#include "icore.h"

#include <utils/icon.h>
#include <utils/theme/theme.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QMetaEnum>
#include <QPainter>
#include <QPixmap>
#include <QUrl>

using namespace Utils;

namespace Core {

IWelcomePage::IWelcomePage()
{
}

IWelcomePage::~IWelcomePage()
{
}

static QPalette buttonPalette(bool isActive, bool isCursorInside, bool forText)
{
    QPalette pal;
    Theme *theme = Utils::creatorTheme();
    if (isActive) {
        if (forText) {
            pal.setColor(QPalette::Background, theme->color(Theme::Welcome_ForegroundPrimaryColor));
            pal.setColor(QPalette::Foreground, theme->color(Theme::Welcome_ForegroundPrimaryColor));
            pal.setColor(QPalette::WindowText, theme->color(Theme::Welcome_BackgroundColor));
        } else {
            pal.setColor(QPalette::Background, theme->color(Theme::Welcome_ForegroundPrimaryColor));
            pal.setColor(QPalette::Foreground, theme->color(Theme::Welcome_ForegroundPrimaryColor));
        }
    } else {
        if (isCursorInside) {
            if (forText) {
                pal.setColor(QPalette::Background, theme->color(Theme::Welcome_HoverColor));
                pal.setColor(QPalette::Foreground, theme->color(Theme::Welcome_HoverColor));
                pal.setColor(QPalette::WindowText, theme->color(Theme::Welcome_TextColor));
            } else {
                pal.setColor(QPalette::Background, theme->color(Theme::Welcome_HoverColor));
                pal.setColor(QPalette::Foreground, theme->color(Theme::Welcome_ForegroundSecondaryColor));
            }
        } else {
            if (forText) {
                pal.setColor(QPalette::Background, theme->color(Theme::Welcome_ForegroundPrimaryColor));
                pal.setColor(QPalette::Foreground, theme->color(Theme::Welcome_BackgroundColor));
                pal.setColor(QPalette::WindowText, theme->color(Theme::Welcome_TextColor));
            } else {
                pal.setColor(QPalette::Background, theme->color(Theme::Welcome_BackgroundColor));
                pal.setColor(QPalette::Foreground, theme->color(Theme::Welcome_ForegroundSecondaryColor));
            }
        }
    }
    return pal;
}

WelcomePageFrame::WelcomePageFrame(QWidget *parent)
    : QWidget(parent)
{
    setContentsMargins(1, 1, 1, 1);
}

void WelcomePageFrame::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    const QRectF adjustedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5));
    QPen pen(palette().color(QPalette::WindowText));
    pen.setJoinStyle(Qt::MiterJoin);

    QPainter p(this);
    p.setPen(pen);
    p.drawRect(adjustedRect);
}

class WelcomePageButtonPrivate
{
public:
    WelcomePageButtonPrivate(WelcomePageButton *parent) : q(parent) {}
    bool isActive() const;
    void doUpdate(bool cursorInside);

    WelcomePageButton *q;
    QHBoxLayout *m_layout = nullptr;
    QLabel *m_label = nullptr;
    QLabel *m_icon = nullptr;

    std::function<void()> onClicked;
    std::function<bool()> activeChecker;
};

WelcomePageButton::WelcomePageButton(QWidget *parent)
    : WelcomePageFrame(parent), d(new WelcomePageButtonPrivate(this))
{
    setAutoFillBackground(true);
    setPalette(buttonPalette(false, false, false));

    QFont f = font();
    f.setPixelSize(15);
    d->m_label = new QLabel(this);
    d->m_label->setFont(f);
    d->m_label->setPalette(buttonPalette(false, false, true));

    d->m_layout = new QHBoxLayout;
    d->m_layout->setContentsMargins(13, 5, 20, 5);
    d->m_layout->setSpacing(0);
    d->m_layout->addWidget(d->m_label);
    setLayout(d->m_layout);
}

WelcomePageButton::~WelcomePageButton()
{
    delete d;
}

void WelcomePageButton::mousePressEvent(QMouseEvent *)
{
    if (d->onClicked)
        d->onClicked();
}

void WelcomePageButton::enterEvent(QEvent *)
{
    d->doUpdate(true);
}

void WelcomePageButton::leaveEvent(QEvent *)
{
    d->doUpdate(false);
}

bool WelcomePageButtonPrivate::isActive() const
{
    return activeChecker && activeChecker();
}

void WelcomePageButtonPrivate::doUpdate(bool cursorInside)
{
    const bool active = isActive();
    q->setPalette(buttonPalette(active, cursorInside, false));
    const QPalette lpal = buttonPalette(active, cursorInside, true);
    m_label->setPalette(lpal);
    if (m_icon)
        m_icon->setPalette(lpal);
    q->update();
}

void WelcomePageButton::setText(const QString &text)
{
    d->m_label->setText(text);
}

void WelcomePageButton::setIcon(const QPixmap &pixmap)
{
    if (!d->m_icon) {
        d->m_icon = new QLabel(this);
        d->m_layout->insertWidget(0, d->m_icon);
        d->m_layout->insertSpacing(1, 10);
    }
    d->m_icon->setPixmap(pixmap);
}

void WelcomePageButton::setActiveChecker(const std::function<bool ()> &value)
{
    d->activeChecker = value;
}

void WelcomePageButton::recheckActive()
{
    bool isActive = d->isActive();
    d->doUpdate(isActive);
}

void WelcomePageButton::click()
{
    if (d->onClicked)
        d->onClicked();
}

void WelcomePageButton::setOnClicked(const std::function<void ()> &value)
{
    d->onClicked = value;
    if (d->isActive())
        click();
}

} // namespace Core
