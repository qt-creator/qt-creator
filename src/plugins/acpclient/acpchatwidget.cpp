// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpchatwidget.h"
#include "acpchattab.h"
#include "acpclienttr.h"

#include <utils/utilsicons.h>
#include <utils/widgets.h>

#include <QHBoxLayout>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <coreplugin/rightpane.h>

namespace AcpClient::Internal {

class TabWidget : public QTabWidget
{
public:
    TabWidget(QWidget *parent = nullptr)
        : QTabWidget(parent)
    {
        setTabBar(new Utils::DocumentTabBar(this));
    }
};

AcpChatWidget::AcpChatWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *toolBar = new Utils::StyledBar(this);
    auto *toolBarLayout = new QHBoxLayout(toolBar);
    toolBarLayout->setContentsMargins(0, 0, 0, 0);
    toolBarLayout->setSpacing(0);
    layout->addWidget(toolBar);

    auto *addButton = new QToolButton(toolBar);
    addButton->setIcon(Utils::Icons::PLUS_TOOLBAR.icon());
    addButton->setText(Tr::tr("Add Chat"));
    addButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    addButton->setToolTip(addButton->text());
    connect(addButton, &QToolButton::clicked, this, [this] {
        AcpChatTab *tab = addNewTab();
        tab->setFocus();
    });
    toolBarLayout->addWidget(addButton);

    toolBarLayout->addStretch();

    auto *closeButton = new QToolButton(toolBar);
    closeButton->setIcon(Utils::Icons::CLOSE_SPLIT_RIGHT.icon());
    closeButton->setToolTip(Tr::tr("Close"));
    connect(closeButton, &QToolButton::clicked, this, [] {
        Core::RightPaneWidget::instance()->setShown(false);
    });
    toolBarLayout->addWidget(closeButton);

    m_tabWidget = new TabWidget;
    m_tabWidget->setTabBarAutoHide(false);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->tabBar()->setShape(QTabBar::RoundedNorth);
    m_tabWidget->setMovable(true);
    layout->addWidget(m_tabWidget);

    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &AcpChatWidget::closeTab);

    // Auto-create first tab
    addNewTab();
}

AcpChatWidget::~AcpChatWidget() = default;

void AcpChatWidget::setInspector(AcpInspector *inspector)
{
    m_inspector = inspector;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (auto *tab = qobject_cast<AcpChatTab *>(m_tabWidget->widget(i)))
            tab->setInspector(inspector);
    }
}

void AcpChatWidget::closeTab(int index)
{
    QWidget *w = m_tabWidget->widget(index);
    m_tabWidget->removeTab(index);
    delete w;
    emit navigateStateUpdate();

    if (m_tabWidget->count() == 0) {
        addNewTab();
        Core::RightPaneWidget::instance()->setShown(false);
    }
}

void AcpChatWidget::setFocus()
{
    if (auto *tab = currentTab())
        tab->setFocus();
}

AcpChatTab *AcpChatWidget::addNewTab()
{
    auto *tab = new AcpChatTab(m_tabWidget);
    if (m_inspector)
        tab->setInspector(m_inspector);

    const int index = m_tabWidget->addTab(tab, tab->title());
    m_tabWidget->setCurrentIndex(index);

    connect(tab, &AcpChatTab::titleChanged, this, [this, tab] {
        const int i = m_tabWidget->indexOf(tab);
        if (i >= 0)
            m_tabWidget->setTabText(i, tab->title());
    });

    emit navigateStateUpdate();
    return tab;
}

AcpChatTab *AcpChatWidget::currentTab() const
{
    return qobject_cast<AcpChatTab *>(m_tabWidget->currentWidget());
}

} // namespace AcpClient::Internal
