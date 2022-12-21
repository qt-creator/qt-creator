// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iwelcomepage.h"

#include "icore.h"
#include "welcomepagehelper.h"

#include <utils/icon.h>
#include <utils/theme/theme.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QUrl>

#include <qdrawutil.h>

using namespace Utils;

namespace Core {

const char WITHACCENTCOLOR_PROPERTY_NAME[] = "_withAccentColor";

static QList<IWelcomePage *> g_welcomePages;

const QList<IWelcomePage *> IWelcomePage::allWelcomePages()
{
    return g_welcomePages;
}

IWelcomePage::IWelcomePage()
{
    g_welcomePages.append(this);
}

IWelcomePage::~IWelcomePage()
{
    g_welcomePages.removeOne(this);
}

QPalette WelcomePageFrame::buttonPalette(bool isActive, bool isCursorInside, bool forText)
{
    QPalette pal;
    pal.setBrush(QPalette::Window, {});
    pal.setBrush(QPalette::WindowText, {});

    Theme *theme = Utils::creatorTheme();
    if (isActive) {
        if (forText) {
            pal.setColor(QPalette::Window, theme->color(Theme::Welcome_ForegroundPrimaryColor));
            pal.setColor(QPalette::WindowText, theme->color(Theme::Welcome_BackgroundPrimaryColor));
        } else {
            pal.setColor(QPalette::Window, theme->color(Theme::Welcome_AccentColor));
            pal.setColor(QPalette::WindowText, theme->color(Theme::Welcome_AccentColor));
        }
    } else {
        if (isCursorInside) {
            if (forText) {
                pal.setColor(QPalette::Window, theme->color(Theme::Welcome_HoverColor));
                pal.setColor(QPalette::WindowText, theme->color(Theme::Welcome_TextColor));
            } else {
                pal.setColor(QPalette::Window, theme->color(Theme::Welcome_HoverColor));
                pal.setColor(QPalette::WindowText, theme->color(Theme::Welcome_ForegroundSecondaryColor));
            }
        } else {
            if (forText) {
                pal.setColor(QPalette::Window, theme->color(Theme::Welcome_ForegroundPrimaryColor));
                pal.setColor(QPalette::WindowText, theme->color(Theme::Welcome_TextColor));
            } else {
                pal.setColor(QPalette::Window, theme->color(Theme::Welcome_BackgroundPrimaryColor));
                pal.setColor(QPalette::WindowText, theme->color(Theme::Welcome_ForegroundSecondaryColor));
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
    QPainter p(this);

    qDrawPlainRect(&p, rect(), palette().color(QPalette::WindowText), 1);

    if (property(WITHACCENTCOLOR_PROPERTY_NAME).toBool()) {
        const int accentRectWidth = 10;
        const QRect accentRect = rect().adjusted(width() - accentRectWidth, 0, 0, 0);
        p.fillRect(accentRect, creatorTheme()->color(Theme::Welcome_AccentColor));
    }
}

class WelcomePageButtonPrivate
{
public:
    explicit WelcomePageButtonPrivate(WelcomePageButton *parent)
        : q(parent) {}
    bool isActive() const;
    void doUpdate(bool cursorInside);

    WelcomePageButton *q;
    QHBoxLayout *m_layout = nullptr;
    QLabel *m_label = nullptr;

    std::function<void()> onClicked;
    std::function<bool()> activeChecker;
};

WelcomePageButton::WelcomePageButton(QWidget *parent)
    : WelcomePageFrame(parent), d(new WelcomePageButtonPrivate(this))
{
    setAutoFillBackground(true);
    setPalette(buttonPalette(false, false, false));
    setContentsMargins(0, 1, 0, 1);

    d->m_label = new QLabel(this);
    d->m_label->setPalette(buttonPalette(false, false, true));
    d->m_label->setAlignment(Qt::AlignCenter);

    d->m_layout = new QHBoxLayout;
    d->m_layout->setSpacing(0);
    d->m_layout->addWidget(d->m_label);
    setSize(SizeLarge);
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

void WelcomePageButton::enterEvent(QEnterEvent *)
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
    q->setPalette(WelcomePageFrame::buttonPalette(active, cursorInside, false));
    const QPalette lpal = WelcomePageFrame::buttonPalette(active, cursorInside, true);
    m_label->setPalette(lpal);
    q->update();
}

void WelcomePageButton::setText(const QString &text)
{
    d->m_label->setText(text);
}

void WelcomePageButton::setSize(Size size)
{
    const int hMargin = size == SizeSmall ? 12 : 26;
    const int vMargin = size == SizeSmall ? 2 : 4;
    d->m_layout->setContentsMargins(hMargin, vMargin, hMargin, vMargin);
    d->m_label->setFont(size == SizeSmall ? font() : WelcomePageHelpers::brandFont());
}

void WelcomePageButton::setWithAccentColor(bool withAccent)
{
    setProperty(WITHACCENTCOLOR_PROPERTY_NAME, withAccent);
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
