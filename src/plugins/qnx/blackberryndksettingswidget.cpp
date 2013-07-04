/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberryndksettingswidget.h"
#include "ui_blackberryndksettingswidget.h"
#include "qnxutils.h"
#include "blackberryutils.h"
#include "blackberrysetupwizard.h"

#include "blackberryconfigurationmanager.h"
#include "blackberryconfiguration.h"

#include <utils/pathchooser.h>

#include <coreplugin/icore.h>

#include <QMessageBox>
#include <QFileDialog>

#include <QStandardItemModel>
#include <QTreeWidgetItem>

namespace Qnx {
namespace Internal {

BlackBerryNDKSettingsWidget::BlackBerryNDKSettingsWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui_BlackBerryNDKSettingsWidget),
    m_autoDetectedNdks(0),
    m_manualNdks(0)
{
    m_bbConfigManager = &BlackBerryConfigurationManager::instance();
    m_ui->setupUi(this);

    m_ui->removeNdkButton->setEnabled(false);

    initInfoTable();
    initNdkList();

    connect(m_ui->wizardButton, SIGNAL(clicked()), this, SLOT(launchBlackBerrySetupWizard()));
    connect(m_ui->addNdkButton, SIGNAL(clicked()), this, SLOT(addNdk()));
    connect(m_ui->removeNdkButton, SIGNAL(clicked()), this, SLOT(removeNdk()));
    connect(m_ui->ndksTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(updateInfoTable(QTreeWidgetItem*)));
}

void BlackBerryNDKSettingsWidget::setWizardMessageVisible(bool visible)
{
    m_ui->wizardLabel->setVisible(visible);
    m_ui->wizardButton->setVisible(visible);
}

bool BlackBerryNDKSettingsWidget::hasActiveNdk() const
{
    return !m_bbConfigManager->configurations().isEmpty();
}

void BlackBerryNDKSettingsWidget::launchBlackBerrySetupWizard() const
{
    const bool alreadyConfigured = BlackBerryUtils::hasRegisteredKeys();

    if (alreadyConfigured) {
        QMessageBox::information(0, tr("Qt Creator"),
            tr("It appears that your BlackBerry environment has already been configured."));
            return;
    }

    BlackBerrySetupWizard wizard;
    wizard.exec();
}

void BlackBerryNDKSettingsWidget::updateInfoTable(QTreeWidgetItem* currentNdk)
{
    QString ndkPath = currentNdk->text(1);
    if (ndkPath.isEmpty()) {
        m_ui->removeNdkButton->setEnabled(false);
        return;
    }

    QMultiMap<QString, QString> env = QnxUtils::parseEnvironmentFile(QnxUtils::envFilePath(ndkPath));
    if (env.isEmpty()) {
        clearInfoTable();
        return;
    }

    m_infoModel->clear();
    m_infoModel->setHorizontalHeaderItem(0, new QStandardItem(QString(QLatin1String("Variable"))));
    m_infoModel->setHorizontalHeaderItem(1, new QStandardItem(QString(QLatin1String("Value"))));

    m_ui->ndkInfosTableView->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
    m_ui->ndkInfosTableView->horizontalHeader()->setStretchLastSection(true);

    QMultiMap<QString, QString>::const_iterator it;
    QMultiMap<QString, QString>::const_iterator end(env.constEnd());
    for (it = env.constBegin(); it != end; ++it) {
        const QString key = it.key();
        const QString value = it.value();
        QList <QStandardItem*> row;
        row << new QStandardItem(key) << new QStandardItem(value);
        m_infoModel->appendRow(row);
    }

    BlackBerryConfiguration *config = m_bbConfigManager->configurationFromNdkPath(ndkPath);
    if (!config)
        return;

    QString qmake4Path = config->qmake4BinaryFile().toString();
    QString qmake5Path = config->qmake5BinaryFile().toString();

    if (!qmake4Path.isEmpty())
        m_infoModel->appendRow(QList<QStandardItem*>() << new QStandardItem(QString(QLatin1String("QMAKE 4"))) << new QStandardItem(qmake4Path));

    if (!qmake5Path.isEmpty())
        m_infoModel->appendRow(QList<QStandardItem*>() << new QStandardItem(QString(QLatin1String("QMAKE 5"))) << new QStandardItem(qmake5Path));

    m_infoModel->appendRow(QList<QStandardItem*>() << new QStandardItem(QString(QLatin1String("COMPILER"))) << new QStandardItem(config->gccCompiler().toString()));

    m_ui->removeNdkButton->setEnabled(!config->isAutoDetected());
}

void BlackBerryNDKSettingsWidget::updateNdkList()
{
    foreach (BlackBerryConfiguration *config, m_bbConfigManager->configurations()) {
        QTreeWidgetItem *parent = config->isAutoDetected() ? m_autoDetectedNdks : m_manualNdks;
        QTreeWidgetItem *item = new QTreeWidgetItem(parent);
        item->setText(0, config->displayName());
        item->setText(1, config->ndkPath()); // TODO: should be target name for NDKs >= v10.2
    }

    if (m_autoDetectedNdks->child(0)) {
        m_autoDetectedNdks->child(0)->setSelected(true);
        updateInfoTable(m_autoDetectedNdks->child(0));
    }
}

void BlackBerryNDKSettingsWidget::clearInfoTable()
{
    m_infoModel->clear();
}

void BlackBerryNDKSettingsWidget::addNdk()
{
    QString selectedPath = QFileDialog::getExistingDirectory(0, tr("Select the NDK path"),
                                                             QString(),
                                                             QFileDialog::ShowDirsOnly);
    if (selectedPath.isEmpty())
        return;

    BlackBerryConfiguration *config = m_bbConfigManager->configurationFromNdkPath(selectedPath);
    if (!config) {
        config = new BlackBerryConfiguration(selectedPath, false);
        if (!m_bbConfigManager->addConfiguration(config)) {
            delete config;
            return;
        }

        QTreeWidgetItem *item = new QTreeWidgetItem(m_manualNdks);
        item->setText(0, selectedPath.split(QDir::separator()).last());
        item->setText(1, selectedPath);
        updateInfoTable(item);
    }
}

void BlackBerryNDKSettingsWidget::removeNdk()
{
    QString ndkPath = m_ui->ndksTreeWidget->currentItem()->text(1);
    QMessageBox::StandardButton button =
            QMessageBox::question(Core::ICore::mainWindow(),
                                  tr("Clean BlackBerry 10 Configuration"),
                                  tr("Are you sure you want to remove:\n %1?").arg(ndkPath),
                                  QMessageBox::Yes | QMessageBox::No);

    if (button == QMessageBox::Yes) {
        BlackBerryConfiguration *config = m_bbConfigManager->configurationFromNdkPath(ndkPath);
        if (config)
            m_bbConfigManager->removeConfiguration(config);
            m_manualNdks->removeChild(m_ui->ndksTreeWidget->currentItem());
    }

}

void BlackBerryNDKSettingsWidget::initInfoTable()
{
    m_infoModel = new QStandardItemModel(this);

    m_ui->ndkInfosTableView->setModel(m_infoModel);
    m_ui->ndkInfosTableView->verticalHeader()->hide();
    m_ui->ndkInfosTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void BlackBerryNDKSettingsWidget::initNdkList()
{
    m_ui->ndksTreeWidget->header()->setResizeMode(QHeaderView::Stretch);
    m_ui->ndksTreeWidget->header()->setStretchLastSection(false);
    m_ui->ndksTreeWidget->setHeaderItem(new QTreeWidgetItem(QStringList() << tr("NDK") << tr("Path")));
    m_ui->ndksTreeWidget->setTextElideMode(Qt::ElideNone);
    m_ui->ndksTreeWidget->setColumnCount(2);
    m_autoDetectedNdks = new QTreeWidgetItem(m_ui->ndksTreeWidget);
    m_autoDetectedNdks->setText(0, tr("Auto-Detected"));
    m_autoDetectedNdks->setFirstColumnSpanned(true);
    m_autoDetectedNdks->setFlags(Qt::ItemIsEnabled);
    m_manualNdks = new QTreeWidgetItem(m_ui->ndksTreeWidget);
    m_manualNdks->setText(0, tr("Manual"));
    m_manualNdks->setFirstColumnSpanned(true);
    m_manualNdks->setFlags(Qt::ItemIsEnabled);

    m_ui->ndksTreeWidget->expandAll();

    updateNdkList();
}

} // namespace Internal
} // namespace Qnx

