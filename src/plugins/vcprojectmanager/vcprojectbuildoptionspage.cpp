/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
** Contact: http://www.qt-project.org/legal
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
#include "vcprojectbuildoptionspage.h"

#include "widgets/schemaoptionswidget.h"
#include "msbuildversionmanager.h"
#include "vcschemamanager.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <QDialogButtonBox>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QTableWidget>
#include <QVBoxLayout>

namespace VcProjectManager {
namespace Internal {

VcProjectEditMsBuildDialog::VcProjectEditMsBuildDialog(QWidget *parent) :
    QDialog(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *filePathLayout = new QHBoxLayout;
    m_pathChooser = new QLineEdit;
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_browseButton = new QPushButton(tr("Browse..."));
    filePathLayout->addWidget(m_pathChooser);
    filePathLayout->addWidget(m_browseButton);
    filePathLayout->setStretch(0, 1);
    mainLayout->addLayout(filePathLayout);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()),
            this, SLOT(accept()));
    connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()),
            this, SLOT(reject()));
    connect(m_browseButton, SIGNAL(clicked()), this, SLOT(showBrowseFileDialog()));
}

VcProjectEditMsBuildDialog::~VcProjectEditMsBuildDialog()
{
}

QString VcProjectEditMsBuildDialog::path() const
{
    return m_pathChooser->text();
}

void VcProjectEditMsBuildDialog::setPath(const QString &path)
{
    m_pathChooser->setText(path);
}

void VcProjectEditMsBuildDialog::showBrowseFileDialog()
{
    m_pathChooser->setText(QFileDialog::getOpenFileName(0, tr("Select Ms Build"), QString(), QLatin1String("*.exe")));
}

MsBuildTableItem::MsBuildTableItem()
{
}

MsBuildTableItem::~MsBuildTableItem()
{
}

Core::Id MsBuildTableItem::msBuildID() const
{
    return m_id;
}

void MsBuildTableItem::setMsBuildID(Core::Id id)
{
    m_id = id;
}

VcProjectBuildOptionsWidget::VcProjectBuildOptionsWidget(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *mainlayout = new QVBoxLayout(this);
    QHBoxLayout *msBuildMainLayout = new QHBoxLayout;

    QVBoxLayout *buttonLayout = new QVBoxLayout;
    m_addMsBuildButton = new QPushButton(tr("Add..."));
    m_editBuildButton = new QPushButton(tr("Edit..."));
    m_editBuildButton->setEnabled(false);
    m_deleteBuildButton = new QPushButton(tr("Delete"));
    m_deleteBuildButton->setEnabled(false);
    buttonLayout->addWidget(m_addMsBuildButton);
    buttonLayout->addWidget(m_editBuildButton);
    buttonLayout->addWidget(m_deleteBuildButton);
    buttonLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));

    QVBoxLayout *tableLayout = new QVBoxLayout;
    m_buildTableWidget = new QTableWidget();
    m_buildTableWidget->setColumnCount(2);
    QStringList horizontalHeaderLabels;
    horizontalHeaderLabels << tr("Executable") << tr("Version");
    m_buildTableWidget->setHorizontalHeaderLabels(horizontalHeaderLabels);
    m_buildTableWidget->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    m_buildTableWidget->horizontalHeader()->setClickable(false);
    m_buildTableWidget->horizontalHeader()->setMovable(false);
    m_buildTableWidget->horizontalHeader()->setSelectionMode(QAbstractItemView::NoSelection);
    m_buildTableWidget->verticalHeader()->hide();
    m_buildTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableLayout->addWidget(m_buildTableWidget);

    tableLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));

    msBuildMainLayout->addLayout(tableLayout);
    msBuildMainLayout->addLayout(buttonLayout);

    mainlayout->addLayout(msBuildMainLayout);

    m_schemaOptionsWidget = new SchemaOptionsWidget;
    mainlayout->addWidget(m_schemaOptionsWidget);

    connect(m_addMsBuildButton, SIGNAL(clicked()), this, SIGNAL(addNewButtonClicked()));
    connect(m_editBuildButton, SIGNAL(clicked()), this, SIGNAL(editButtonClicked()));
    connect(m_deleteBuildButton, SIGNAL(clicked()), this, SIGNAL(deleteButtonClicked()));
    connect(m_buildTableWidget, SIGNAL(cellClicked(int, int)), this, SLOT(onTableRowIndexChange(int)));

    MsBuildVersionManager *msBVM = MsBuildVersionManager::instance();
    QList<MsBuildInformation *> msBuildInfos = msBVM->msBuildInformations();
    foreach (MsBuildInformation *msBuildInfo, msBuildInfos)
        insertMsBuildIntoTable(msBuildInfo);

    connect(msBVM, SIGNAL(msBuildAdded(Core::Id)), this, SLOT(onMsBuildAdded(Core::Id)));
    connect(msBVM, SIGNAL(msBuildRemoved(Core::Id)), this, SLOT(onMsBuildRemoved(Core::Id)));
    connect(msBVM, SIGNAL(msBuildReplaced(Core::Id,Core::Id)), this, SLOT(onMsBuildReplaced(Core::Id,Core::Id)));
}

VcProjectBuildOptionsWidget::~VcProjectBuildOptionsWidget()
{
}

Core::Id VcProjectBuildOptionsWidget::currentSelectedBuildId() const
{
    QModelIndex currentIndex = m_buildTableWidget->selectionModel()->currentIndex();

    if (currentIndex.isValid()) {
        MsBuildTableItem *item = static_cast<MsBuildTableItem *>(m_buildTableWidget->item(currentIndex.row(), 0));
        return item->msBuildID(); // select ms build id
    }

    return Core::Id();
}

bool VcProjectBuildOptionsWidget::exists(const QString &exePath)
{
    for (int i = 0; i < m_buildTableWidget->rowCount(); ++i) {
        QTableWidgetItem *item = m_buildTableWidget->item(i, 0);

        if (item->text() == exePath)
            return true;
    }

    return false;
}

bool VcProjectBuildOptionsWidget::hasAnyBuilds() const
{
    return m_buildTableWidget->rowCount() >= 0 ? true : false;
}

void VcProjectBuildOptionsWidget::insertMSBuild(MsBuildInformation *msBuild)
{
    if (msBuild) {
        insertMsBuildIntoTable(msBuild);
        m_newMsBuilds.append(msBuild);
    }
}

void VcProjectBuildOptionsWidget::removeMsBuild(Core::Id msBuildId)
{
    for (int i = 0; i < m_buildTableWidget->rowCount(); ++i) {
        MsBuildTableItem *item = static_cast<MsBuildTableItem *>(m_buildTableWidget->item(i, 0));

        if (item->msBuildID() == msBuildId) {
            m_buildTableWidget->removeRow(i);
            break;
        }
    }

    foreach (MsBuildInformation *info, m_newMsBuilds) {
        if (info->getId() == msBuildId) {
            m_newMsBuilds.removeAll(info);
            return;
        }
    }

    m_removedMsBuilds.append(msBuildId);
}

void VcProjectBuildOptionsWidget::replaceMsBuild(Core::Id msBuildId, MsBuildInformation *newMsBuild)
{
    // update data in table
    for (int i = 0; i < m_buildTableWidget->rowCount(); ++i) {
        MsBuildTableItem *item = static_cast<MsBuildTableItem *>(m_buildTableWidget->item(i, 0));

        if (item->msBuildID() == msBuildId) {
            item->setText(newMsBuild->m_executable);
            item->setMsBuildID(newMsBuild->getId());

            // update version column item
            QTableWidgetItem *item2 = m_buildTableWidget->item(i, 1);
            item2->setText(newMsBuild->m_versionString);
            break;
        }
    }

    MsBuildVersionManager *msBVM = MsBuildVersionManager::instance();

    if (msBVM->msBuildInformation(msBuildId)) {
        m_removedMsBuilds.append(msBuildId);
        m_newMsBuilds.append(newMsBuild);
    }

    else {
        m_newMsBuilds.append(newMsBuild);
        foreach (MsBuildInformation *info, m_newMsBuilds) {
            if (info->getId() == msBuildId) {
                m_newMsBuilds.removeAll(info);
                break;
            }
        }
    }
}

void VcProjectBuildOptionsWidget::saveSettings() const
{
    MsBuildVersionManager *msBVM = MsBuildVersionManager::instance();

    disconnect(msBVM, SIGNAL(msBuildAdded(Core::Id)), this, SLOT(onMsBuildAdded(Core::Id)));
    disconnect(msBVM, SIGNAL(msBuildRemoved(Core::Id)), this, SLOT(onMsBuildRemoved(Core::Id)));
    disconnect(msBVM, SIGNAL(msBuildReplaced(Core::Id,Core::Id)), this, SLOT(onMsBuildReplaced(Core::Id,Core::Id)));

    if (msBVM) {
        foreach (const Core::Id &id, m_removedMsBuilds)
            msBVM->removeMsBuildInformation(id);

        foreach (MsBuildInformation *info, m_newMsBuilds)
            msBVM->addMsBuildInformation(info);

        msBVM->saveSettings();
    }

    connect(msBVM, SIGNAL(msBuildAdded(Core::Id)), this, SLOT(onMsBuildAdded(Core::Id)));
    connect(msBVM, SIGNAL(msBuildRemoved(Core::Id)), this, SLOT(onMsBuildRemoved(Core::Id)));
    connect(msBVM, SIGNAL(msBuildReplaced(Core::Id,Core::Id)), this, SLOT(onMsBuildReplaced(Core::Id,Core::Id)));

    m_schemaOptionsWidget->saveSettings();
}

SchemaOptionsWidget *VcProjectBuildOptionsWidget::schemaOptionsWidget() const
{
    return m_schemaOptionsWidget;
}

void VcProjectBuildOptionsWidget::onMsBuildAdded(Core::Id msBuildId)
{
    MsBuildVersionManager *msBVM = MsBuildVersionManager::instance();
    MsBuildInformation *msBuild = msBVM->msBuildInformation(msBuildId);

    if (msBuild)
        insertMsBuildIntoTable(msBuild);
}

void VcProjectBuildOptionsWidget::onMsBuildReplaced(Core::Id oldMsBuildId, Core::Id newMsBuildId)
{
    MsBuildVersionManager *msBVM = MsBuildVersionManager::instance();
    MsBuildInformation *newMsBuild = msBVM->msBuildInformation(newMsBuildId);

    // update data in table
    for (int i = 0; i < m_buildTableWidget->rowCount(); ++i) {
        MsBuildTableItem *item = static_cast<MsBuildTableItem *>(m_buildTableWidget->item(i, 0));

        if (item->msBuildID() == oldMsBuildId) {
            item->setText(newMsBuild->m_executable);
            item->setMsBuildID(newMsBuild->getId());

            // update version column item
            QTableWidgetItem *item2 = m_buildTableWidget->item(i, 1);
            item2->setText(newMsBuild->m_versionString);
            break;
        }
    }
}

void VcProjectBuildOptionsWidget::onMsBuildRemoved(Core::Id msBuildId)
{
    for (int i = 0; i < m_buildTableWidget->rowCount(); ++i) {
        MsBuildTableItem *item = static_cast<MsBuildTableItem *>(m_buildTableWidget->item(i, 0));

        if (item->msBuildID() == msBuildId) {
            m_buildTableWidget->removeRow(i);
            return;
        }
    }
}

void VcProjectBuildOptionsWidget::onTableRowIndexChange(int index)
{
    if (0 <= index && index < m_buildTableWidget->rowCount()) {
        m_editBuildButton->setEnabled(true);
        m_deleteBuildButton->setEnabled(true);
    }

    else {
        m_editBuildButton->setEnabled(false);
        m_deleteBuildButton->setEnabled(false);
    }

    emit currentBuildSelectionChanged(index);
}

void VcProjectBuildOptionsWidget::insertMsBuildIntoTable(MsBuildInformation *msBuild)
{
    if (msBuild) {
        MsBuildTableItem *exeTableItem = new MsBuildTableItem();
        exeTableItem->setFlags(exeTableItem->flags() ^ Qt::ItemIsEditable);
        exeTableItem->setText(msBuild->m_executable);
        exeTableItem->setMsBuildID(msBuild->getId());

        QTableWidgetItem *versionTableItem = new QTableWidgetItem();
        versionTableItem->setFlags(versionTableItem->flags() ^ Qt::ItemIsEditable);
        versionTableItem->setText(msBuild->m_versionString);

        m_buildTableWidget->insertRow(m_buildTableWidget->rowCount());
        m_buildTableWidget->setItem(m_buildTableWidget->rowCount() - 1, 0, exeTableItem);
        m_buildTableWidget->setItem(m_buildTableWidget->rowCount() - 1, 1, versionTableItem);

        if (!m_buildTableWidget->selectionModel()->currentIndex().isValid())
            m_buildTableWidget->selectRow(0);

        if (!m_editBuildButton->isEnabled())
            m_editBuildButton->setEnabled(true);

        if (!m_deleteBuildButton->isEnabled())
            m_deleteBuildButton->setEnabled(true);
    }
}

VcProjectBuildOptionsPage::VcProjectBuildOptionsPage() :
    m_optionsWidget(0)
{
    setId(Core::Id("VcProject.MSBuild"));
    setDisplayName(tr("MS Build"));
    setCategory(Core::Id(VcProjectManager::Constants::VC_PROJECT_SETTINGS_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
                                                   VcProjectManager::Constants::VC_PROJECT_SETTINGS_TR_CATEGORY));

    // TODO(Radovan): create and set proper icon
    setCategoryIcon(QLatin1String(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY_ICON));
}

VcProjectBuildOptionsPage::~VcProjectBuildOptionsPage()
{
}

QWidget *VcProjectBuildOptionsPage::createPage(QWidget *parent)
{
    m_optionsWidget = new VcProjectBuildOptionsWidget(parent);

    connect(m_optionsWidget, SIGNAL(addNewButtonClicked()), this, SLOT(addNewMsBuild()));
    connect(m_optionsWidget, SIGNAL(editButtonClicked()), this, SLOT(editMsBuild()));
    connect(m_optionsWidget, SIGNAL(deleteButtonClicked()), this, SLOT(deleteMsBuild()));
    return m_optionsWidget;
}

void VcProjectBuildOptionsPage::apply()
{
    if (!m_optionsWidget || (m_optionsWidget && !m_optionsWidget->hasAnyBuilds()))
        return;

    saveSettings();
}

void VcProjectBuildOptionsPage::finish()
{
}

void VcProjectBuildOptionsPage::saveSettings()
{
    if (m_optionsWidget)
        m_optionsWidget->saveSettings();
}

void VcProjectBuildOptionsPage::startVersionCheck()
{
    if (m_validator.m_executable.isEmpty()) {
        return;
    }

    m_validator.m_process = new QProcess();
    connect(m_validator.m_process, SIGNAL(finished(int)),
            this, SLOT(versionCheckFinished()));

    m_validator.m_process->start(m_validator.m_executable, QStringList(QLatin1String("/version")));
    m_validator.m_process->waitForStarted();
}

void VcProjectBuildOptionsPage::addNewMsBuild()
{
    QString newMsBuild = QFileDialog::getOpenFileName(0, tr("Select Ms Build"), QString(), QLatin1String("*.exe"));

    if (m_optionsWidget && !newMsBuild.isEmpty()) {
        if (m_optionsWidget->exists(newMsBuild)) {
            QMessageBox::information(0, tr("Ms Build already present"), tr("Selected MSBuild.exe is already present in the list."));
            return;
        }

        m_validator.m_executable = newMsBuild;
        m_validator.m_requestType = VcProjectValidator::ValidationRequest_Add;
        startVersionCheck();
    }
}

void VcProjectBuildOptionsPage::editMsBuild()
{
    if (m_optionsWidget) {
        MsBuildVersionManager *msBVM = MsBuildVersionManager::instance();
        MsBuildInformation *currentSelectedMsBuild = msBVM->msBuildInformation(m_optionsWidget->currentSelectedBuildId());

        if (currentSelectedMsBuild) {
            m_validator.m_originalMsInfoID = currentSelectedMsBuild->getId();

            VcProjectEditMsBuildDialog editDialog;
            editDialog.setPath(currentSelectedMsBuild->m_executable);

            if (editDialog.exec() == QDialog::Accepted) {
                if (editDialog.path() == currentSelectedMsBuild->m_executable)
                    return;

                m_validator.m_requestType = VcProjectValidator::ValidationRequest_Edit;
                m_validator.m_executable = editDialog.path();
                startVersionCheck();
            }
        }
    }
}

void VcProjectBuildOptionsPage::deleteMsBuild()
{
    m_optionsWidget->removeMsBuild(m_optionsWidget->currentSelectedBuildId());
}

void VcProjectBuildOptionsPage::versionCheckFinished()
{
    if (m_validator.m_process) {
        QString response = QVariant(m_validator.m_process->readAll()).toString();
        QStringList splitData = response.split(QLatin1Char('\n'));

        if (m_validator.m_requestType == VcProjectValidator::ValidationRequest_Add) {
            MsBuildInformation *newMsBuild = MsBuildVersionManager::createMsBuildInfo(m_validator.m_executable, splitData.last());
            m_optionsWidget->insertMSBuild(newMsBuild);
        }

        else if (m_validator.m_requestType == VcProjectValidator::ValidationRequest_Edit) {
            MsBuildInformation *newMsBuildInfo = MsBuildVersionManager::createMsBuildInfo(m_validator.m_executable, splitData.last());

            // update table data
            if (m_optionsWidget)
                m_optionsWidget->replaceMsBuild(m_validator.m_originalMsInfoID, newMsBuildInfo);
        }

        m_validator.m_process->deleteLater();
        m_validator.m_process = 0;
    }
}

} // namespace Internal
} // namespace VcProjectManager
