/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "doubletabwidget.h"
#include "ui_doubletabwidget.h"

#include <utils/stylehelper.h>

#include <QRect>
#include <QPainter>
#include <QFont>
#include <QMouseEvent>
#include <QMenu>
#include <QToolTip>

#include <QDebug>

using namespace ProjectExplorer::Internal;

static const int MIN_LEFT_MARGIN = 50;
static const int MARGIN = 12;
static const int OTHER_HEIGHT = 38;
static const int SELECTION_IMAGE_WIDTH = 10;
static const int SELECTION_IMAGE_HEIGHT = 20;
static const int OVERFLOW_DROPDOWN_WIDTH = Utils::StyleHelper::navigationWidgetHeight();

static void drawFirstLevelSeparator(QPainter *painter, QPoint top, QPoint bottom)
{
    QLinearGradient grad(top, bottom);
    grad.setColorAt(0, QColor(255, 255, 255, 20));
    grad.setColorAt(0.4, QColor(255, 255, 255, 60));
    grad.setColorAt(0.7, QColor(255, 255, 255, 50));
    grad.setColorAt(1, QColor(255, 255, 255, 40));
    painter->setPen(QPen(grad, 0));
    painter->drawLine(top, bottom);
    grad.setColorAt(0, QColor(0, 0, 0, 30));
    grad.setColorAt(0.4, QColor(0, 0, 0, 70));
    grad.setColorAt(0.7, QColor(0, 0, 0, 70));
    grad.setColorAt(1, QColor(0, 0, 0, 40));
    painter->setPen(QPen(grad, 0));
    painter->drawLine(top - QPoint(1,0), bottom - QPoint(1,0));
}

static void drawSecondLevelSeparator(QPainter *painter, QPoint top, QPoint bottom)
{
    QLinearGradient grad(top, bottom);
    grad.setColorAt(0, QColor(255, 255, 255, 0));
    grad.setColorAt(0.4, QColor(255, 255, 255, 100));
    grad.setColorAt(0.7, QColor(255, 255, 255, 100));
    grad.setColorAt(1, QColor(255, 255, 255, 0));
    painter->setPen(QPen(grad, 0));
    painter->drawLine(top, bottom);
    grad.setColorAt(0, QColor(0, 0, 0, 0));
    grad.setColorAt(0.4, QColor(0, 0, 0, 100));
    grad.setColorAt(0.7, QColor(0, 0, 0, 100));
    grad.setColorAt(1, QColor(0, 0, 0, 0));
    painter->setPen(QPen(grad, 0));
    painter->drawLine(top - QPoint(1,0), bottom - QPoint(1,0));
}

DoubleTabWidget::DoubleTabWidget(QWidget *parent) :
    QWidget(parent),
    m_left(QLatin1String(":/projectexplorer/images/leftselection.png")),
    m_mid(QLatin1String(":/projectexplorer/images/midselection.png")),
    m_right(QLatin1String(":/projectexplorer/images/rightselection.png")),
    ui(new Ui::DoubleTabWidget),
    m_currentIndex(-1),
    m_lastVisibleIndex(-1)
{
    ui->setupUi(this);
}

DoubleTabWidget::~DoubleTabWidget()
{
    delete ui;
}

int DoubleTabWidget::currentIndex() const
{
    return m_currentIndex;
}

void DoubleTabWidget::setCurrentIndex(int index)
{
    Q_ASSERT(index < m_tabs.size());
    if (index == m_currentIndex)
        return;
    m_currentIndex = index;
    emit currentIndexChanged(m_currentIndex, m_tabs.at(m_currentIndex).currentSubTab);
    update();
}

int DoubleTabWidget::currentSubIndex() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_tabs.size())
        return m_tabs.at(m_currentIndex).currentSubTab;
    return -1;
}

void DoubleTabWidget::setTitle(const QString &title)
{
    m_title = title;
    update();
}

QSize DoubleTabWidget::minimumSizeHint() const
{
    return QSize(0, Utils::StyleHelper::navigationWidgetHeight() + OTHER_HEIGHT + 1);
}

void DoubleTabWidget::updateNameIsUniqueAdd(Tab *tab)
{
    tab->nameIsUnique = true;
    for (int i=0; i < m_tabs.size(); ++i) {
        if (m_tabs.at(i).name == tab->name) {
            m_tabs[i].nameIsUnique = false;
            tab->nameIsUnique = false;
            break;
        }
    }
}

void DoubleTabWidget::updateNameIsUniqueRemove(const Tab &tab)
{
    if (tab.nameIsUnique)
        return;
    int index;
    int count = 0;
    for (int i=0; i < m_tabs.size(); ++i) {
        if (m_tabs.at(i).name == tab.name) {
            ++count;
            index = i;
        }
    }

    if (count == 1)
        m_tabs[index].nameIsUnique = true;
}

void DoubleTabWidget::addTab(const QString &name, const QString &fullName, const QStringList &subTabs)
{
    Tab tab;
    tab.name = name;
    tab.fullName = fullName;
    tab.subTabs = subTabs;
    tab.currentSubTab = tab.subTabs.isEmpty() ? -1 : 0;
    updateNameIsUniqueAdd(&tab);

    m_tabs.append(tab);
    update();
}

void DoubleTabWidget::insertTab(int index, const QString &name, const QString &fullName, const QStringList &subTabs)
{
    Tab tab;
    tab.name = name;
    tab.fullName = fullName;
    tab.subTabs = subTabs;
    tab.currentSubTab = tab.subTabs.isEmpty() ? -1 : 0;
    updateNameIsUniqueAdd(&tab);

    m_tabs.insert(index, tab);
    if (m_currentIndex >= index) {
        ++m_currentIndex;
        emit currentIndexChanged(m_currentIndex, m_tabs.at(m_currentIndex).currentSubTab);
    }
    update();
}

void DoubleTabWidget::removeTab(int index)
{
    Tab t = m_tabs.takeAt(index);
    updateNameIsUniqueRemove(t);
    if (index <= m_currentIndex) {
        --m_currentIndex;
        if (m_currentIndex < 0 && m_tabs.size() > 0)
            m_currentIndex = 0;
        if (m_currentIndex < 0)
            emit currentIndexChanged(-1, -1);
        else
            emit currentIndexChanged(m_currentIndex, m_tabs.at(m_currentIndex).currentSubTab);
    }
    update();
}

int DoubleTabWidget::tabCount() const
{
    return m_tabs.size();
}

/// Converts a position to the tab/subtab that is undeneath
/// If HitArea is tab or subtab, then the second part of the pair
/// is the tab or subtab number
QPair<DoubleTabWidget::HitArea, int> DoubleTabWidget::convertPosToTab(QPoint pos)
{
    if (pos.y() < Utils::StyleHelper::navigationWidgetHeight()) {
        // on the top level part of the bar
        int eventX = pos.x();
        QFontMetrics fm(font());
        int x = m_title.isEmpty() ? 0 :
                2 * MARGIN + qMax(fm.width(m_title), MIN_LEFT_MARGIN);

        if (eventX <= x)
            return qMakePair(HITNOTHING, -1);
        int i;
        for (i = 0; i <= m_lastVisibleIndex; ++i) {
            int otherX = x + 2 * MARGIN + fm.width(m_tabs.at(
                    m_currentTabIndices.at(i)).displayName());
            if (eventX > x && eventX < otherX)
                break;
            x = otherX;
        }
        if (i <= m_lastVisibleIndex) {
            return qMakePair(HITTAB, i);
        } else if (m_lastVisibleIndex < m_tabs.size() - 1) {
            // handle overflow menu
            if (eventX > x && eventX < x + OVERFLOW_DROPDOWN_WIDTH)
                return qMakePair(HITOVERFLOW, -1);
        }
    } else if (pos.y() < Utils::StyleHelper::navigationWidgetHeight() + OTHER_HEIGHT) {
        int diff = (OTHER_HEIGHT - SELECTION_IMAGE_HEIGHT) / 2;
        if (pos.y() < Utils::StyleHelper::navigationWidgetHeight() + diff
                || pos.y() > Utils::StyleHelper::navigationWidgetHeight() + OTHER_HEIGHT - diff)
            return qMakePair(HITNOTHING, -1);
        // on the lower level part of the bar
        if (m_currentIndex == -1)
            return qMakePair(HITNOTHING, -1);
        Tab currentTab = m_tabs.at(m_currentIndex);
        QStringList subTabs = currentTab.subTabs;
        if (subTabs.isEmpty())
            return qMakePair(HITNOTHING, -1);
        int eventX = pos.x();
        QFontMetrics fm(font());
        int x = MARGIN;
        int i;
        for (i = 0; i < subTabs.size(); ++i) {
            int otherX = x + 2 * SELECTION_IMAGE_WIDTH + fm.width(subTabs.at(i));
            if (eventX > x && eventX < otherX)
                break;
            x = otherX + 2 * MARGIN;
        }
        if (i < subTabs.size())
            return qMakePair(HITSUBTAB, i);
    }
    return qMakePair(HITNOTHING, -1);
}

void DoubleTabWidget::mousePressEvent(QMouseEvent *event)
{
    // todo:
    // the even wasn't accepted/ignored in a consistent way
    // now the event is accepted everywhere were it hitted something interesting
    // and otherwise ignored
    // should not make any difference
    QPair<HitArea, int> hit = convertPosToTab(event->pos());
    if (hit.first == HITTAB) {
        if (m_currentIndex != m_currentTabIndices.at(hit.second)) {
            m_currentIndex = m_currentTabIndices.at(hit.second);
            update();
            event->accept();
            emit currentIndexChanged(m_currentIndex, m_tabs.at(m_currentIndex).currentSubTab);
            return;
        }
    } else if (hit.first == HITOVERFLOW) {
        QMenu overflowMenu;
        QList<QAction *> actions;
        for (int i = m_lastVisibleIndex + 1; i < m_tabs.size(); ++i) {
            actions << overflowMenu.addAction(m_tabs.at(m_currentTabIndices.at(i)).displayName());
        }
        if (QAction *action = overflowMenu.exec(event->globalPos())) { // todo used different position before
            int index = m_currentTabIndices.at(actions.indexOf(action) + m_lastVisibleIndex + 1);
            if (m_currentIndex != index) {
                m_currentIndex = index;
                update();
                event->accept();
                emit currentIndexChanged(m_currentIndex, m_tabs.at(m_currentIndex).currentSubTab);
                return;
            }
        }
    } else if (hit.first == HITSUBTAB) {
        if (m_tabs[m_currentIndex].currentSubTab != hit.second) {
            m_tabs[m_currentIndex].currentSubTab = hit.second;
            update();
            // todo next two lines were outside the if leading to
            // unnecessary (?) signal emissions?
            event->accept();
            emit currentIndexChanged(m_currentIndex, m_tabs.at(m_currentIndex).currentSubTab);
            return;
        }
    }
    event->ignore();
}

void DoubleTabWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    QRect r = rect();

    // draw top level tab bar
    r.setHeight(Utils::StyleHelper::navigationWidgetHeight());

    QPoint offset = window()->mapToGlobal(QPoint(0, 0)) - mapToGlobal(r.topLeft());
    QRect gradientSpan = QRect(offset, window()->size());
    Utils::StyleHelper::horizontalGradient(&painter, gradientSpan, r);

    painter.setPen(Utils::StyleHelper::borderColor());

    QColor lighter(255, 255, 255, 40);
    painter.drawLine(r.bottomLeft(), r.bottomRight());
    painter.setPen(lighter);
    painter.drawLine(r.topLeft(), r.topRight());

    QFontMetrics fm(font());
    int baseline = (r.height() + fm.ascent()) / 2 - 1;

    // top level title
    if (!m_title.isEmpty()) {
        painter.setPen(Utils::StyleHelper::panelTextColor());
        painter.drawText(MARGIN, baseline, m_title);
    }

    QLinearGradient grad(QPoint(0, 0), QPoint(0, r.height() + OTHER_HEIGHT - 1));
    grad.setColorAt(0, QColor(247, 247, 247));
    grad.setColorAt(1, QColor(205, 205, 205));

    // draw background of second bar
    painter.fillRect(QRect(0, r.height(), r.width(), OTHER_HEIGHT), grad);
    painter.setPen(QColor(0x505050));
    painter.drawLine(0, r.height() + OTHER_HEIGHT,
                     r.width(), r.height() + OTHER_HEIGHT);
    painter.setPen(Qt::white);
    painter.drawLine(0, r.height(),
                     r.width(), r.height());

    // top level tabs
    int x = m_title.isEmpty() ? 0 :
            2 * MARGIN + qMax(fm.width(m_title), MIN_LEFT_MARGIN);

    // calculate sizes
    QList<int> nameWidth;
    int width = x;
    int indexSmallerThanOverflow = -1;
    int indexSmallerThanWidth = -1;
    for (int i = 0; i < m_tabs.size(); ++i) {
        const Tab &tab = m_tabs.at(i);
        int w = fm.width(tab.displayName());
        nameWidth << w;
        width += 2 * MARGIN + w;
        if (width < r.width())
            indexSmallerThanWidth = i;
        if (width < r.width() - OVERFLOW_DROPDOWN_WIDTH)
            indexSmallerThanOverflow = i;
    }
    m_lastVisibleIndex = -1;
    m_currentTabIndices.resize(m_tabs.size());
    if (indexSmallerThanWidth == m_tabs.size() - 1) {
        // => everything fits
        for (int i = 0; i < m_tabs.size(); ++i)
            m_currentTabIndices[i] = i;
        m_lastVisibleIndex = m_tabs.size()-1;
    } else {
        // => we need the overflow thingy
        if (m_currentIndex <= indexSmallerThanOverflow) {
            // easy going, simply draw everything that fits
            for (int i = 0; i < m_tabs.size(); ++i)
                m_currentTabIndices[i] = i;
            m_lastVisibleIndex = indexSmallerThanOverflow;
        } else {
            // now we need to put the current tab into
            // visible range. for that we need to find the place
            // to put it, so it fits
            width = x;
            int index = 0;
            bool handledCurrentIndex = false;
            for (int i = 0; i < m_tabs.size(); ++i) {
                if (index != m_currentIndex) {
                    if (!handledCurrentIndex) {
                        // check if enough room for current tab after this one
                        if (width + 2 * MARGIN + nameWidth.at(index)
                                + 2 * MARGIN + nameWidth.at(m_currentIndex)
                                < r.width() - OVERFLOW_DROPDOWN_WIDTH) {
                            m_currentTabIndices[i] = index;
                            ++index;
                            width += 2 * MARGIN + nameWidth.at(index);
                        } else {
                            m_currentTabIndices[i] = m_currentIndex;
                            handledCurrentIndex = true;
                            m_lastVisibleIndex = i;
                        }
                    } else {
                        m_currentTabIndices[i] = index;
                        ++index;
                    }
                } else {
                    ++index;
                    --i;
                }
            }
        }
    }

    // actually draw top level tabs
    for (int i = 0; i <= m_lastVisibleIndex; ++i) {
        int actualIndex = m_currentTabIndices.at(i);
        Tab tab = m_tabs.at(actualIndex);
        if (actualIndex == m_currentIndex) {
            painter.setPen(Utils::StyleHelper::borderColor());
            painter.drawLine(x - 1, 0, x - 1, r.height() - 1);
            painter.fillRect(QRect(x, 0,
                                   2 * MARGIN + fm.width(tab.displayName()),
                                   r.height() + 1),
                             grad);

            if (actualIndex != 0) {
                painter.setPen(QColor(255, 255, 255, 170));
                painter.drawLine(x, 0, x, r.height());
            }
            x += MARGIN;
            painter.setPen(Qt::black);
            painter.drawText(x, baseline, tab.displayName());
            x += nameWidth.at(actualIndex);
            x += MARGIN;
            painter.setPen(Utils::StyleHelper::borderColor());
            painter.drawLine(x, 0, x, r.height() - 1);
            painter.setPen(QColor(0, 0, 0, 20));
            painter.drawLine(x + 1, 0, x + 1, r.height() - 1);
            painter.setPen(QColor(255, 255, 255, 170));
            painter.drawLine(x - 1, 0, x - 1, r.height());
        } else {
            if (i == 0)
                drawFirstLevelSeparator(&painter, QPoint(x, 0), QPoint(x, r.height()-1));
            x += MARGIN;
            painter.setPen(Utils::StyleHelper::panelTextColor());
            painter.drawText(x + 1, baseline, tab.displayName());
            x += nameWidth.at(actualIndex);
            x += MARGIN;
            drawFirstLevelSeparator(&painter, QPoint(x, 0), QPoint(x, r.height()-1));
        }
    }

    // draw overflow button
    if (m_lastVisibleIndex < m_tabs.size() - 1) {
        QStyleOption opt;
        opt.rect = QRect(x, 0, OVERFLOW_DROPDOWN_WIDTH - 1, r.height() - 1);
        style()->drawPrimitive(QStyle::PE_IndicatorArrowDown,
                               &opt, &painter, this);
        drawFirstLevelSeparator(&painter, QPoint(x + OVERFLOW_DROPDOWN_WIDTH, 0),
                                QPoint(x + OVERFLOW_DROPDOWN_WIDTH, r.height()-1));
    }

    // second level tabs
    if (m_currentIndex != -1) {
        int y = r.height() + (OTHER_HEIGHT - m_left.height()) / 2.;
        int imageHeight = m_left.height();
        Tab currentTab = m_tabs.at(m_currentIndex);
        QStringList subTabs = currentTab.subTabs;
        x = 0;
        for (int i = 0; i < subTabs.size(); ++i) {
            x += MARGIN;
            int textWidth = fm.width(subTabs.at(i));
            if (currentTab.currentSubTab == i) {
                painter.setPen(Qt::white);
                painter.drawPixmap(x, y, m_left);
                painter.drawPixmap(QRect(x + SELECTION_IMAGE_WIDTH, y,
                                         textWidth, imageHeight),
                                   m_mid, QRect(0, 0, m_mid.width(), m_mid.height()));
                painter.drawPixmap(x + SELECTION_IMAGE_WIDTH + textWidth, y, m_right);
            } else {
                painter.setPen(Qt::black);
            }
            x += SELECTION_IMAGE_WIDTH;
            painter.drawText(x, y + (imageHeight + fm.ascent()) / 2. - 1,
                             subTabs.at(i));
            x += textWidth + SELECTION_IMAGE_WIDTH + MARGIN;
            drawSecondLevelSeparator(&painter, QPoint(x, y), QPoint(x, y + imageHeight));
        }
    }
}

void DoubleTabWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

bool DoubleTabWidget::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpevent = static_cast<QHelpEvent*>(event);
        QPair<HitArea, int> hit = convertPosToTab(helpevent->pos());
        if (hit.first == HITTAB && m_tabs.at(m_currentTabIndices.at(hit.second)).nameIsUnique)
            QToolTip::showText(helpevent->globalPos(), m_tabs.at(m_currentTabIndices.at(hit.second)).fullName, this);
        else
            QToolTip::showText(helpevent->globalPos(), QString(), this);
    }
    return QWidget::event(event);
}
