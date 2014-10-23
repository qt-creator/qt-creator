/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debuggerkitconfigwidget.h"

#include "debuggeritemmanager.h"
#include "debuggeritemmodel.h"
#include "debuggerkitinformation.h"

#include <coreplugin/icore.h>

#include <projectexplorer/abi.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <utils/elidinglabel.h>
#include <utils/environment.h>
#include <utils/pathchooser.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QComboBox>
#include <QDirIterator>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QUuid>

using namespace ProjectExplorer;

namespace Debugger {
namespace Internal {

class DebuggerItemConfigWidget;

// -----------------------------------------------------------------------
// DebuggerKitConfigWidget
// -----------------------------------------------------------------------

DebuggerKitConfigWidget::DebuggerKitConfigWidget(Kit *workingCopy, const KitInformation *ki)
    : KitConfigWidget(workingCopy, ki)
{
    m_comboBox = new QComboBox;
    m_comboBox->setEnabled(true);
    m_comboBox->setToolTip(toolTip());
    m_comboBox->addItem(tr("None"), QString());
    foreach (const DebuggerItem &item, DebuggerItemManager::debuggers())
        m_comboBox->addItem(item.displayName(), item.id());

    refresh();
    connect(m_comboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &DebuggerKitConfigWidget::currentDebuggerChanged);

    m_manageButton = new QPushButton(KitConfigWidget::msgManage());
    m_manageButton->setContentsMargins(0, 0, 0, 0);
    connect(m_manageButton, &QAbstractButton::clicked,
            this, &DebuggerKitConfigWidget::manageDebuggers);

    DebuggerItemManager *manager = DebuggerItemManager::instance();
    connect(manager, &DebuggerItemManager::debuggerAdded,
            this, &DebuggerKitConfigWidget::onDebuggerAdded);
    connect(manager, &DebuggerItemManager::debuggerUpdated,
            this, &DebuggerKitConfigWidget::onDebuggerUpdated);
    connect(manager, &DebuggerItemManager::debuggerRemoved,
            this, &DebuggerKitConfigWidget::onDebuggerRemoved);
}

DebuggerKitConfigWidget::~DebuggerKitConfigWidget()
{
    delete m_comboBox;
    delete m_manageButton;
}

QString DebuggerKitConfigWidget::toolTip() const
{
    return tr("The debugger to use for this kit.");
}

QString DebuggerKitConfigWidget::displayName() const
{
    return tr("Debugger:");
}

void DebuggerKitConfigWidget::makeReadOnly()
{
    m_manageButton->setEnabled(false);
    m_comboBox->setEnabled(false);
}

void DebuggerKitConfigWidget::refresh()
{
    const DebuggerItem *item = DebuggerKitInformation::debugger(m_kit);
    updateComboBox(item ? item->id() : QVariant());
}

QWidget *DebuggerKitConfigWidget::buttonWidget() const
{
    return m_manageButton;
}

QWidget *DebuggerKitConfigWidget::mainWidget() const
{
    return m_comboBox;
}

void DebuggerKitConfigWidget::manageDebuggers()
{
    Core::ICore::showOptionsDialog(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY,
                                   ProjectExplorer::Constants::DEBUGGER_SETTINGS_PAGE_ID,
                                   buttonWidget());
}

void DebuggerKitConfigWidget::currentDebuggerChanged(int)
{
    int currentIndex = m_comboBox->currentIndex();
    QVariant id = m_comboBox->itemData(currentIndex);
    m_kit->setValue(DebuggerKitInformation::id(), id);
}

void DebuggerKitConfigWidget::onDebuggerAdded(const QVariant &id)
{
    const DebuggerItem *item = DebuggerItemManager::findById(id);
    QTC_ASSERT(item, return);
    m_comboBox->addItem(item->displayName(), id);
}

void DebuggerKitConfigWidget::onDebuggerUpdated(const QVariant &id)
{
    const DebuggerItem *item = DebuggerItemManager::findById(id);
    QTC_ASSERT(item, return);
    const int pos = indexOf(id);
    if (pos < 0)
        return;
    m_comboBox->setItemText(pos, item->displayName());
}

void DebuggerKitConfigWidget::onDebuggerRemoved(const QVariant &id)
{
    if (const int pos = indexOf(id)) {
        m_comboBox->removeItem(pos);
        refresh();
    }
}

int DebuggerKitConfigWidget::indexOf(const QVariant &id)
{
    QTC_ASSERT(id.isValid(), return -1);
    for (int i = 0; i < m_comboBox->count(); ++i) {
        if (id == m_comboBox->itemData(i))
            return i;
    }
    return -1;
}

QVariant DebuggerKitConfigWidget::currentId() const
{
    return m_comboBox->itemData(m_comboBox->currentIndex());
}

void DebuggerKitConfigWidget::updateComboBox(const QVariant &id)
{
    for (int i = 0; i < m_comboBox->count(); ++i) {
        if (id == m_comboBox->itemData(i)) {
            m_comboBox->setCurrentIndex(i);
            return;
        }
    }
    m_comboBox->setCurrentIndex(0);
}

} // namespace Internal
} // namespace Debugger
