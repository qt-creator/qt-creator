/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "navigationsubwidget.h"
#include "navigationwidget.h"

#include "coreconstants.h"
#include "inavigationwidgetfactory.h"
#include "actionmanager/command.h"
#include "id.h"

#include <utils/styledbar.h>

#include <QDebug>

#include <QHBoxLayout>
#include <QMenu>
#include <QResizeEvent>
#include <QToolButton>

Q_DECLARE_METATYPE(Core::INavigationWidgetFactory *)

namespace Core {
namespace Internal {

////
// NavigationSubWidget
////

NavigationSubWidget::NavigationSubWidget(NavigationWidget *parentWidget, int position, int factoryIndex)
    : m_parentWidget(parentWidget),
      m_position(position)
{
    m_navigationComboBox = new NavComboBox(this);
    m_navigationComboBox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_navigationComboBox->setFocusPolicy(Qt::TabFocus);
    m_navigationComboBox->setMinimumContentsLength(0);
    m_navigationComboBox->setModel(parentWidget->factoryModel());
    m_navigationWidget = 0;
    m_navigationWidgetFactory = 0;

    m_toolBar = new Utils::StyledBar(this);
    QHBoxLayout *toolBarLayout = new QHBoxLayout;
    toolBarLayout->setMargin(0);
    toolBarLayout->setSpacing(0);
    m_toolBar->setLayout(toolBarLayout);
    toolBarLayout->addWidget(m_navigationComboBox);

    QToolButton *splitAction = new QToolButton();
    splitAction->setIcon(QIcon(QLatin1String(Constants::ICON_SPLIT_HORIZONTAL)));
    splitAction->setToolTip(tr("Split"));
    splitAction->setPopupMode(QToolButton::InstantPopup);
    splitAction->setProperty("noArrow", true);
    m_splitMenu = new QMenu(splitAction);
    splitAction->setMenu(m_splitMenu);
    connect(m_splitMenu, &QMenu::aboutToShow, this, &NavigationSubWidget::populateSplitMenu);

    m_closeButton = new QToolButton();
    m_closeButton->setIcon(QIcon(QLatin1String(Constants::ICON_CLOSE_SPLIT_BOTTOM)));
    m_closeButton->setToolTip(tr("Close"));

    toolBarLayout->addWidget(splitAction);
    toolBarLayout->addWidget(m_closeButton);

    QVBoxLayout *lay = new QVBoxLayout();
    lay->setMargin(0);
    lay->setSpacing(0);
    setLayout(lay);
    lay->addWidget(m_toolBar);

    connect(m_closeButton, SIGNAL(clicked()), this, SIGNAL(closeMe()));

    setFactoryIndex(factoryIndex);

    connect(m_navigationComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(comboBoxIndexChanged(int)));

    comboBoxIndexChanged(factoryIndex);
}

NavigationSubWidget::~NavigationSubWidget()
{
}

void NavigationSubWidget::comboBoxIndexChanged(int factoryIndex)
{
    saveSettings();

    // Remove toolbutton
    foreach (QWidget *w, m_additionalToolBarWidgets)
        delete w;
    m_additionalToolBarWidgets.clear();

    // Remove old Widget
    delete m_navigationWidget;
    m_navigationWidget = 0;
    m_navigationWidgetFactory = 0;
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
    QHBoxLayout *layout = qobject_cast<QHBoxLayout *>(m_toolBar->layout());
    foreach (QToolButton *w, m_additionalToolBarWidgets) {
        layout->insertWidget(layout->count()-2, w);
    }

    restoreSettings();
}

void NavigationSubWidget::populateSplitMenu()
{
    m_splitMenu->clear();
    QAbstractItemModel *factoryModel = m_parentWidget->factoryModel();
    int count = factoryModel->rowCount();
    for (int i = 0; i < count; ++i) {
        QModelIndex index = factoryModel->index(i, 0);
        QAction *action = m_splitMenu->addAction(factoryModel->data(index).toString());
        connect(action, &QAction::triggered, this, [this, i]() { emit splitMe(i); });
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
    factory()->saveSettings(position(), m_navigationWidget);
}

void NavigationSubWidget::restoreSettings()
{
    if (!m_navigationWidget || !factory())
        return;
    factory()->restoreSettings(position(), m_navigationWidget);
}

Core::Command *NavigationSubWidget::command(const QString &title) const
{
    const QHash<Id, Command *> commandMap = m_parentWidget->commandMap();
    QHash<Id, Command *>::const_iterator r = commandMap.find(Id::fromString(title));
    if (r != commandMap.end())
        return r.value();
    return 0;
}

void NavigationSubWidget::setCloseIcon(const QIcon &icon)
{
    m_closeButton->setIcon(icon);
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
            const QString tooltip = tr("Activate %1 View").arg(text);
            setToolTip(cmd->stringWithAppendedShortcut(tooltip));
        } else {
            setToolTip(text);
        }
    }
    return QComboBox::event(e);
}

} // namespace Internal
} // namespace Core
