// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpchatwidget.h"
#include "acpchattab.h"
#include "acpclienttr.h"

#include <utils/utilsicons.h>
#include <utils/widgets.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QTabBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <coreplugin/generalsettings.h>
#include <coreplugin/rightpane.h>
#include <utils/stylehelper.h>

namespace AcpClient::Internal {

AcpChatWidget::AcpChatWidget(QWidget *parent)
    : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto toolBar = new Utils::StyledBar(this);
    auto *toolBarLayout = new QHBoxLayout(toolBar);
    toolBarLayout->setContentsMargins(0, 0, 0, 0);
    toolBarLayout->setSpacing(0);
    layout->addWidget(toolBar);

    m_addButton = new QToolButton(toolBar);
    m_addButton->setIcon(Utils::Icons::PLUS_TOOLBAR.icon());
    m_addButton->setText(Tr::tr("Add Chat"));
    m_addButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_addButton->setToolTip(m_addButton->text());
    connect(m_addButton, &QToolButton::clicked, this, [this] {
        AcpChatTab *tab = addNewTab();
        tab->setFocus();
    });
    toolBarLayout->addWidget(m_addButton);

    // Combobox selector (shown when "Use tabbed editors" is disabled).
    m_switcher = new QComboBox(toolBar);
    m_switcher->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_switcher->setToolTip(Tr::tr("Switch Chat"));
    connect(m_switcher, &QComboBox::currentIndexChanged,
            this, &AcpChatWidget::setCurrentIndex);
    toolBarLayout->addWidget(m_switcher);

    // Close-chat button (shown only in combobox mode; tabs carry their own close button).
    m_closeChatButton = new QToolButton(toolBar);
    m_closeChatButton->setIcon(Utils::Icons::CLOSE_TOOLBAR.icon());
    m_closeChatButton->setToolTip(Tr::tr("Close Chat"));
    m_closeChatButton->setProperty(Utils::StyleHelper::C_SHOW_BORDER, true);
    connect(m_closeChatButton, &QToolButton::clicked, this, [this] {
        closeTab(m_stack->currentIndex());
    });
    toolBarLayout->addWidget(m_closeChatButton);

    toolBarLayout->addStretch();

    auto *closeButton = new QToolButton(toolBar);
    closeButton->setIcon(Utils::Icons::CLOSE_SPLIT_RIGHT.icon());
    closeButton->setToolTip(Tr::tr("Close"));
    connect(closeButton, &QToolButton::clicked, this, [] {
        Core::RightPaneWidget::instance()->setShown(false);
    });
    toolBarLayout->addWidget(closeButton);

    // Tab-bar selector (shown when "Use tabbed editors" is enabled), between toolbar and stack.
    m_tabBar = new Utils::DocumentTabBar(this);
    m_tabBar->setDocumentMode(true);
    m_tabBar->setTabsClosable(true);
    m_tabBar->setShape(QTabBar::RoundedNorth);
    m_tabBar->setExpanding(false);
    connect(m_tabBar, &QTabBar::currentChanged, this, &AcpChatWidget::setCurrentIndex);
    connect(m_tabBar, &QTabBar::tabCloseRequested, this, &AcpChatWidget::closeTab);
    layout->addWidget(m_tabBar);

    m_stack = new QStackedWidget(this);
    layout->addWidget(m_stack);
    connect(m_stack, &QStackedWidget::currentChanged, this, &AcpChatWidget::setCurrentIndex);

    setUseTabs(Core::generalSettings().useTabsInEditorViews());
    Core::generalSettings().useTabsInEditorViews.addOnChanged(this, [this] {
        setUseTabs(Core::generalSettings().useTabsInEditorViews());
    });

    // Auto-create first tab
    addNewTab();
}

AcpChatWidget::~AcpChatWidget() = default;

void AcpChatWidget::setUseTabs(bool useTabs)
{
    m_tabBar->setVisible(useTabs);
    m_addButton->setProperty(Utils::StyleHelper::C_SHOW_BORDER, !useTabs);
    m_switcher->setVisible(!useTabs);
    m_closeChatButton->setVisible(!useTabs);
}

void AcpChatWidget::setCurrentIndex(int index)
{
    if (m_blockIndexChanges || index < 0)
        return;
    if (index != m_stack->currentIndex())
        m_stack->setCurrentIndex(index);
    if (index != m_tabBar->currentIndex())
        m_tabBar->setCurrentIndex(index);
    if (index != m_switcher->currentIndex())
        m_switcher->setCurrentIndex(index);
}

void AcpChatWidget::setInspector(AcpInspector *inspector)
{
    m_inspector = inspector;
    for (int i = 0; i < m_stack->count(); ++i) {
        if (auto *tab = qobject_cast<AcpChatTab *>(m_stack->widget(i)))
            tab->setInspector(inspector);
    }
}

void AcpChatWidget::closeTab(int index)
{
    if (index < 0)
        return;

    QWidget *w = m_stack->widget(index);
    m_blockIndexChanges = true;
    m_stack->removeWidget(w);
    m_tabBar->removeTab(index);
    m_switcher->removeItem(index);
    m_blockIndexChanges = false;
    setCurrentIndex(m_stack->currentIndex());
    delete w;

    emit navigateStateUpdate();

    if (m_stack->count() == 0) {
        addNewTab();
        Core::RightPaneWidget::instance()->setShown(false);
    }
}

void AcpChatWidget::setFocus()
{
    if (auto *tab = qobject_cast<AcpChatTab *>(m_stack->currentWidget()))
        tab->setFocus();
}

AcpChatTab *AcpChatWidget::addNewTab()
{
    auto *tab = new AcpChatTab;
    if (m_inspector)
        tab->setInspector(m_inspector);

    m_blockIndexChanges = true;
    const int index = m_stack->addWidget(tab);
    m_tabBar->insertTab(index, tab->title());
    m_switcher->insertItem(index, tab->title());
    m_blockIndexChanges = false;
    setCurrentIndex(index);

    connect(tab, &AcpChatTab::titleChanged, this, [this, tab] {
        const int i = m_stack->indexOf(tab);
        if (i < 0)
            return;
        m_tabBar->setTabText(i, tab->title());
        m_switcher->setItemText(i, tab->title());
    });

    emit navigateStateUpdate();
    return tab;
}

} // namespace AcpClient::Internal
