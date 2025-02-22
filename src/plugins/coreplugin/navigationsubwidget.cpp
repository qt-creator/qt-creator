// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "navigationsubwidget.h"

#include "actionmanager/actionmanager.h"
#include "actionmanager/command.h"
#include "coreplugintr.h"
#include "icore.h"
#include "inavigationwidgetfactory.h"
#include "navigationwidget.h"

#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QHBoxLayout>
#include <QMenu>
#include <QResizeEvent>
#include <QToolButton>

using namespace Utils;

namespace Core {
namespace Internal {

////
// NavigationSubWidget
////

NavigationSubWidget::NavigationSubWidget(NavigationWidget *parentWidget, int position, int factoryIndex)
    : QWidget(parentWidget),
      m_parentWidget(parentWidget),
      m_position(position)
{
    m_navigationComboBox = new NavComboBox(this);
    m_navigationComboBox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_navigationComboBox->setFocusPolicy(Qt::TabFocus);
    m_navigationComboBox->setMinimumContentsLength(0);
    m_navigationComboBox->setModel(parentWidget->factoryModel());
    m_navigationWidget = nullptr;
    m_navigationWidgetFactory = nullptr;

    m_toolBar = new Utils::StyledBar(this);
    auto toolBarLayout = new QHBoxLayout;
    toolBarLayout->setContentsMargins(0, 0, 0, 0);
    toolBarLayout->setSpacing(0);
    m_toolBar->setLayout(toolBarLayout);
    toolBarLayout->addWidget(m_navigationComboBox);

    auto splitAction = new QToolButton();
    splitAction->setIcon(Utils::Icons::SPLIT_HORIZONTAL_TOOLBAR.icon());
    splitAction->setToolTip(Tr::tr("Split"));
    splitAction->setPopupMode(QToolButton::InstantPopup);
    splitAction->setProperty(StyleHelper::C_NO_ARROW, true);
    m_splitMenu = new QMenu(splitAction);
    splitAction->setMenu(m_splitMenu);
    connect(m_splitMenu, &QMenu::aboutToShow, this, &NavigationSubWidget::populateSplitMenu);

    m_closeButton = new QToolButton();
    m_closeButton->setIcon(Utils::Icons::CLOSE_SPLIT_BOTTOM.icon());
    m_closeButton->setToolTip(Tr::tr("Close"));

    toolBarLayout->addWidget(splitAction);
    toolBarLayout->addWidget(m_closeButton);

    auto lay = new QVBoxLayout();
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    setLayout(lay);
    lay->addWidget(m_toolBar);

    connect(m_closeButton, &QAbstractButton::clicked, this, &NavigationSubWidget::closeMe);

    setFactoryIndex(factoryIndex);

    connect(m_navigationComboBox, &QComboBox::currentIndexChanged,
            this, &NavigationSubWidget::comboBoxIndexChanged);

    comboBoxIndexChanged(factoryIndex);
}

NavigationSubWidget::~NavigationSubWidget() = default;

void NavigationSubWidget::comboBoxIndexChanged(int factoryIndex)
{
    saveSettings();

    // Remove toolbutton
    for (QWidget *w : std::as_const(m_additionalToolBarWidgets))
        delete w;
    m_additionalToolBarWidgets.clear();

    // Remove old Widget
    delete m_navigationWidget;
    m_navigationWidget = nullptr;
    m_navigationWidgetFactory = nullptr;
    if (factoryIndex == -1)
        return;

    // Get new stuff
    m_navigationWidgetFactory = m_navigationComboBox->itemData(factoryIndex,
                           NavigationWidget::FactoryObjectRole).value<INavigationWidgetFactory *>();
    NavigationView n = m_navigationWidgetFactory->createWidget();
    m_navigationWidget = n.widget;
    layout()->addWidget(m_navigationWidget);

    // Add Toolbutton
    m_additionalToolBarWidgets = n.dockToolBarWidgets;
    auto layout = qobject_cast<QHBoxLayout *>(m_toolBar->layout());
    for (QToolButton *w : std::as_const(m_additionalToolBarWidgets))
        layout->insertWidget(layout->count()-2, w);

    restoreSettings();
    emit factoryIndexChanged(factoryIndex);
}

void NavigationSubWidget::populateSplitMenu()
{
    m_splitMenu->clear();
    QAbstractItemModel *factoryModel = m_parentWidget->factoryModel();
    int count = factoryModel->rowCount();
    for (int i = 0; i < count; ++i) {
        QModelIndex index = factoryModel->index(i, 0);
        const Id id = factoryModel->data(index, NavigationWidget::FactoryActionIdRole).value<Id>();
        Command *command = ActionManager::command(id);
        const QString factoryName = factoryModel->data(index).toString();
        const QString displayName = command->keySequence().isEmpty()
                                        ? factoryName
                                        : QString("%1 (%2)").arg(factoryName,
                                                                 command->keySequence().toString(
                                                                     QKeySequence::NativeText));
        QAction *action = m_splitMenu->addAction(displayName);
        connect(action, &QAction::triggered, this, [this, i] { emit splitMe(i); });
    }
}

void NavigationSubWidget::setFocusWidget()
{
    if (m_navigationWidget)
        m_navigationWidget->setFocus();
}

INavigationWidgetFactory *NavigationSubWidget::factory()
{
    return m_navigationWidgetFactory;
}


void NavigationSubWidget::saveSettings()
{
    if (!m_navigationWidget || !factory())
        return;

    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(m_parentWidget->settingsGroup());
    factory()->saveSettings(settings, position(), m_navigationWidget);
    settings->endGroup();
}

void NavigationSubWidget::restoreSettings()
{
    if (!m_navigationWidget || !factory())
        return;

    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(m_parentWidget->settingsGroup());
    factory()->restoreSettings(settings, position(), m_navigationWidget);
    settings->endGroup();
}

Core::Command *NavigationSubWidget::command(const QString &title) const
{
    const QHash<Id, Command *> commandMap = m_parentWidget->commandMap();
    QHash<Id, Command *>::const_iterator r = commandMap.find(Id::fromString(title));
    if (r != commandMap.end())
        return r.value();
    return nullptr;
}

void NavigationSubWidget::setCloseIcon(const QIcon &icon)
{
    m_closeButton->setIcon(icon);
}

QWidget *NavigationSubWidget::widget()
{
    return m_navigationWidget;
}

int NavigationSubWidget::factoryIndex() const
{
    return m_navigationComboBox->currentIndex();
}

void NavigationSubWidget::setFactoryIndex(int i)
{
    m_navigationComboBox->setCurrentIndex(i);
}

int NavigationSubWidget::position() const
{
    return m_position;
}

void NavigationSubWidget::setPosition(int position)
{
    m_position = position;
}

CommandComboBox::CommandComboBox(QWidget *parent) : QComboBox(parent)
{
}

bool CommandComboBox::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip) {
        const QString text = currentText();
        if (const Core::Command *cmd = command(text)) {
            const QString tooltip = Tr::tr("Activate %1 View").arg(text);
            setToolTip(cmd->stringWithAppendedShortcut(tooltip));
        } else {
            setToolTip(text);
        }
    }
    return QComboBox::event(e);
}

} // namespace Internal
} // namespace Core
