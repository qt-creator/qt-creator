/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidpackagecreationwidget.h"
#include "androidpackagecreationstep.h"
#include "androidconfigurations.h"
#include "androidcreatekeystorecertificate.h"
#include "androidmanager.h"
#include "ui_androidpackagecreationwidget.h"

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qmakestep.h>

#include <QTimer>

#include <QFileDialog>
#include <QFileSystemWatcher>
#include <QMessageBox>

namespace Android {
namespace Internal {

using namespace Qt4ProjectManager;

const QLatin1String packageNameRegExp("^([a-z_]{1}[a-z0-9_]+(\\.[a-zA-Z_]{1}[a-zA-Z0-9_]*)*)$");

bool checkPackageName(const QString &packageName)
{
    return QRegExp(packageNameRegExp).exactMatch(packageName);
}

///////////////////////////// CheckModel /////////////////////////////

CheckModel::CheckModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void CheckModel::setAvailableItems(const QStringList &items)
{
    beginResetModel();
    m_availableItems = items;
    endResetModel();
}

void CheckModel::setCheckedItems(const QStringList &items)
{
    m_checkedItems = items;
    if (rowCount())
        emit dataChanged(index(0), index(rowCount()-1));
}

const QStringList &CheckModel::checkedItems()
{
    return m_checkedItems;
}

QVariant CheckModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    switch (role) {
    case Qt::CheckStateRole:
        return m_checkedItems.contains(m_availableItems.at(index.row())) ? Qt::Checked : Qt::Unchecked;
    case Qt::DisplayRole:
        return m_availableItems.at(index.row());
    }
    return QVariant();
}

void CheckModel::moveUp(int index)
{
    beginMoveRows(QModelIndex(), index, index, QModelIndex(), index - 1);
    const QString &item1 = m_availableItems[index];
    const QString &item2 = m_availableItems[index - 1];
    m_availableItems.swap(index, index - 1);
    index = m_checkedItems.indexOf(item1);
    int index2 = m_checkedItems.indexOf(item2);
    if (index > -1 && index2 > -1)
        m_checkedItems.swap(index, index2);
    endMoveRows();
}

bool CheckModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::CheckStateRole || !index.isValid())
        return false;
    if (value.toInt() == Qt::Checked)
        m_checkedItems.append(m_availableItems.at(index.row()));
    else
        m_checkedItems.removeAll(m_availableItems.at(index.row()));
    emit dataChanged(index, index);
    return true;
}

int CheckModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_availableItems.count();
}

Qt::ItemFlags CheckModel::flags(const QModelIndex &/*index*/) const
{
    return Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled;
}


///////////////////////////// PermissionsModel /////////////////////////////

PermissionsModel::PermissionsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void PermissionsModel::setPermissions(const QStringList &permissions)
{
    beginResetModel();
    m_permissions = permissions;
    endResetModel();
}

const QStringList &PermissionsModel::permissions()
{
    return m_permissions;
}

QModelIndex PermissionsModel::addPermission(const QString &permission)
{
    const int idx = m_permissions.count();
    beginInsertRows(QModelIndex(), idx, idx);
    m_permissions.push_back(permission);
    endInsertRows();
    return index(idx);
}

bool PermissionsModel::updatePermission(QModelIndex index, const QString &permission)
{
    if (!index.isValid())
        return false;
    if (m_permissions[index.row()] == permission)
        return false;
    m_permissions[index.row()] = permission;
    emit dataChanged(index, index);
    return true;
}

void PermissionsModel::removePermission(int index)
{
    if (index >= m_permissions.size())
        return;
    beginRemoveRows(QModelIndex(), index, index);
    m_permissions.removeAt(index);
    endRemoveRows();
}

QVariant PermissionsModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !index.isValid())
        return QVariant();
    return m_permissions[index.row()];
}

int PermissionsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_permissions.count();
}


///////////////////////////// AndroidPackageCreationWidget /////////////////////////////

AndroidPackageCreationWidget::AndroidPackageCreationWidget(AndroidPackageCreationStep *step)
    : ProjectExplorer::BuildStepConfigWidget(),
      m_step(step),
      m_ui(new Ui::AndroidPackageCreationWidget),
      m_fileSystemWatcher(new QFileSystemWatcher(this))
{
    m_qtLibsModel = new CheckModel(this);
    m_prebundledLibs = new CheckModel(this);
    m_permissionsModel = new PermissionsModel(this);

    m_ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QTimer::singleShot(50, this, SLOT(initGui()));
    connect(m_step, SIGNAL(updateRequiredLibrariesModels()), SLOT(updateRequiredLibrariesModels()));
}

void AndroidPackageCreationWidget::initGui()
{
    updateAndroidProjectInfo();
    ProjectExplorer::Target *target = m_step->target();

    m_fileSystemWatcher->addPath(AndroidManager::dirPath(target).toString());
    m_fileSystemWatcher->addPath(AndroidManager::manifestPath(target).toString());
    m_fileSystemWatcher->addPath(AndroidManager::srcPath(target).toString());
    connect(m_fileSystemWatcher, SIGNAL(directoryChanged(QString)),
            this, SLOT(updateAndroidProjectInfo()));
    connect(m_fileSystemWatcher, SIGNAL(fileChanged(QString)), this,
            SLOT(updateAndroidProjectInfo()));

    m_ui->packageNameLineEdit->setValidator(new QRegExpValidator(QRegExp(packageNameRegExp), this));
    connect(m_ui->packageNameLineEdit, SIGNAL(editingFinished()), SLOT(setPackageName()));
    connect(m_ui->appNameLineEdit, SIGNAL(editingFinished()), SLOT(setApplicationName()));
    connect(m_ui->versionCode, SIGNAL(editingFinished()), SLOT(setVersionCode()));
    connect(m_ui->versionNameLinedit, SIGNAL(editingFinished()), SLOT(setVersionName()));
    connect(m_ui->targetSDKComboBox, SIGNAL(activated(QString)), SLOT(setTargetSDK(QString)));
    connect(m_ui->permissionsListView, SIGNAL(activated(QModelIndex)), SLOT(permissionActivated(QModelIndex)));
    connect(m_ui->addPermissionButton, SIGNAL(clicked()), SLOT(addPermission()));
    connect(m_ui->removePermissionButton, SIGNAL(clicked()), SLOT(removePermission()));
    connect(m_ui->permissionsComboBox->lineEdit(), SIGNAL(editingFinished()), SLOT(updatePermission()));
    connect(m_ui->savePermissionsButton, SIGNAL(clicked()), SLOT(savePermissionsButton()));
    connect(m_ui->discardPermissionsButton, SIGNAL(clicked()), SLOT(discardPermissionsButton()));
    connect(m_ui->targetComboBox, SIGNAL(activated(QString)), SLOT(setTarget(QString)));
    connect(m_qtLibsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), SLOT(setQtLibs(QModelIndex,QModelIndex)));
    connect(m_prebundledLibs, SIGNAL(dataChanged(QModelIndex,QModelIndex)), SLOT(setPrebundledLibs(QModelIndex,QModelIndex)));
    connect(m_ui->prebundledLibsListView, SIGNAL(activated(QModelIndex)), SLOT(prebundledLibSelected(QModelIndex)));
    connect(m_ui->prebundledLibsListView, SIGNAL(clicked(QModelIndex)), SLOT(prebundledLibSelected(QModelIndex)));
    connect(m_ui->upPushButton, SIGNAL(clicked()), SLOT(prebundledLibMoveUp()));
    connect(m_ui->downPushButton, SIGNAL(clicked()), SLOT(prebundledLibMoveDown()));
    connect(m_ui->readInfoPushButton, SIGNAL(clicked()), SLOT(readElfInfo()));

    connect(m_ui->hIconButton, SIGNAL(clicked()), SLOT(setHDPIIcon()));
    connect(m_ui->mIconButton, SIGNAL(clicked()), SLOT(setMDPIIcon()));
    connect(m_ui->lIconButton, SIGNAL(clicked()), SLOT(setLDPIIcon()));

    m_ui->qtLibsListView->setModel(m_qtLibsModel);
    m_ui->prebundledLibsListView->setModel(m_prebundledLibs);
    m_ui->permissionsListView->setModel(m_permissionsModel);
    m_ui->KeystoreLocationLineEdit->setText(m_step->keystorePath().toUserOutput());

    // Make the buildconfiguration emit a evironmentChanged() signal
    // TODO find a better way
    Qt4BuildConfiguration *bc = qobject_cast<Qt4BuildConfiguration *>(m_step->target()->activeBuildConfiguration());
    if (!bc)
        return;
    bool use = bc->useSystemEnvironment();
    bc->setUseSystemEnvironment(!use);
    bc->setUseSystemEnvironment(use);
}

void AndroidPackageCreationWidget::updateAndroidProjectInfo()
{
    ProjectExplorer::Target *target = m_step->target();
    const QString packageName = AndroidManager::packageName(target);
    m_ui->targetSDKComboBox->clear();
    QStringList targets = AndroidConfigurations::instance().sdkTargets();
    m_ui->targetSDKComboBox->addItems(targets);
    m_ui->targetSDKComboBox->setCurrentIndex(targets.indexOf(AndroidManager::targetSDK(target)));
    m_ui->packageNameLineEdit->setText(packageName);
    m_ui->appNameLineEdit->setText(AndroidManager::applicationName(target));
    if (!m_ui->appNameLineEdit->text().length()) {
        QString applicationName = target->project()->displayName();
        AndroidManager::setPackageName(target, packageName + QLatin1Char('.') + applicationName);
        m_ui->packageNameLineEdit->setText(packageName);
        if (!applicationName.isEmpty())
            applicationName[0] = applicationName[0].toUpper();
        m_ui->appNameLineEdit->setText(applicationName);
        AndroidManager::setApplicationName(target, applicationName);
    }
    m_ui->versionCode->setValue(AndroidManager::versionCode(target));
    m_ui->versionNameLinedit->setText(AndroidManager::versionName(target));

    m_qtLibsModel->setAvailableItems(AndroidManager::availableQtLibs(target));
    m_qtLibsModel->setCheckedItems(AndroidManager::qtLibs(target));
    m_prebundledLibs->setAvailableItems(AndroidManager::availablePrebundledLibs(target));
    m_prebundledLibs->setCheckedItems(AndroidManager::prebundledLibs(target));

    m_permissionsModel->setPermissions(AndroidManager::permissions(target));
    m_ui->removePermissionButton->setEnabled(m_permissionsModel->permissions().size());

    targets = AndroidManager::availableTargetApplications(target);
    m_ui->targetComboBox->clear();
    m_ui->targetComboBox->addItems(targets);
    m_ui->targetComboBox->setCurrentIndex(targets.indexOf(AndroidManager::targetApplication(target)));
    if (m_ui->targetComboBox->currentIndex() == -1 && targets.count()) {
        m_ui->targetComboBox->setCurrentIndex(0);
        AndroidManager::setTargetApplication(target, m_ui->targetComboBox->currentText());
    }
    m_ui->hIconButton->setIcon(AndroidManager::highDpiIcon(target));
    m_ui->mIconButton->setIcon(AndroidManager::mediumDpiIcon(target));
    m_ui->lIconButton->setIcon(AndroidManager::lowDpiIcon(target));
}

void AndroidPackageCreationWidget::setPackageName()
{
    const QString packageName= m_ui->packageNameLineEdit->text();
    if (!checkPackageName(packageName)) {
        QMessageBox::critical(this, tr("Invalid Package Name") ,
                              tr("The package name '%1' is not valid.\n"
                                 "Please choose a valid package name for your application (e.g. \"org.example.myapplication\").")
                              .arg(packageName));
        m_ui->packageNameLineEdit->selectAll();
        m_ui->packageNameLineEdit->setFocus();
        return;
    }
    AndroidManager::setPackageName(m_step->target(), packageName);
}

void AndroidPackageCreationWidget::setApplicationName()
{
    AndroidManager::setApplicationName(m_step->target(), m_ui->appNameLineEdit->text());
}

void AndroidPackageCreationWidget::setTargetSDK(const QString &sdk)
{
    AndroidManager::setTargetSDK(m_step->target(), sdk);
    Qt4BuildConfiguration *bc = qobject_cast<Qt4BuildConfiguration *>(m_step->target()->activeBuildConfiguration());
    if (!bc)
        return;
    QMakeStep *qs = bc->qmakeStep();
    if (!qs)
        return;

    qs->setForced(true);

    ProjectExplorer::BuildManager *bm = ProjectExplorer::ProjectExplorerPlugin::instance()->buildManager();
    bm->buildList(bc->stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN)),
                  ProjectExplorer::ProjectExplorerPlugin::displayNameForStepId(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN)));
    bm->appendStep(qs, ProjectExplorer::ProjectExplorerPlugin::displayNameForStepId(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN)));
    bc->setSubNodeBuild(0);
    // Make the buildconfiguration emit a evironmentChanged() signal
    // TODO find a better way
    bool use = bc->useSystemEnvironment();
    bc->setUseSystemEnvironment(!use);
    bc->setUseSystemEnvironment(use);
}

void AndroidPackageCreationWidget::setVersionCode()
{
    AndroidManager::setVersionCode(m_step->target(), m_ui->versionCode->value());
}

void AndroidPackageCreationWidget::setVersionName()
{
    AndroidManager::setVersionName(m_step->target(), m_ui->versionNameLinedit->text());
}

void AndroidPackageCreationWidget::setTarget(const QString &target)
{
    AndroidManager::setTargetApplication(m_step->target(), target);
}

void AndroidPackageCreationWidget::setQtLibs(QModelIndex, QModelIndex)
{
    AndroidManager::setQtLibs(m_step->target(), m_qtLibsModel->checkedItems());
}

void AndroidPackageCreationWidget::setPrebundledLibs(QModelIndex, QModelIndex)
{
    AndroidManager::setPrebundledLibs(m_step->target(), m_prebundledLibs->checkedItems());
}

void AndroidPackageCreationWidget::prebundledLibSelected(const QModelIndex &index)
{
    m_ui->upPushButton->setEnabled(false);
    m_ui->downPushButton->setEnabled(false);
    if (!index.isValid())
        return;
    if (index.row() > 0)
        m_ui->upPushButton->setEnabled(true);
    if (index.row() < m_prebundledLibs->rowCount(QModelIndex()) - 1)
        m_ui->downPushButton->setEnabled(true);
}

void AndroidPackageCreationWidget::prebundledLibMoveUp()
{
    const QModelIndex &index = m_ui->prebundledLibsListView->currentIndex();
    if (index.isValid())
        m_prebundledLibs->moveUp(index.row());
}

void AndroidPackageCreationWidget::prebundledLibMoveDown()
{
    const QModelIndex &index = m_ui->prebundledLibsListView->currentIndex();
    if (index.isValid())
        m_prebundledLibs->moveUp(index.row() + 1);
}

void AndroidPackageCreationWidget::setHDPIIcon()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Choose High DPI Icon"), QDir::homePath(), tr("PNG images (*.png)"));
    if (!file.length())
        return;
    AndroidManager::setHighDpiIcon(m_step->target(), file);
    m_ui->hIconButton->setIcon(AndroidManager::highDpiIcon(m_step->target()));
}

void AndroidPackageCreationWidget::setMDPIIcon()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Choose Medium DPI Icon"), QDir::homePath(), tr("PNG images (*.png)"));
    if (!file.length())
        return;
    AndroidManager::setMediumDpiIcon(m_step->target(), file);
    m_ui->mIconButton->setIcon(AndroidManager::mediumDpiIcon(m_step->target()));
}

void AndroidPackageCreationWidget::setLDPIIcon()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Choose Low DPI Icon"), QDir::homePath(), tr("PNG images (*.png)"));
    if (!file.length())
        return;
    AndroidManager::setLowDpiIcon(m_step->target(), file);
    m_ui->lIconButton->setIcon(AndroidManager::lowDpiIcon(m_step->target()));
}

void AndroidPackageCreationWidget::permissionActivated(QModelIndex index)
{
    m_ui->permissionsComboBox->setCurrentIndex(
                m_ui->permissionsComboBox->findText(m_permissionsModel->data(index, Qt::DisplayRole).toString()));
    m_ui->permissionsComboBox->lineEdit()->setText(m_permissionsModel->data(index, Qt::DisplayRole).toString());
}

void AndroidPackageCreationWidget::addPermission()
{
    setEnabledSaveDiscardButtons(true);
    m_ui->permissionsListView->setCurrentIndex(m_permissionsModel->addPermission(tr("< Type or choose a permission >")));
    m_ui->permissionsComboBox->lineEdit()->setText(tr("< Type or choose a permission >"));
    m_ui->permissionsComboBox->setFocus();
    m_ui->removePermissionButton->setEnabled(m_permissionsModel->permissions().size());
}

void AndroidPackageCreationWidget::updatePermission()
{
    if (m_permissionsModel->updatePermission(m_ui->permissionsListView->currentIndex(),
                                             m_ui->permissionsComboBox->lineEdit()->text()))
        setEnabledSaveDiscardButtons(true);
}

void AndroidPackageCreationWidget::removePermission()
{
    setEnabledSaveDiscardButtons(true);
    if (m_ui->permissionsListView->currentIndex().isValid())
        m_permissionsModel->removePermission(m_ui->permissionsListView->currentIndex().row());
    m_ui->removePermissionButton->setEnabled(m_permissionsModel->permissions().size());
}

void AndroidPackageCreationWidget::savePermissionsButton()
{
    setEnabledSaveDiscardButtons(false);
    AndroidManager::setPermissions(m_step->target(), m_permissionsModel->permissions());
}

void AndroidPackageCreationWidget::discardPermissionsButton()
{
    setEnabledSaveDiscardButtons(false);
    m_permissionsModel->setPermissions(AndroidManager::permissions(m_step->target()));
    m_ui->permissionsComboBox->setCurrentIndex(-1);
    m_ui->removePermissionButton->setEnabled(m_permissionsModel->permissions().size());
}

void AndroidPackageCreationWidget::updateRequiredLibrariesModels()
{
    m_qtLibsModel->setCheckedItems(AndroidManager::qtLibs(m_step->target()));
    m_prebundledLibs->setCheckedItems(AndroidManager::prebundledLibs(m_step->target()));
}

void AndroidPackageCreationWidget::readElfInfo()
{
    m_step->checkRequiredLibraries();
}

void AndroidPackageCreationWidget::setEnabledSaveDiscardButtons(bool enabled)
{
    if (!enabled)
        m_ui->permissionsListView->setFocus();
    m_ui->savePermissionsButton->setEnabled(enabled);
    m_ui->discardPermissionsButton->setEnabled(enabled);
}

QString AndroidPackageCreationWidget::summaryText() const
{
    return tr("<b>Package configurations</b>");
}

QString AndroidPackageCreationWidget::displayName() const
{
    return m_step->displayName();
}

void AndroidPackageCreationWidget::setCertificates()
{
    QAbstractItemModel *certificates = m_step->keystoreCertificates();
    m_ui->signPackageCheckBox->setChecked(certificates);
    m_ui->certificatesAliasComboBox->setModel(certificates);
}

void AndroidPackageCreationWidget::on_signPackageCheckBox_toggled(bool checked)
{
    if (!checked)
        return;
    if (!m_step->keystorePath().isEmpty())
        setCertificates();
}

void AndroidPackageCreationWidget::on_KeystoreCreatePushButton_clicked()
{
    AndroidCreateKeystoreCertificate d;
    if (d.exec() != QDialog::Accepted)
        return;
    m_ui->KeystoreLocationLineEdit->setText(d.keystoreFilePath().toUserOutput());
    m_step->setKeystorePath(d.keystoreFilePath());
    m_step->setKeystorePassword(d.keystorePassword());
    m_step->setCertificateAlias(d.certificateAlias());
    m_step->setCertificatePassword(d.certificatePassword());
    setCertificates();
}

void AndroidPackageCreationWidget::on_KeystoreLocationPushButton_clicked()
{
    Utils::FileName keystorePath = m_step->keystorePath();
    if (keystorePath.isEmpty())
        keystorePath = Utils::FileName::fromString(QDir::homePath());
    Utils::FileName file = Utils::FileName::fromString(QFileDialog::getOpenFileName(this, tr("Select keystore file"), keystorePath.toString(), tr("Keystore files (*.keystore *.jks)")));
    if (file.isEmpty())
        return;
    m_ui->KeystoreLocationLineEdit->setText(file.toUserOutput());
    m_step->setKeystorePath(file);
    m_ui->signPackageCheckBox->setChecked(false);
}

void AndroidPackageCreationWidget::on_certificatesAliasComboBox_activated(const QString &alias)
{
    if (alias.length())
        m_step->setCertificateAlias(alias);
}

void AndroidPackageCreationWidget::on_certificatesAliasComboBox_currentIndexChanged(const QString &alias)
{
    if (alias.length())
        m_step->setCertificateAlias(alias);
}

void AndroidPackageCreationWidget::on_openPackageLocationCheckBox_toggled(bool checked)
{
    m_step->setOpenPackageLocation(checked);
}

} // namespace Internal
} // namespace Android
