/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "fancytabwidget.h"
#include <utils/stylehelper.h>
#include <utils/styledbar.h>

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
#include <QtGui/QToolButton>
#include <QtGui/QToolTip>

using namespace Core;
using namespace Internal;

const int FancyTabBar::m_rounding = 22;
const int FancyTabBar::m_textPadding = 4;

FancyTabBar::FancyTabBar(QWidget *parent)
    : QWidget(parent)
{
    m_hoverIndex = -1;
    m_currentIndex = -1;
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setStyle(new QWindowsStyle);
    setMinimumWidth(qMax(2 * m_rounding, 40));
    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::NoFocus);
    m_hoverControl.setFrameRange(0, 20);
    m_hoverControl.setDuration(130);
    m_hoverControl.setCurveShape(QTimeLine::EaseInCurve);
    connect(&m_hoverControl, SIGNAL(frameChanged(int)), this, SLOT(updateHover()));
    setMouseTracking(true); // Needed for hover events
}

FancyTabBar::~FancyTabBar()
{
    delete style();
}

QSize FancyTabBar::tabSizeHint(bool minimum) const
{
    QFont boldFont(font());
    boldFont.setPointSizeF(Utils::StyleHelper::sidebarFontSize());
    boldFont.setBold(true);
    QFontMetrics fm(boldFont);
    int spacing = 6;
    int width = 60 + spacing + 2;

    int iconHeight = minimum ? 0 : 32;
    return QSize(width, iconHeight + spacing + fm.height());
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
        int newHover = -1;
        for (int i = 0; i < count(); ++i) {
            QRect area = tabRect(i);
            if (area.contains(e->pos())) {
                newHover = i;
                break;
            }
        }

        m_hoverControl.stop();
        m_hoverIndex = newHover;
        update(m_hoverRect);
        m_hoverRect = QRect();

        if (m_hoverIndex >=0) {
            QRect oldHoverRect = m_hoverRect;
            m_hoverRect = tabRect(m_hoverIndex);
            m_hoverControl.start();
        }
    }
}

bool FancyTabBar::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        if (m_hoverIndex >= 0 && m_hoverIndex < m_tabs.count()) {
            QString tt = tabToolTip(m_hoverIndex);
            if (!tt.isEmpty()) {
                QToolTip::showText(static_cast<QHelpEvent*>(event)->globalPos(), tt, this);
                return true;
            }
        }
    }
    return QWidget::event(event);
}

void FancyTabBar::updateHover()
{
    update(m_hoverRect);
}

// Resets hover animation on mouse enter
void FancyTabBar::enterEvent(QEvent *e)
{
    Q_UNUSED(e)
    m_hoverRect = QRect();
    m_hoverIndex = -1;
}

// Resets hover animation on mouse enter
void FancyTabBar::leaveEvent(QEvent *e)
{
    Q_UNUSED(e)
    if (m_hoverIndex >= 0) {
        m_hoverIndex = -1;
        update(m_hoverRect);
        m_hoverRect = QRect();
    }
}

QSize FancyTabBar::sizeHint() const
{
    QSize sh = tabSizeHint();
    return QSize(sh.width(), sh.height() * m_tabs.count());
}

QSize FancyTabBar::minimumSizeHint() const
{
    QSize sh = tabSizeHint(true);
    return QSize(sh.width(), sh.height() * m_tabs.count());
}

QRect FancyTabBar::tabRect(int index) const
{
    QSize sh = tabSizeHint();

    if (sh.height() * m_tabs.count() > height())
        sh.setHeight(height() / m_tabs.count());

    return QRect(0, index * sh.height(), sh.width(), sh.height());

}

void FancyTabBar::mousePressEvent(QMouseEvent *e)
{
    e->accept();
    for (int i = 0; i < m_tabs.count(); ++i) {
        if (tabRect(i).contains(e->pos())) {
            setCurrentIndex(i);
            break;
        }
    }
}

void FancyTabBar::paintTab(QPainter *painter, int tabIndex) const
{
    painter->save();

    QRect rect = tabRect(tabIndex);

    bool selected = (tabIndex == m_currentIndex);
    bool hover = (tabIndex == m_hoverIndex);

#ifdef Q_WS_MAC
    hover = false; // Do not hover on Mac
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
        painter->fillRect(rect, background);
        if (hover)
            painter->fillRect(rect, hoverColor);
        painter->setPen(QPen(light, 0));
        painter->drawLine(rect.topLeft(), rect.topRight());
        painter->setPen(QPen(dark, 0));
        painter->drawLine(rect.bottomLeft(), rect.bottomRight());
    }

    QString tabText(this->tabText(tabIndex));
    QRect tabTextRect(tabRect(tabIndex));
    QRect tabIconRect(tabTextRect);
    QFont boldFont(painter->font());
    boldFont.setPointSizeF(Utils::StyleHelper::sidebarFontSize());
    boldFont.setBold(true);
    painter->setFont(boldFont);
    painter->setPen(selected ? Utils::StyleHelper::panelTextColor() : QColor(30, 30, 30, 80));
    int textFlags = Qt::AlignCenter | Qt::AlignBottom | Qt::ElideRight | Qt::TextWordWrap;
    painter->drawText(tabTextRect, textFlags, tabText);
    painter->setPen(selected ? QColor(60, 60, 60) : Utils::StyleHelper::panelTextColor());
    int textHeight = painter->fontMetrics().boundingRect(QRect(0, 0, width(), height()), Qt::TextWordWrap, tabText).height();
    tabIconRect.adjust(0, 4, 0, -textHeight);
    int iconSize = qMin(tabIconRect.width(), tabIconRect.height());
    if (iconSize > 4)
        style()->drawItemPixmap(painter, tabIconRect, Qt::AlignCenter | Qt::AlignVCenter,
                                tabIcon(tabIndex).pixmap(tabIconRect.size()));
    painter->translate(0, -1);
    painter->drawText(tabTextRect, textFlags, tabText);
    painter->restore();
}

void FancyTabBar::setCurrentIndex(int index) {
    m_currentIndex = index;
    update();
    emit currentChanged(index);
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
            Utils::StyleHelper::setBaseColor(QColorDialog::getColor(Utils::StyleHelper::baseColor(), m_parent));
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

    m_selectionWidget = new QWidget(this);
    QVBoxLayout *selectionLayout = new QVBoxLayout;
    selectionLayout->setSpacing(0);
    selectionLayout->setMargin(0);

    Utils::StyledBar *bar = new Utils::StyledBar;
    QHBoxLayout *layout = new QHBoxLayout(bar);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(new FancyColorButton(this));
    selectionLayout->addWidget(bar);

    selectionLayout->addWidget(m_tabBar, 1);
    m_selectionWidget->setLayout(selectionLayout);
    m_selectionWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    m_cornerWidgetContainer = new QWidget(this);
    m_cornerWidgetContainer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
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

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setMargin(0);
    mainLayout->setSpacing(1);
    mainLayout->addWidget(m_selectionWidget);
    mainLayout->addLayout(vlayout);
    setLayout(mainLayout);

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
    Utils::StyleHelper::verticalGradient(&p, rect, rect);
    p.setPen(Utils::StyleHelper::borderColor());
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
