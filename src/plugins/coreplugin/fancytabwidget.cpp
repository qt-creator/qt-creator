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

#include "fancytabwidget.h"

#include "coreconstants.h"
#include "fancyactionbar.h"

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QDebug>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmapCache>
#include <QStackedLayout>
#include <QStatusBar>
#include <QStyleFactory>
#include <QStyleOption>
#include <QToolTip>
#include <QVBoxLayout>

using namespace Core;
using namespace Internal;
using namespace Utils;

static const int kMenuButtonWidth = 16;

void FancyTab::fadeIn()
{
    m_animator.stop();
    m_animator.setDuration(80);
    m_animator.setEndValue(1);
    m_animator.start();
}

void FancyTab::fadeOut()
{
    m_animator.stop();
    m_animator.setDuration(160);
    m_animator.setEndValue(0);
    m_animator.start();
}

void FancyTab::setFader(qreal value)
{
    m_fader = value;
    m_tabbar->update();
}

FancyTabBar::FancyTabBar(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(true); // Needed for hover events
}

QSize FancyTabBar::tabSizeHint(bool minimum) const
{
    if (m_iconsOnly) {
        return {Core::Constants::MODEBAR_ICONSONLY_BUTTON_SIZE,
                    Core::Constants::MODEBAR_ICONSONLY_BUTTON_SIZE / (minimum ? 3 : 1)};
    }

    QFont boldFont(font());
    boldFont.setPointSizeF(StyleHelper::sidebarFontSize());
    boldFont.setBold(true);
    const QFontMetrics fm(boldFont);
    const int spacing = 8;
    const int width = 60 + spacing + 2;
    int maxLabelwidth = 0;
    for (auto tab : qAsConst(m_tabs)) {
        const int width = fm.horizontalAdvance(tab->text);
        if (width > maxLabelwidth)
            maxLabelwidth = width;
    }
    const int iconHeight = minimum ? 0 : 32;
    return {qMax(width, maxLabelwidth + 4), iconHeight + spacing + fm.height()};
}

void FancyTabBar::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    if (creatorTheme()->flag(Theme::FlatToolBars)) {
        // draw background of upper part of left tab widget
        // (Welcome, ... Help)
        p.fillRect(event->rect(), StyleHelper::baseColor());
    }

    for (int i = 0; i < count(); ++i)
        if (i != currentIndex())
            paintTab(&p, i);

    // paint active tab last, since it overlaps the neighbors
    if (currentIndex() != -1)
        paintTab(&p, currentIndex());
}

// Handle hover events for mouse fade ins
void FancyTabBar::mouseMoveEvent(QMouseEvent *event)
{
    int newHover = -1;
    for (int i = 0; i < count(); ++i) {
        const QRect area = tabRect(i);
        if (area.contains(event->pos())) {
            newHover = i;
            break;
        }
    }
    if (newHover == m_hoverIndex)
        return;

    if (validIndex(m_hoverIndex))
        m_tabs[m_hoverIndex]->fadeOut();

    m_hoverIndex = newHover;

    if (validIndex(m_hoverIndex)) {
        m_tabs[m_hoverIndex]->fadeIn();
        m_hoverRect = tabRect(m_hoverIndex);
    }
}

bool FancyTabBar::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        if (validIndex(m_hoverIndex)) {
            const QString tt = tabToolTip(m_hoverIndex);
            if (!tt.isEmpty()) {
                QToolTip::showText(static_cast<QHelpEvent *>(event)->globalPos(), tt, this);
                return true;
            }
        }
    }
    return QWidget::event(event);
}

// Resets hover animation on mouse enter
void FancyTabBar::enterEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_hoverRect = QRect();
    m_hoverIndex = -1;
}

// Resets hover animation on mouse enter
void FancyTabBar::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_hoverIndex = -1;
    m_hoverRect = QRect();
    for (auto tab : qAsConst(m_tabs))
        tab->fadeOut();
}

QSize FancyTabBar::sizeHint() const
{
    const QSize sh = tabSizeHint();
    return {sh.width(), sh.height() * int(m_tabs.count())};
}

QSize FancyTabBar::minimumSizeHint() const
{
    const QSize sh = tabSizeHint(true);
    return {sh.width(), sh.height() * int(m_tabs.count())};
}

QRect FancyTabBar::tabRect(int index) const
{
    QSize sh = tabSizeHint();

    if (sh.height() * m_tabs.count() > height())
        sh.setHeight(height() / m_tabs.count());

    return {0, index * sh.height(), sh.width(), sh.height()};
}

void FancyTabBar::mousePressEvent(QMouseEvent *event)
{
    event->accept();
    for (int index = 0; index < m_tabs.count(); ++index) {
        const QRect rect = tabRect(index);
        if (rect.contains(event->pos())) {
            if (isTabEnabled(index)) {
                if (m_tabs.at(index)->hasMenu
                    && ((!m_iconsOnly && rect.right() - event->pos().x() <= kMenuButtonWidth)
                        || event->button() == Qt::RightButton)) {
                    // menu arrow clicked or right-click
                    emit menuTriggered(index, event);
                } else {
                    if (index != m_currentIndex) {
                        emit currentAboutToChange(index);
                        m_currentIndex = index;
                        update();
                        // update tab bar before showing widget
                        QTimer::singleShot(0, this, [this]() { emit currentChanged(m_currentIndex); });
                    }
                }
            }
            break;
        }
    }
}

static void paintSelectedTabBackground(QPainter *painter, const QRect &spanRect)
{
    const int verticalOverlap = 2; // Grows up and down for the overlaps
    const int dpr = painter->device()->devicePixelRatio();
    const QString cacheKey = QLatin1String(Q_FUNC_INFO) + QString::number(spanRect.width())
                             + QLatin1Char('x') + QString::number(spanRect.height())
                             + QLatin1Char('@') + QString::number(dpr);
    QPixmap selection;
    if (!QPixmapCache::find(cacheKey, &selection)) {
        selection = QPixmap(QSize(spanRect.width(), spanRect.height() + 2 * verticalOverlap) * dpr);
        selection.fill(Qt::transparent);
        selection.setDevicePixelRatio(dpr);
        QPainter p(&selection);
        p.translate(QPoint(0, verticalOverlap));

        const QRect rect(QPoint(), spanRect.size());
        const QRectF borderRect = QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5);

        //background
        p.save();
        QLinearGradient grad(rect.topLeft(), rect.topRight());
        grad.setColorAt(0, QColor(255, 255, 255, 140));
        grad.setColorAt(1, QColor(255, 255, 255, 210));
        p.fillRect(rect, grad);
        p.restore();

        //shadows
        p.setPen(QColor(0, 0, 0, 110));
        p.drawLine(borderRect.topLeft() + QPointF(1, -1), borderRect.topRight() - QPointF(0, 1));
        p.drawLine(borderRect.bottomLeft(), borderRect.bottomRight());
        p.setPen(QColor(0, 0, 0, 40));
        p.drawLine(borderRect.topLeft(), borderRect.bottomLeft());

        //highlights
        p.setPen(QColor(255, 255, 255, 50));
        p.drawLine(borderRect.topLeft() + QPointF(0, -2), borderRect.topRight() - QPointF(0, 2));
        p.drawLine(borderRect.bottomLeft() + QPointF(0, 1),
                   borderRect.bottomRight() + QPointF(0, 1));
        p.setPen(QColor(255, 255, 255, 40));
        p.drawLine(borderRect.topLeft() + QPointF(0, 0), borderRect.topRight());
        p.drawLine(borderRect.topRight() + QPointF(0, 1), borderRect.bottomRight() - QPointF(0, 1));
        p.drawLine(borderRect.bottomLeft() + QPointF(0, -1),
                   borderRect.bottomRight() - QPointF(0, 1));
        p.end();

        QPixmapCache::insert(cacheKey, selection);
    }
    painter->drawPixmap(spanRect.topLeft() + QPoint(0, -verticalOverlap), selection);
}

static void paintIcon(QPainter *painter, const QRect &rect,
                      const QIcon &icon,
                      bool enabled, bool selected)
{
    const QIcon::Mode iconMode = enabled ? (selected ? QIcon::Active : QIcon::Normal)
                                         : QIcon::Disabled;
    QRect iconRect(0, 0, Core::Constants::MODEBAR_ICON_SIZE, Core::Constants::MODEBAR_ICON_SIZE);
    iconRect.moveCenter(rect.center());
    iconRect = iconRect.intersected(rect);
    if (!enabled && !creatorTheme()->flag(Theme::FlatToolBars))
        painter->setOpacity(0.7);
    StyleHelper::drawIconWithShadow(icon, iconRect, painter, iconMode);

    if (selected && creatorTheme()->flag(Theme::FlatToolBars)) {
        painter->setOpacity(1.0);
        QRect accentRect = rect;
        accentRect.setWidth(2);
        painter->fillRect(accentRect, creatorTheme()->color(Theme::IconsBaseColor));
    }
}

static void paintIconAndText(QPainter *painter, const QRect &rect,
                             const QIcon &icon, const QString &text,
                             bool enabled, bool selected)
{
    QFont boldFont(painter->font());
    boldFont.setPointSizeF(StyleHelper::sidebarFontSize());
    boldFont.setBold(true);
    painter->setFont(boldFont);

    const bool drawIcon = rect.height() > 36;
    if (drawIcon) {
        const int textHeight =
                painter->fontMetrics().boundingRect(rect, Qt::TextWordWrap, text).height();
        const QRect tabIconRect(rect.adjusted(0, 4, 0, -textHeight));
        const QIcon::Mode iconMode = enabled ? (selected ? QIcon::Active : QIcon::Normal)
                                             : QIcon::Disabled;
        QRect iconRect(0, 0, Core::Constants::MODEBAR_ICON_SIZE, Core::Constants::MODEBAR_ICON_SIZE);
        iconRect.moveCenter(tabIconRect.center());
        iconRect = iconRect.intersected(tabIconRect);
        if (!enabled && !creatorTheme()->flag(Theme::FlatToolBars))
            painter->setOpacity(0.7);
        StyleHelper::drawIconWithShadow(icon, iconRect, painter, iconMode);
    }

    painter->setOpacity(1.0); //FIXME: was 0.7 before?
    if (selected && creatorTheme()->flag(Theme::FlatToolBars)) {
        QRect accentRect = rect;
        accentRect.setWidth(2);
        painter->fillRect(accentRect, creatorTheme()->color(Theme::IconsBaseColor));
    }
    if (enabled) {
        painter->setPen(
            selected ? creatorTheme()->color(Theme::FancyTabWidgetEnabledSelectedTextColor)
                     : creatorTheme()->color(Theme::FancyTabWidgetEnabledUnselectedTextColor));
    } else {
        painter->setPen(
            selected ? creatorTheme()->color(Theme::FancyTabWidgetDisabledSelectedTextColor)
                     : creatorTheme()->color(Theme::FancyTabWidgetDisabledUnselectedTextColor));
    }

    painter->translate(0, -1);
    QRect tabTextRect(rect);
    tabTextRect.translate(0, drawIcon ? -2 : 1);
    const int textFlags = Qt::AlignCenter | (drawIcon ? Qt::AlignBottom : Qt::AlignVCenter)
                          | Qt::TextWordWrap;
    painter->drawText(tabTextRect, textFlags, text);
}

void FancyTabBar::paintTab(QPainter *painter, int tabIndex) const
{
    if (!validIndex(tabIndex)) {
        qWarning("invalid index");
        return;
    }
    painter->save();

    const FancyTab *tab = m_tabs.at(tabIndex);
    const QRect rect = tabRect(tabIndex);
    const bool selected = (tabIndex == m_currentIndex);
    const bool enabled = isTabEnabled(tabIndex);

    if (selected) {
        if (creatorTheme()->flag(Theme::FlatToolBars)) {
            // background color of a fancy tab that is active
            painter->fillRect(rect, creatorTheme()->color(Theme::FancyTabBarSelectedBackgroundColor));
        } else {
            paintSelectedTabBackground(painter, rect);
        }
    }

    const qreal fader = tab->fader();
    if (fader > 0 && !HostOsInfo::isMacHost() && !selected && enabled) {
        painter->save();
        painter->setOpacity(fader);
        if (creatorTheme()->flag(Theme::FlatToolBars))
            painter->fillRect(rect, creatorTheme()->color(Theme::FancyToolButtonHoverColor));
        else
            FancyToolButton::hoverOverlay(painter, rect);
        painter->restore();
    }

    if (m_iconsOnly)
        paintIcon(painter, rect, tab->icon, enabled, selected);
    else
        paintIconAndText(painter, rect, tab->icon, tab->text, enabled, selected);

    // menu arrow
    if (tab->hasMenu && !m_iconsOnly) {
        QStyleOption opt;
        opt.initFrom(this);
        opt.rect = rect.adjusted(rect.width() - kMenuButtonWidth, 0, -8, 0);
        StyleHelper::drawArrow(QStyle::PE_IndicatorArrowRight, painter, &opt);
    }
    painter->restore();
}

void FancyTabBar::setCurrentIndex(int index)
{
    if (isTabEnabled(index) && index != m_currentIndex) {
        emit currentAboutToChange(index);
        m_currentIndex = index;
        update();
        emit currentChanged(m_currentIndex);
    }
}

void FancyTabBar::setIconsOnly(bool iconsOnly)
{
    m_iconsOnly = iconsOnly;
    updateGeometry();
}

void FancyTabBar::setTabEnabled(int index, bool enable)
{
    Q_ASSERT(index < m_tabs.size());
    Q_ASSERT(index >= 0);

    if (index < m_tabs.size() && index >= 0) {
        m_tabs[index]->enabled = enable;
        update(tabRect(index));
    }
}

bool FancyTabBar::isTabEnabled(int index) const
{
    Q_ASSERT(index < m_tabs.size());
    Q_ASSERT(index >= 0);

    if (index < m_tabs.size() && index >= 0)
        return m_tabs[index]->enabled;

    return false;
}

class FancyColorButton : public QWidget
{
    Q_OBJECT

public:
    explicit FancyColorButton(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    }

    void mousePressEvent(QMouseEvent *ev) override { emit clicked(ev->button(), ev->modifiers()); }

    void paintEvent(QPaintEvent *event) override
    {
        QWidget::paintEvent(event);

        // Some Themes do not want highlights, shadows and borders in the toolbars.
        // But we definitely want a separator between FancyColorButton and FancyTabBar
        if (!creatorTheme()->flag(Theme::DrawToolBarHighlights)
            && !creatorTheme()->flag(Theme::DrawToolBarBorders)) {
            QPainter p(this);
            p.setPen(StyleHelper::toolBarBorderColor());
            const QRectF innerRect = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
            p.drawLine(innerRect.bottomLeft(), innerRect.bottomRight());
        }
    }

signals:
    void clicked(Qt::MouseButton button, Qt::KeyboardModifiers modifiers);
};

//////
// FancyTabWidget
//////

FancyTabWidget::FancyTabWidget(QWidget *parent)
    : QWidget(parent)
{
    m_tabBar = new FancyTabBar(this);
    m_tabBar->setObjectName("ModeSelector"); // used for UI introduction

    m_selectionWidget = new QWidget(this);
    auto selectionLayout = new QVBoxLayout;
    selectionLayout->setSpacing(0);
    selectionLayout->setContentsMargins(0, 0, 0, 0);

    auto bar = new StyledBar;
    auto layout = new QHBoxLayout(bar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    auto fancyButton = new FancyColorButton(this);
    connect(fancyButton, &FancyColorButton::clicked, this, &FancyTabWidget::topAreaClicked);
    layout->addWidget(fancyButton);
    selectionLayout->addWidget(bar);

    selectionLayout->addWidget(m_tabBar);
    selectionLayout->addStretch(1);
    m_selectionWidget->setLayout(selectionLayout);
    m_selectionWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    m_cornerWidgetContainer = new QWidget(this);
    m_cornerWidgetContainer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    m_cornerWidgetContainer->setAutoFillBackground(false);

    auto cornerWidgetLayout = new QVBoxLayout;
    cornerWidgetLayout->setSpacing(0);
    cornerWidgetLayout->setContentsMargins(0, 0, 0, 0);
    cornerWidgetLayout->addStretch();
    m_cornerWidgetContainer->setLayout(cornerWidgetLayout);

    selectionLayout->addWidget(m_cornerWidgetContainer, 0);

    m_modesStack = new QStackedLayout;
    m_statusBar = new QStatusBar;
    m_statusBar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    auto vlayout = new QVBoxLayout;
    vlayout->setContentsMargins(0, 0, 0, 0);
    vlayout->setSpacing(0);
    vlayout->addLayout(m_modesStack);
    vlayout->addWidget(m_statusBar);

    m_infoBarDisplay.setTarget(vlayout, 1);
    m_infoBarDisplay.setStyle(QFrame::Sunken);

    auto mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(1);
    mainLayout->addWidget(m_selectionWidget);
    mainLayout->addLayout(vlayout);
    setLayout(mainLayout);

    connect(m_tabBar, &FancyTabBar::currentAboutToChange, this, &FancyTabWidget::currentAboutToShow);
    connect(m_tabBar, &FancyTabBar::currentChanged, this, &FancyTabWidget::showWidget);
    connect(m_tabBar, &FancyTabBar::menuTriggered, this, &FancyTabWidget::menuTriggered);
}

void FancyTabWidget::setSelectionWidgetVisible(bool visible)
{
    m_selectionWidget->setVisible(visible);
}

bool FancyTabWidget::isSelectionWidgetVisible() const
{
    return m_selectionWidget->isVisible();
}

void FancyTabWidget::insertTab(int index, QWidget *tab, const QIcon &icon, const QString &label, bool hasMenu)
{
    m_modesStack->insertWidget(index, tab);
    m_tabBar->insertTab(index, icon, label, hasMenu);
}

void FancyTabWidget::removeTab(int index)
{
    m_modesStack->removeWidget(m_modesStack->widget(index));
    m_tabBar->removeTab(index);
}

void FancyTabWidget::setBackgroundBrush(const QBrush &brush)
{
    QPalette pal;
    pal.setBrush(QPalette::Mid, brush);
    m_tabBar->setPalette(pal);
    m_cornerWidgetContainer->setPalette(pal);
}

void FancyTabWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    if (m_selectionWidget->isVisible()) {
        QPainter painter(this);

        QRect rect = m_selectionWidget->rect().adjusted(0, 0, 1, 0);
        rect = style()->visualRect(layoutDirection(), geometry(), rect);
        const QRectF boderRect = QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5);

        if (creatorTheme()->flag(Theme::FlatToolBars)) {
            painter.fillRect(rect, StyleHelper::baseColor());
            painter.setPen(StyleHelper::toolBarBorderColor());
            painter.drawLine(boderRect.topRight(), boderRect.bottomRight());
        } else {
            StyleHelper::verticalGradient(&painter, rect, rect);
            painter.setPen(StyleHelper::borderColor());
            painter.drawLine(boderRect.topRight(), boderRect.bottomRight());

            const QColor light = StyleHelper::sidebarHighlight();
            painter.setPen(light);
            painter.drawLine(boderRect.bottomLeft(), boderRect.bottomRight());
        }
    }
}

void FancyTabWidget::insertCornerWidget(int pos, QWidget *widget)
{
    auto layout = static_cast<QVBoxLayout *>(m_cornerWidgetContainer->layout());
    layout->insertWidget(pos, widget);
}

int FancyTabWidget::cornerWidgetCount() const
{
    return m_cornerWidgetContainer->layout()->count();
}

void FancyTabWidget::addCornerWidget(QWidget *widget)
{
    m_cornerWidgetContainer->layout()->addWidget(widget);
}

int FancyTabWidget::currentIndex() const
{
    return m_tabBar->currentIndex();
}

QStatusBar *FancyTabWidget::statusBar() const
{
    return m_statusBar;
}

InfoBar *FancyTabWidget::infoBar()
{
    if (!m_infoBarDisplay.infoBar())
        m_infoBarDisplay.setInfoBar(&m_infoBar);
    return &m_infoBar;
}

void FancyTabWidget::setCurrentIndex(int index)
{
    m_tabBar->setCurrentIndex(index);
}

void FancyTabWidget::showWidget(int index)
{
    m_modesStack->setCurrentIndex(index);
    QWidget *w = m_modesStack->currentWidget();
    if (QTC_GUARD(w)) {
        if (QWidget *focusWidget = w->focusWidget())
            w = focusWidget;
        w->setFocus();
    }
    emit currentChanged(index);
}

void FancyTabWidget::setTabToolTip(int index, const QString &toolTip)
{
    m_tabBar->setTabToolTip(index, toolTip);
}

void FancyTabWidget::setTabEnabled(int index, bool enable)
{
    m_tabBar->setTabEnabled(index, enable);
}

bool FancyTabWidget::isTabEnabled(int index) const
{
    return m_tabBar->isTabEnabled(index);
}

void FancyTabWidget::setIconsOnly(bool iconsOnly)
{
    m_tabBar->setIconsOnly(iconsOnly);
}

#include "fancytabwidget.moc"
