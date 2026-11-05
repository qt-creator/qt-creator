// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpchatwidget.h"
#include "acpchattab.h"
#include "acpclienttr.h"
#include "acpsettings.h"
#include "acptermswidget.h"
#include "agentoverview.h"

#include <coreplugin/rightpane.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>
#include <utils/widgets.h>

#include <QHBoxLayout>
#include <QResizeEvent>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>

namespace AcpClient::Internal {

AcpChatWidget::AcpChatWidget(QWidget *parent)
    : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    auto toolBar = new Utils::StyledBar(this);
    auto *toolBarLayout = new QHBoxLayout(toolBar);
    toolBarLayout->setContentsMargins(0, 0, 0, 0);
    toolBarLayout->setSpacing(0);
    m_layout->addWidget(toolBar);

    // Shows the current chat and opens the list of all chats below the tool bar.
    m_switcherButton = new SessionSwitcherButton(toolBar);
    connect(m_switcherButton, &QAbstractButton::clicked, this, [this](bool checked) {
        setOverviewShown(checked);
        if (checked)
            m_overview->focusCurrentEntry();
    });
    toolBarLayout->addWidget(m_switcherButton);

    m_addButton = new QToolButton(toolBar);
    m_addButton->setIcon(Utils::Icons::PLUS_TOOLBAR.icon());
    m_addButton->setObjectName("newChatButton");
    m_addButton->setToolTip(Tr::tr("Add Chat"));
    connect(m_addButton, &QToolButton::clicked, this, &AcpChatWidget::addNewTabFromUser);
    toolBarLayout->addWidget(m_addButton);

    m_toolBarWidgetStack = new QStackedWidget(toolBar);
    toolBarLayout->addWidget(m_toolBarWidgetStack);

    toolBarLayout->addStretch();

    auto *closeButton = new QToolButton(toolBar);
    closeButton->setIcon(Utils::Icons::CLOSE_SPLIT_RIGHT.icon());
    closeButton->setToolTip(Tr::tr("Close"));
    connect(closeButton, &QToolButton::clicked, this, [] {
        Core::RightPaneWidget::instance()->setShown(false);
    });
    toolBarLayout->addWidget(closeButton);

    m_stack = new QStackedWidget;
    connect(m_stack, &QStackedWidget::currentChanged, this, &AcpChatWidget::setCurrentIndex);

    m_runningIndicator = new RunningIndicator(this);
    connect(m_runningIndicator, &RunningIndicator::iconChanged, this, [this] {
        m_overview->setRunningIcon(m_runningIndicator->icon());
        if (m_switcherButton->entry().activity == SessionActivity::Running)
            m_switcherButton->setIcon(m_runningIndicator->icon());
    });

    m_overview = new AgentOverview;
    m_overview->hide();
    connect(m_overview, &AgentOverview::entryActivated, this, [this](int index) {
        setCurrentIndex(index);
        setFocus();
    });
    connect(m_overview, &AgentOverview::entryCloseRequested,
            this, &AcpChatWidget::closeTab);
    connect(m_overview, &AgentOverview::dismissRequested, this, [this] {
        setOverviewShown(false);
        m_switcherButton->setFocus();
    });

    m_chatArea = new QWidget;
    auto *chatAreaLayout = new QVBoxLayout(m_chatArea);
    chatAreaLayout->setContentsMargins(0, 0, 0, 0);
    chatAreaLayout->setSpacing(0);
    chatAreaLayout->addWidget(m_overview);
    chatAreaLayout->addWidget(m_stack, 1);
    m_layout->addWidget(m_chatArea);

    updateTermsState();
}

AcpChatWidget::~AcpChatWidget() = default;

void AcpChatWidget::updateTermsState()
{
    const bool gated = acpTermsPending();
    if (gated && !m_termsWidget)
        createTermsWidget();
    if (m_termsWidget)
        m_termsWidget->setVisible(gated);
    updateContentVisibility();
    m_addButton->setEnabled(!gated);
    if (!gated && m_stack->count() == 0)
        addNewTab();
}

void AcpChatWidget::createTermsWidget()
{
    m_termsWidget = new AcpTermsWidget(this);
    connect(m_termsWidget, &AcpTermsWidget::accepted, this, &AcpChatWidget::updateTermsState);
    connect(m_termsWidget, &AcpTermsWidget::declined, this, [] {
        Core::RightPaneWidget::instance()->setShown(false);
    });
    m_layout->addWidget(m_termsWidget);
}

void AcpChatWidget::addNewTabFromUser()
{
    AcpChatTab *tab = addNewTab();
    setOverviewShown(false);
    tab->setFocus();
}

void AcpChatWidget::setOverviewShown(bool shown)
{
    m_overviewShown = shown;
    updateContentVisibility();
}

void AcpChatWidget::updateContentVisibility()
{
    const bool gated = acpTermsPending();
    m_chatArea->setVisible(!gated);
    m_overview->setVisible(m_overviewShown);
    m_switcherButton->setEnabled(!gated);
    m_switcherButton->setChecked(m_overviewShown);
}

void AcpChatWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // The list of chats takes at most a third of the room.
    m_overview->setMaximumHeight(event->size().height() / 3);
}

void AcpChatWidget::updateOverview()
{
    QList<AgentOverview::Entry> entries;
    bool anyRunning = false;
    for (int i = 0; i < m_stack->count(); ++i) {
        if (auto *tab = qobject_cast<AcpChatTab *>(m_stack->widget(i))) {
            entries.append({tab->agentName(), tab->toolTip(), tab->activity(),
                            tab->serverIcon()});
            anyRunning |= tab->activity() == SessionActivity::Running;
        }
    }
    m_runningIndicator->setActive(anyRunning);
    m_overview->setRunningIcon(m_runningIndicator->icon());
    m_overview->setEntries(entries);
    m_overview->setCurrentEntry(m_stack->currentIndex());
    updateSwitcherButton();
}

void AcpChatWidget::updateSwitcherButton()
{
    auto *tab = qobject_cast<AcpChatTab *>(m_stack->currentWidget());
    if (!tab)
        return;
    m_switcherButton->setEntry({tab->agentName(), tab->toolTip(), tab->activity(),
                                tab->serverIcon()},
                               m_runningIndicator->icon());
}

void AcpChatWidget::setCurrentIndex(int index)
{
    if (m_blockIndexChanges || index < 0)
        return;
    if (index != m_stack->currentIndex())
        m_stack->setCurrentIndex(index);
    if (index != m_toolBarWidgetStack->currentIndex())
        m_toolBarWidgetStack->setCurrentIndex(index);
    updateOverview();
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
    // Only detaches it from display; the owning ChatPanel deletes it.
    m_toolBarWidgetStack->removeWidget(m_toolBarWidgetStack->widget(index));
    m_blockIndexChanges = false;
    setCurrentIndex(m_stack->currentIndex());
    delete w;
    updateOverview();

    emit navigateStateUpdate();

    if (m_stack->count() == 0) {
        updateTermsState();
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
    m_toolBarWidgetStack->insertWidget(index, tab->toolBarWidget());
    m_blockIndexChanges = false;
    setCurrentIndex(index);

    connect(tab, &AcpChatTab::titleChanged, this, &AcpChatWidget::updateOverview);
    connect(tab, &AcpChatTab::activityChanged, this, &AcpChatWidget::updateOverview);
    connect(tab, &AcpChatTab::serverIconChanged, this, &AcpChatWidget::updateOverview);
    updateOverview();

    emit navigateStateUpdate();
    return tab;
}

} // namespace AcpClient::Internal
