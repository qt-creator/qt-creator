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

#include "pluginerroroverview.h"
#include "ui_pluginerroroverview.h"
#include "pluginspec.h"
#include "pluginmanager.h"

Q_DECLARE_METATYPE(ExtensionSystem::PluginSpec*)

namespace ExtensionSystem {

PluginErrorOverview::PluginErrorOverview(QWidget *parent) :
    QDialog(parent),
    m_ui(new Internal::Ui::PluginErrorOverview)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_ui->setupUi(this);
    m_ui->buttonBox->addButton(tr("Continue"), QDialogButtonBox::AcceptRole);

    foreach (PluginSpec *spec, PluginManager::plugins()) {
        // only show errors on startup if plugin is enabled.
        if (spec->hasError() && spec->isEffectivelyEnabled()) {
            QListWidgetItem *item = new QListWidgetItem(spec->name());
            item->setData(Qt::UserRole, qVariantFromValue(spec));
            m_ui->pluginList->addItem(item);
        }
    }

    connect(m_ui->pluginList, &QListWidget::currentItemChanged,
            this, &PluginErrorOverview::showDetails);

    if (m_ui->pluginList->count() > 0)
        m_ui->pluginList->setCurrentRow(0);
}

PluginErrorOverview::~PluginErrorOverview()
{
    delete m_ui;
}

void PluginErrorOverview::showDetails(QListWidgetItem *item)
{
    if (item) {
        PluginSpec *spec = item->data(Qt::UserRole).value<PluginSpec *>();
        m_ui->pluginError->setText(spec->errorString());
    } else {
        m_ui->pluginError->clear();
    }
}

} // namespace ExtensionSystem
