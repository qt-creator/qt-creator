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

#include "switchsplittabwidget.h"

#include <utils/utilsicons.h>

#include <QVector>
#include <QBoxLayout>
#include <QTabWidget>
#include <QTabBar>
#include <QToolButton>
#include <QSplitter>
#include <QLayoutItem>
#include <QEvent>

namespace QmlDesigner {
SwitchSplitTabWidget::SwitchSplitTabWidget(QWidget *parent)
    : QWidget(parent)
    , m_splitter(new QSplitter)
    , m_tabBar(new QTabBar)
    , m_tabBarBackground(new QWidget)
{
    // setting object names for css
    setObjectName("backgroundWidget");
    m_splitter->setObjectName("centralTabWidget");
    m_splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_tabBar->setObjectName("centralTabBar");
    m_tabBar->setShape(QTabBar::RoundedEast);
    m_tabBar->setDocumentMode(false);
    // add a faketab to have the possibility to unselect all tabs
    m_tabBar->addTab(QString());
    selectFakeTab();

    m_tabBarBackground->setObjectName("tabBarBackground");

    connect(m_tabBar, &QTabBar::tabBarClicked, [this] (int index) {
        if (index != -1)
            updateSplitterSizes(index - fakeTab);
    });

    setLayout(new QHBoxLayout);
    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->setSpacing(0);
    layout()->addWidget(m_splitter);

    m_tabBarBackground->setLayout(new QVBoxLayout);
    m_tabBarBackground->layout()->setContentsMargins(0, 0, 0, 0);
    m_tabBarBackground->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    m_tabBarBackground->layout()->addWidget(m_tabBar);

    QToolButton *horizontalButton = new QToolButton;
    horizontalButton->setIcon(Utils::Icons::SPLIT_HORIZONTAL.icon());
    connect(horizontalButton, &QToolButton::clicked, [this] () {
        m_splitter->setOrientation(Qt::Vertical);
        updateSplitterSizes();
        selectFakeTab();
    });
    QToolButton *verticalButton = new QToolButton;
    verticalButton->setIcon(Utils::Icons::SPLIT_VERTICAL.icon());
    connect(verticalButton, &QToolButton::clicked, [this] () {
        m_splitter->setOrientation(Qt::Horizontal);
        updateSplitterSizes();
        selectFakeTab();
    });

    m_tabBarBackground->layout()->addWidget(horizontalButton);
    m_tabBarBackground->layout()->addWidget(verticalButton);
    layout()->addWidget(m_tabBarBackground);
    updateSplitButtons();
}

int SwitchSplitTabWidget::count() const
{
    return m_splitter->count();
}

QWidget *SwitchSplitTabWidget::currentWidget() const
{
    QList<int> sizes = m_splitter->sizes();
    for (int i = 0; i < count(); ++i) {
        if (sizes.at(i) > 0 && m_splitter->widget(i)->hasFocus())
            return m_splitter->widget(i);
    }
    return nullptr;
}

void SwitchSplitTabWidget::updateSplitterSizes(int index)
{
    if (isHidden()) {
        // we can not get the sizes if the splitter is hidden
        m_splittSizesAreDirty = true;
        return;
    }
    QVector<int> splitterSizes = m_splitter->sizes().toVector();
    int splitterFullSize = 0;
    for (int size : splitterSizes)
        splitterFullSize += size;
    if (index > -1) {
        // collapse all but not the one at index
        splitterSizes.fill(0);
        splitterSizes.replace(index, splitterFullSize);
    } else {
        // distribute full size
        splitterSizes.fill(splitterFullSize / splitterSizes.count());
    }
    m_splitter->setSizes(splitterSizes.toList());
    m_splittSizesAreDirty = false;
}

int SwitchSplitTabWidget::addTab(QWidget *w, const QString &label)
{
    m_splitter->addWidget(w);
    const int newIndex = m_tabBar->addTab(label);
    if (mode() == TabMode) {
        m_tabBar->setCurrentIndex(newIndex);
        updateSplitterSizes(newIndex - fakeTab);
    }
    if (mode() == SplitMode)
        updateSplitterSizes();
    updateSplitButtons();
    return newIndex;
}

QWidget *SwitchSplitTabWidget::takeTabWidget(const int index)
{
    if (index == -1 || index > count() - 1)
        return nullptr;
    QWidget *widget = m_splitter->widget(index);
    widget->setParent(nullptr);
    m_tabBar->removeTab(index + fakeTab);
    // TODO: set which mode and tab is the current one
    updateSplitButtons();
    return widget;
}

void SwitchSplitTabWidget::switchTo(QWidget *widget)
{
    if (widget == nullptr || currentWidget() == widget)
        return;
    const int widgetIndex = m_splitter->indexOf(widget);
    Q_ASSERT(widgetIndex != -1);
    if (mode() == TabMode) {
        updateSplitterSizes(widgetIndex);
        m_tabBar->setCurrentIndex(widgetIndex + fakeTab);
    }
    widget->setFocus();
}

bool SwitchSplitTabWidget::event(QEvent *event)
{
    if (event->type() == QEvent::Show && m_splittSizesAreDirty) {
        bool returnValue = QWidget::event(event);
        updateSplitterSizes(m_tabBar->currentIndex() - fakeTab);
        return returnValue;
    }

    return QWidget::event(event);
}

void SwitchSplitTabWidget::updateSplitButtons()
{
    const bool isTabBarNecessary = count() > 1;
    m_tabBarBackground->setVisible(isTabBarNecessary);
}

void SwitchSplitTabWidget::selectFakeTab()
{
    m_tabBar->setCurrentIndex(0);
}

SwitchSplitTabWidget::Mode SwitchSplitTabWidget::mode()
{
    const bool isTabBarNecessary = count() > 1;
    const int fakeTabPosition = 0;
    const int hasSelectedTab = m_tabBar->currentIndex() > fakeTabPosition;
    if (isTabBarNecessary && !hasSelectedTab)
        return SplitMode;
    return TabMode;
}

} // namespace QmlDesigner
