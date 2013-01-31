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

#include <utils/pathchooser.h>

#include <coreplugin/icore.h>

#include <QMessageBox>

namespace Qnx {
namespace Internal {

BlackBerryNDKSettingsWidget::BlackBerryNDKSettingsWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui_BlackBerryNDKSettingsWidget)
{
    m_bbConfig = &BlackBerryConfiguration::instance();
    m_ui->setupUi(this);
    m_ui->sdkPath->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_ui->sdkPath->setPath(m_bbConfig->ndkPath());

    initInfoTable();

    connect(m_ui->sdkPath, SIGNAL(changed(QString)), this, SLOT(checkSdkPath()));
    connect(m_ui->removeButton, SIGNAL(clicked()), this, SLOT(cleanConfiguration()));
    connect(m_bbConfig, SIGNAL(updated()), this, SLOT(updateInfoTable()));
}

void BlackBerryNDKSettingsWidget::checkSdkPath()
{
    if (!m_ui->sdkPath->path().isEmpty() &&
            QnxUtils::isValidNdkPath(m_ui->sdkPath->path()))
        m_bbConfig->setupConfiguration(m_ui->sdkPath->path());
}

void BlackBerryNDKSettingsWidget::updateInfoTable()
{
    QMultiMap<QString, QString> env = m_bbConfig->qnxEnv();

    if (env.isEmpty()) {
        // clear
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

    m_infoModel->appendRow( QList<QStandardItem*>() << new QStandardItem(QString(QLatin1String("QMAKE"))) << new QStandardItem(m_bbConfig->qmakePath().toString()));
    m_infoModel->appendRow( QList<QStandardItem*>() << new QStandardItem(QString(QLatin1String("COMPILER"))) << new QStandardItem(m_bbConfig->gccPath().toString()));

    m_ui->removeButton->setEnabled(true);
}

void BlackBerryNDKSettingsWidget::clearInfoTable()
{
    m_infoModel->clear();
    m_ui->sdkPath->setPath(QString());
    m_ui->removeButton->setEnabled(false);
}

void BlackBerryNDKSettingsWidget::cleanConfiguration()
{
    QMessageBox::StandardButton button =
            QMessageBox::question(Core::ICore::mainWindow(),
                                  tr("Clean BlackBerry 10 Configuration"),
                                  tr("Are you sure you want to remove the current BlackBerry configuration?"),
                                  QMessageBox::Yes | QMessageBox::No);

    if (button == QMessageBox::Yes)
        m_bbConfig->cleanConfiguration();
}

void BlackBerryNDKSettingsWidget::initInfoTable()
{
    m_infoModel = new QStandardItemModel(this);

    m_ui->ndkInfosTableView->setModel(m_infoModel);
    m_ui->ndkInfosTableView->verticalHeader()->hide();
    m_ui->ndkInfosTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    updateInfoTable();
}

} // namespace Internal
} // namespace Qnx

