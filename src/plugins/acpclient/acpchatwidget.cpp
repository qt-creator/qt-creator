// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpchatwidget.h"
#include "acpchattab.h"
#include "acpclienttr.h"

#include <utils/documenttabbar.h>
#include <utils/utilsicons.h>

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
        setTabBar(new Utils::DocumentTabBar);
    }
};

AcpChatWidget::AcpChatWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tabWidget = new TabWidget;
    m_tabWidget->setTabBarAutoHide(false);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    layout->addWidget(m_tabWidget);

    auto *cornerButton = new QToolButton(m_tabWidget);
    cornerButton->setIcon(Utils::Icons::PLUS_TOOLBAR.icon());
    cornerButton->setToolTip(Tr::tr("New Chat Tab"));
    cornerButton->setAutoRaise(true);
    connect(cornerButton, &QToolButton::clicked, this, [this] {
        AcpChatTab *tab = addNewTab();
        tab->setFocus();
    });
    m_tabWidget->setCornerWidget(cornerButton, Qt::TopRightCorner);

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
