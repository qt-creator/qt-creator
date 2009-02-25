/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "fancytabwidget.h"
#include "stylehelper.h"

#include <QDebug>

#include <QtGui/QColorDialog>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMouseEvent>
#include <QtGui/QWindowsStyle>
#include <QtGui/QPainter>
#include <QtGui/QSplitter>
#include <QtGui/QStackedLayout>
#include <QtGui/QStatusBar>
#include <QtGui/QToolBar>
#include <QtGui/QToolButton>

using namespace Core;
using namespace Internal;

const int FancyTabBar::m_rounding = 22;
const int FancyTabBar::m_textPadding = 4;

FancyTabBar::FancyTabBar(QWidget *parent)
    : QTabBar(parent)
{
    setStyle(new QWindowsStyle);
    setDrawBase(false);
    setElideMode(Qt::ElideNone);
    setMinimumWidth(qMax(2 * m_rounding, 40));
    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::NoFocus);
    m_hoverControl.setFrameRange(0, 20);
    m_hoverControl.setDuration(130);
    m_hoverControl.setCurveShape(QTimeLine::EaseInCurve);
    connect(&m_hoverControl, SIGNAL(frameChanged(int)), this, SLOT(updateHover()));
    setMouseTracking(true); // Needed for hover events
    setExpanding(false);
}

FancyTabBar::~FancyTabBar()
{
    delete style();
}

QSize FancyTabBar::tabSizeHint(int index) const
{
    QFont boldFont(font());
    boldFont.setPointSizeF(StyleHelper::sidebarFontSize());
    boldFont.setBold(true);
    QFontMetrics fm(boldFont);
    int spacing = 6;
    int width = 60 + spacing + 2;
    return QSize(width, tabIcon(index).actualSize(QSize(64, 64)).height() + spacing + fm.height());
}

void FancyTabBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);

    for (int i = 0; i < count(); ++i)
        if (i != currentIndex())
            paintTab(&p, i);

    // paint active tab last, since it overlaps the neighbors
    paintTab(&p, currentIndex());
}

// Handle hover events for mouse fade ins
void FancyTabBar::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_hoverRect.contains(e->pos())) {
        for (int i = 0; i < count(); ++i) {
            QRect area = tabRect(i);
            if (area.contains(e->pos())) {
                QRect oldHoverRect = m_hoverRect;
                m_hoverRect = area;
                update(oldHoverRect);
                m_hoverControl.stop();
                m_hoverControl.start();
                break;
            }
        }
    }
}

void FancyTabBar::updateHover()
{
    update(m_hoverRect);
}

// Resets hover animation on mouse enter
void FancyTabBar::enterEvent(QEvent *e)
{
    Q_UNUSED(e);
    m_hoverRect = QRect();
}

// Resets hover animation on mouse enter
void FancyTabBar::leaveEvent(QEvent *e)
{
    Q_UNUSED(e);

    m_hoverControl.stop();
    m_hoverControl.start();
}

void FancyTabBar::paintTab(QPainter *painter, int tabIndex) const
{
    QStyleOptionTabV2 tab;
    initStyleOption(&tab, tabIndex);
    QRect rect = tab.rect;
    painter->save();


    bool selected = tab.state & QStyle::State_Selected;
    bool hover = tab.state & QStyle::State_MouseOver;

#ifdef Q_WS_MAC
    hover = false; // Dont hover on Mac
#endif

    QColor background = QColor(0, 0, 0, 10);
    QColor hoverColor;

    if (hover) {
        hoverColor = QColor(255, 255, 255, m_hoverControl.currentFrame());
    }

    QColor light = QColor(255, 255, 255, 40);
    QColor dark = QColor(0, 0, 0, 60);

    if (selected) {
        QLinearGradient selectedGradient(rect.topLeft(), QPoint(rect.center().x(), rect.bottom()));
        selectedGradient.setColorAt(0, Qt::white);
        selectedGradient.setColorAt(0.3, Qt::white);
        selectedGradient.setColorAt(0.7, QColor(230, 230, 230));

        painter->fillRect(rect, selectedGradient);
        painter->setPen(QColor(200, 200, 200));
        painter->drawLine(rect.topLeft(), rect.topRight());
        painter->setPen(QColor(150, 160, 200));
        painter->drawLine(rect.bottomLeft(), rect.bottomRight());
    } else {
        painter->fillRect(tab.rect, background);
        if (hover)
            painter->fillRect(tab.rect, hoverColor);
        painter->setPen(QPen(light, 0));
        painter->drawLine(rect.topLeft(), rect.topRight());
        painter->setPen(QPen(dark, 0));
        painter->drawLine(rect.bottomLeft(), rect.bottomRight());
    }

    QString tabText(tab.text);
    QRect tabTextRect(tab.rect);
    QRect tabIconRect(tab.rect);
    QFont boldFont(painter->font());
    boldFont.setPointSizeF(StyleHelper::sidebarFontSize());
    boldFont.setBold(true);
    painter->setFont(boldFont);
    painter->setPen(selected ? StyleHelper::panelTextColor() : QColor(30, 30, 30, 80));
    int textFlags = Qt::AlignCenter | Qt::AlignBottom | Qt::ElideRight | Qt::TextWordWrap;
    painter->drawText(tabTextRect, textFlags, tabText);
    painter->setPen(selected ? QColor(60, 60, 60) : StyleHelper::panelTextColor());
    int textHeight = painter->fontMetrics().boundingRect(QRect(0, 0, width(), height()), Qt::TextWordWrap, tabText).height();
    tabIconRect.adjust(0, 4, 0, -textHeight);
    style()->drawItemPixmap(painter, tabIconRect, Qt::AlignCenter | Qt::AlignVCenter,
                            tab.icon.pixmap(QSize(64, 64)));
    painter->translate(0, -1);
    painter->drawText(tabTextRect, textFlags, tabText);
    painter->restore();
}

void FancyTabBar::tabInserted(int index)
{
    Q_UNUSED(index)
}

void FancyTabBar::tabRemoved(int index)
{
    Q_UNUSED(index)
}

//////
// FancyColorButton
//////

class FancyColorButton : public QWidget
{
public:
    FancyColorButton(QWidget *parent)
      : m_parent(parent)
    {
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    }

    void mousePressEvent(QMouseEvent *ev)
    {
        if (ev->modifiers() & Qt::ShiftModifier) 
            StyleHelper::setBaseColor(QColorDialog::getColor(StyleHelper::baseColor(), m_parent));
    }
private:
    QWidget *m_parent;
};

//////
// FancyTabWidget
//////

FancyTabWidget::FancyTabWidget(QWidget *parent)
    : QWidget(parent)
{
    m_tabBar = new FancyTabBar(this);
    m_tabBar->setShape(QTabBar::RoundedEast);
    m_tabBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    m_tabBar->setUsesScrollButtons(false);

    m_selectionWidget = new QWidget(this);
    QVBoxLayout *selectionLayout = new QVBoxLayout;
    selectionLayout->setSpacing(0);
    selectionLayout->setMargin(0);

    QToolBar *bar = new QToolBar;
    bar->addWidget(new FancyColorButton(this));
    bar->setFixedHeight(StyleHelper::navigationWidgetHeight());
    selectionLayout->addWidget(bar);

    selectionLayout->addWidget(m_tabBar, 1);
    m_selectionWidget->setLayout(selectionLayout);
    m_selectionWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);

    m_cornerWidgetContainer = new QWidget(this);
    m_cornerWidgetContainer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
    m_cornerWidgetContainer->setAutoFillBackground(false);

    QVBoxLayout *cornerWidgetLayout = new QVBoxLayout;
    cornerWidgetLayout->setSpacing(0);
    cornerWidgetLayout->setMargin(0);
    cornerWidgetLayout->addStretch();
    m_cornerWidgetContainer->setLayout(cornerWidgetLayout);

    selectionLayout->addWidget(m_cornerWidgetContainer, 0);

    m_modesStack = new QStackedLayout;
    m_statusBar = new QStatusBar;
    m_statusBar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->setMargin(0);
    vlayout->setSpacing(0);
    vlayout->addLayout(m_modesStack);
    vlayout->addWidget(m_statusBar);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(1);
    layout->addWidget(m_selectionWidget);
    layout->addLayout(vlayout);
    setLayout(layout);

    connect(m_tabBar, SIGNAL(currentChanged(int)), this, SLOT(showWidget(int)));
}

void FancyTabWidget::insertTab(int index, QWidget *tab, const QIcon &icon, const QString &label)
{
    m_modesStack->insertWidget(index, tab);
    m_tabBar->insertTab(index, icon, label);
}

void FancyTabWidget::removeTab(int index)
{
    m_modesStack->removeWidget(m_modesStack->widget(index));
    m_tabBar->removeTab(index);
}

void FancyTabWidget::setBackgroundBrush(const QBrush &brush)
{
    QPalette pal = m_tabBar->palette();
    pal.setBrush(QPalette::Mid, brush);
    m_tabBar->setPalette(pal);
    pal = m_cornerWidgetContainer->palette();
    pal.setBrush(QPalette::Mid, brush);
    m_cornerWidgetContainer->setPalette(pal);
}

void FancyTabWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);

    QRect rect = m_selectionWidget->rect().adjusted(0, 0, 1, 0);
    rect = style()->visualRect(layoutDirection(), geometry(), rect);
    StyleHelper::verticalGradient(&p, rect, rect);
    p.setPen(StyleHelper::borderColor());
    p.drawLine(rect.topRight(), rect.bottomRight());
}

void FancyTabWidget::insertCornerWidget(int pos, QWidget *widget)
{
    QVBoxLayout *layout = static_cast<QVBoxLayout *>(m_cornerWidgetContainer->layout());
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

void FancyTabWidget::setCurrentIndex(int index)
{
    m_tabBar->setCurrentIndex(index);
}

void FancyTabWidget::showWidget(int index)
{
    emit currentAboutToShow(index);
    m_modesStack->setCurrentIndex(index);
    emit currentChanged(index);
}

void FancyTabWidget::setTabToolTip(int index, const QString &toolTip)
{
    m_tabBar->setTabToolTip(index, toolTip);
}
