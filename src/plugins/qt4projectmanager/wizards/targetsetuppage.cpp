/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "targetsetuppage.h"

#include "ui_targetsetuppage.h"

#include "qt4project.h"
#include "qt4projectmanagerconstants.h"
#include "qt4target.h"
#include "qtversionmanager.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QtGui/QAction>
#include <QtGui/QFileDialog>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QTreeWidget>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

namespace {
enum Columns {
    NAME_COLUMN = 0,
    STATUS_COLUMN,
    DIRECTORY_COLUMN
};
} // namespace

TargetSetupPage::TargetSetupPage(QWidget *parent) :
    QWizardPage(parent),
    m_preferMobile(false),
    m_toggleWillCheck(false),
    m_ui(new Ui::TargetSetupPage),
    m_contextMenu(0)
{
    m_ui->setupUi(this);
    m_ui->versionTree->header()->setResizeMode(0, QHeaderView::ResizeToContents);
    m_ui->versionTree->header()->setResizeMode(1, QHeaderView::ResizeToContents);

    m_ui->versionTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_contextMenu = new QMenu(this);

    connect(m_ui->importButton, SIGNAL(clicked()),
            this, SLOT(addShadowBuildLocation()));
    connect(m_ui->uncheckButton, SIGNAL(clicked()),
            this, SLOT(checkAllButtonClicked()));
    connect(m_ui->versionTree, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(contextMenuRequested(QPoint)));

    setTitle(tr("Qt Versions"));
}

void TargetSetupPage::initializePage()
{
    // WORKAROUND: Somebody sets all buttons to autoDefault between the ctor and here!
    m_ui->importButton->setAutoDefault(false);
    m_ui->uncheckButton->setAutoDefault(false);
}

TargetSetupPage::~TargetSetupPage()
{
    resetInfos();
    delete m_ui;
}

void TargetSetupPage::setImportInfos(const QList<ImportInfo> &infos)
{
    disconnect(m_ui->versionTree, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
               this, SLOT(itemWasChanged()));

    // Create a list of all temporary Qt versions we need to delete in our existing list
    QList<QtVersion *> toDelete;
    foreach (const ImportInfo &info, m_infos) {
        if (info.isTemporary)
            toDelete.append(info.version);
    }
    // Remove those that got copied into the new list to set up
    foreach (const ImportInfo &info, infos) {
        if (info.isTemporary)
            toDelete.removeAll(info.version);
    }
    // Delete the rest
    qDeleteAll(toDelete);
    // ... and clear the list
    m_infos.clear();

    // Find possible targets:
    QStringList targetIds;
    foreach (const ImportInfo &i, infos) {
        // Make sure we have no duplicate directories:
        bool skip = false;
        foreach (const ImportInfo &j, m_infos) {
            if (j.isExistingBuild && i.isExistingBuild && (j.directory == i.directory)) {
                skip = true;
                break;
            }
        }
        if (skip) {
            if (i.isTemporary)
                delete i.version;
            continue;
        }

        m_infos.append(i);

        QSet<QString> versionTargets = i.version->supportedTargetIds();
        foreach (const QString &t, versionTargets) {
            if (!targetIds.contains(t))
                targetIds.append(t);
        }
    }
    qSort(targetIds.begin(), targetIds.end());

    m_ui->versionTree->clear();

    foreach (const QString &targetId, targetIds) {
        QTreeWidgetItem *targetItem = new QTreeWidgetItem(m_ui->versionTree);

        Qt4BaseTargetFactory *factory = 0;
        QList<Qt4BaseTargetFactory *> factories =
                ExtensionSystem::PluginManager::instance()->getObjects<Qt4BaseTargetFactory>();
        foreach (Qt4BaseTargetFactory *fac, factories) {
            if (fac->supportsTargetId(targetId)) {
                factory = fac;
                break;
            }
        }
        if (!factory)
            continue;

        QString displayName = factory->displayNameForId(targetId);

        targetItem->setText(0, displayName);
        targetItem->setToolTip(0, displayName);
        targetItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        targetItem->setData(NAME_COLUMN, Qt::UserRole, targetId);
        targetItem->setExpanded(true);

        int pos = -1;
        foreach (const ImportInfo &i, m_infos) {
            ++pos;

            if (!i.version->supportsTargetId(targetId))
                continue;
            QTreeWidgetItem *versionItem = new QTreeWidgetItem(targetItem);
            updateVersionItem(versionItem, pos);

            // Prefer imports to creating new builds, but precheck any
            // Qt that exists (if there is no import with that version)
            bool shouldCheck = true;
            if (!i.isExistingBuild) {
                foreach (const ImportInfo &j, m_infos) {
                    if (j.isExistingBuild && j.version == i.version) {
                        shouldCheck = false;
                        break;
                    }
                }
            }

            shouldCheck = shouldCheck && (m_preferMobile == i.version->supportsMobileTarget());
            shouldCheck = shouldCheck || i.isExistingBuild; // always check imports
            shouldCheck = shouldCheck || m_infos.count() == 1; // always check only option
            versionItem->setCheckState(NAME_COLUMN, shouldCheck ? Qt::Checked : Qt::Unchecked);
        }
    }

    connect(m_ui->versionTree, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(itemWasChanged()));

    emit completeChanged();
}

QList<TargetSetupPage::ImportInfo> TargetSetupPage::importInfos() const
{
    return m_infos;
}

bool TargetSetupPage::hasSelection() const
{
    for (int i = 0; i < m_ui->versionTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem * current = m_ui->versionTree->topLevelItem(i);
        for (int j = 0; j < current->childCount(); ++j) {
            QTreeWidgetItem * child = current->child(j);
            if (child->checkState(NAME_COLUMN) == Qt::Checked)
                return true;
        }
    }
    return false;
}

bool TargetSetupPage::isTargetSelected(const QString &targetid) const
{
    for (int i = 0; i < m_ui->versionTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem * current = m_ui->versionTree->topLevelItem(i);
        if (current->data(NAME_COLUMN, Qt::UserRole).toString() != targetid)
            continue;
        for (int j = 0; j < current->childCount(); ++j) {
            QTreeWidgetItem * child = current->child(j);
            if (child->checkState(NAME_COLUMN) == Qt::Checked)
                return true;
        }
    }
    return false;
}

bool TargetSetupPage::setupProject(Qt4ProjectManager::Qt4Project *project)
{
    Q_ASSERT(project->targets().isEmpty());
    QtVersionManager *vm = QtVersionManager::instance();
    for (int i = 0; i < m_ui->versionTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *current = m_ui->versionTree->topLevelItem(i);
        QString targetId = current->data(NAME_COLUMN, Qt::UserRole).toString();

        Qt4BaseTargetFactory *qt4TargetFactory = Qt4BaseTargetFactory::qt4BaseTargetFactoryForId(targetId);
        if (qt4TargetFactory && qt4TargetFactory->canCreate(project, targetId)) {
            QList<BuildConfigurationInfo> targetInfos;
            for (int j = 0; j < current->childCount(); ++j) {
                QTreeWidgetItem *child = current->child(j);
                if (child->checkState(0) != Qt::Checked)
                    continue;
                ImportInfo &info = m_infos[child->data(0, Qt::UserRole).toInt()];

                // Register temporary Qt version
                if (info.isTemporary) {
                    vm->addVersion(info.version);
                    info.isTemporary = false;
                }

                targetInfos.append(BuildConfigurationInfo(info.version, info.buildConfig,
                                                          info.additionalArguments, info.directory));
            }

            // create the target:
            Qt4BaseTarget *target = 0;
            if (!targetInfos.isEmpty()) {
                target = qt4TargetFactory->create(project, targetId, targetInfos);
            }

            if (target) {
                project->addTarget(target);
                if (target->id() == QLatin1String(Constants::QT_SIMULATOR_TARGET_ID))
                    project->setActiveTarget(target);
            }
        }
    }


    // Create a desktop target if nothing else was set up:
    if (project->targets().isEmpty()) {
        Qt4BaseTargetFactory *tf = Qt4BaseTargetFactory::qt4BaseTargetFactoryForId(Constants::DESKTOP_TARGET_ID);
        if (tf) {
            Qt4BaseTarget *target = tf->create(project, Constants::DESKTOP_TARGET_ID);
            if (target)
                project->addTarget(target);
        }
    }

    // Select active target
    // a) Simulator target
    // b) Desktop target
    // c) the first target
    ProjectExplorer::Target *activeTarget = 0;
    QList<ProjectExplorer::Target *> targets = project->targets();
    foreach (ProjectExplorer::Target *t, targets) {
        if (t->id() == Constants::QT_SIMULATOR_TARGET_ID)
            activeTarget = t;
        else if (!activeTarget && t->id() == Constants::DESKTOP_TARGET_ID)
            activeTarget = t;
    }
    if (!activeTarget && !targets.isEmpty())
        activeTarget = targets.first();
    if (activeTarget)
        project->setActiveTarget(activeTarget);

    return !project->targets().isEmpty();
}

void TargetSetupPage::itemWasChanged()
{
    emit completeChanged();
}

bool TargetSetupPage::isComplete() const
{
    return hasSelection();
}

void TargetSetupPage::setImportDirectoryBrowsingEnabled(bool browsing)
{
    m_ui->importButton->setEnabled(browsing);
    m_ui->importButton->setVisible(browsing);
}

void TargetSetupPage::setImportDirectoryBrowsingLocation(const QString &directory)
{
    m_defaultShadowBuildLocation = directory;
}

void TargetSetupPage::setPreferMobile(bool mobile)
{
    m_preferMobile = mobile;
}

void TargetSetupPage::setProFilePath(const QString &path)
{
    m_proFilePath = path;
    if (!m_proFilePath.isEmpty()) {
        m_ui->descriptionLabel->setText(tr("Qt Creator can set up the following targets for<br>project <b>%1</b>:",
                                           "%1: Project name").arg(QFileInfo(m_proFilePath).baseName()));
    }
    // Force regeneration of tree widget contents:
    QList<ImportInfo> tmp = m_infos;
    setImportInfos(tmp);
}

QList<TargetSetupPage::ImportInfo> TargetSetupPage::importInfosForKnownQtVersions(const QString &proFilePath)
{
    QList<Qt4BaseTargetFactory *> factories =
            ExtensionSystem::PluginManager::instance()->getObjects<Qt4BaseTargetFactory>();

    QList<BuildConfigurationInfo> bcinfos;

    foreach (Qt4BaseTargetFactory *fac, factories)
        bcinfos.append(fac->availableBuildConfigurations(proFilePath));

    QList<ImportInfo> infos;
    foreach (const BuildConfigurationInfo &info, bcinfos) {
        infos.append(ImportInfo(info));
    }

    if (infos.isEmpty()) {
        // Fallback to the Qt in Path version
        ImportInfo info;
        info.isExistingBuild = false;
        info.isTemporary = false;
        info.version = QtVersionManager::instance()->versions().at(0);
        info.buildConfig = info.version->defaultBuildConfig();
        infos.append(info);
    }
    return infos;
}

QList<TargetSetupPage::ImportInfo> TargetSetupPage::filterImportInfos(const QSet<QString> &validTargets,
                                                                      const QList<ImportInfo> &infos)
{
    QList<ImportInfo> results;
    foreach (const ImportInfo &info, infos) {
        Q_ASSERT(info.version);
        foreach (const QString &target, validTargets) {
            if (info.version->supportsTargetId(target))
                results.append(info);
        }
    }
    return results;
}

QList<TargetSetupPage::ImportInfo>
TargetSetupPage::scanDefaultProjectDirectories(Qt4ProjectManager::Qt4Project *project)
{
    // Source directory:
    QList<ImportInfo> importVersions = TargetSetupPage::recursivelyCheckDirectoryForBuild(project->projectDirectory(),
                                                                                          project->file()->fileName());
    QtVersionManager *vm = QtVersionManager::instance();
    foreach(const QString &id, vm->supportedTargetIds()) {
        Qt4BaseTargetFactory *qt4Factory = Qt4BaseTargetFactory::qt4BaseTargetFactoryForId(id);
        QString location = qt4Factory->defaultShadowBuildDirectory(project->defaultTopLevelBuildDirectory(), id);
        importVersions.append(TargetSetupPage::recursivelyCheckDirectoryForBuild(location,
                                                                                 project->file()->fileName()));
    }
    return importVersions;
}

QList<TargetSetupPage::ImportInfo>
TargetSetupPage::recursivelyCheckDirectoryForBuild(const QString &directory, const QString &proFile, int maxdepth)
{
    QList<ImportInfo> results;

    if (maxdepth <= 0 || directory.isEmpty())
        return results;

    // Check for in-source builds first:
    QString qmakeBinary = QtVersionManager::findQMakeBinaryFromMakefile(directory + "/Makefile");
    QDir dir(directory);

    // Recurse into subdirectories:
    if (qmakeBinary.isNull() || !QtVersionManager::makefileIsFor(directory + "/Makefile", proFile)) {
        QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        foreach (QString subDir, subDirs)
            results.append(recursivelyCheckDirectoryForBuild(dir.absoluteFilePath(subDir),
                                                             proFile, maxdepth - 1));
        return results;
    }

    // Shiny fresh directory with a Makefile...
    QtVersionManager * vm = QtVersionManager::instance();
    TargetSetupPage::ImportInfo info;
    info.directory = dir.absolutePath();

    // This also means we have a build in there
    // First get the qt version
    info.version = vm->qtVersionForQMakeBinary(qmakeBinary);
    info.isExistingBuild = true;

    // Okay does not yet exist, create
    if (!info.version) {
        info.version = new QtVersion(qmakeBinary);
        info.isTemporary = true;
    }

    QPair<QtVersion::QmakeBuildConfigs, QString> result =
            QtVersionManager::scanMakeFile(directory + "/Makefile", info.version->defaultBuildConfig());
    info.buildConfig = result.first;
    QString aa = result.second;
    QString parsedSpec = Qt4BuildConfiguration::extractSpecFromArguments(&aa, directory, info.version);
    QString versionSpec = info.version->mkspec();

    // Compare mkspecs and add to additional arguments
    if (parsedSpec.isEmpty() || parsedSpec == versionSpec || parsedSpec == "default") {
        // using the default spec, don't modify additional arguments
    } else {
        info.additionalArguments = "-spec " + Utils::QtcProcess::quoteArg(parsedSpec);
    }
    Utils::QtcProcess::addArgs(&info.additionalArguments, aa);

    results.append(info);
    return results;
}

void TargetSetupPage::addShadowBuildLocation()
{
    QString newPath =
        QFileDialog::getExistingDirectory(this,
                                          tr("Choose a directory to scan for additional shadow builds"),
                                          m_defaultShadowBuildLocation);

    if (newPath.isEmpty())
        return;

    QFileInfo dir(QDir::fromNativeSeparators(newPath));
    if (!dir.exists() || !dir.isDir())
        return;

    QList<ImportInfo> tmp;
    tmp.append(recursivelyCheckDirectoryForBuild(dir.absoluteFilePath(), m_proFilePath));
    if (tmp.isEmpty()) {
        QMessageBox::warning(this, tr("No builds found"),
                             tr("No builds for project file \"%1\" were found in the folder \"%2\".",
                                "%1: pro-file, %2: directory that was checked.").
                             arg(m_proFilePath, dir.absoluteFilePath()));
        return;
    }
    tmp.append(m_infos);
    setImportInfos(tmp);
}

void TargetSetupPage::checkAll(bool checked)
{
    for (int i = 0; i < m_ui->versionTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *current = m_ui->versionTree->topLevelItem(i);
        for (int j = 0; j < current->childCount(); ++j)
            checkOne(checked, current->child(j));
    }
}

void TargetSetupPage::checkOne(bool checked, QTreeWidgetItem *item)
{
    Q_ASSERT(item->parent()); // we are a version item
    item->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
}

void TargetSetupPage::checkAllButtonClicked()
{
    checkAll(m_toggleWillCheck);

    m_toggleWillCheck = !m_toggleWillCheck;
    m_ui->uncheckButton->setText(m_toggleWillCheck ? tr("Check All") : tr("Uncheck All"));
    m_ui->uncheckButton->setToolTip(m_toggleWillCheck
                                    ? tr("Check all Qt versions") : tr("Uncheck all Qt versions"));
}

void TargetSetupPage::checkAllTriggered()
{
    m_toggleWillCheck = true;
    checkAllButtonClicked();
}

void TargetSetupPage::uncheckAllTriggered()
{
    m_toggleWillCheck = false;
    checkAllButtonClicked();
}

void TargetSetupPage::checkOneTriggered()
{
    QAction * action = qobject_cast<QAction *>(sender());
    if (!action)
        return;
    QTreeWidgetItem *item = static_cast<QTreeWidgetItem *>(action->data().value<void *>());
    if (!item || !item->parent())
        return;

    checkAll(false);
    checkOne(true, item);
}

void TargetSetupPage::contextMenuRequested(const QPoint &position)
{
    m_contextMenu->clear();

    QTreeWidgetItem *item = m_ui->versionTree->itemAt(position);
    m_contextMenu = new QMenu(this);
    if (item && item->parent()) {
        // Qt version item
        QAction *onlyThisAction = new QAction(tr("Check only this version"), m_contextMenu);
        connect(onlyThisAction, SIGNAL(triggered()), this, SLOT(checkOneTriggered()));
        onlyThisAction->setData(QVariant::fromValue(static_cast<void *>(item)));
        m_contextMenu->addAction(onlyThisAction);

        QAction *checkAllAction = new QAction(tr("Check all versions"), m_contextMenu);
        connect(checkAllAction, SIGNAL(triggered()), this, SLOT(checkAllTriggered()));
        m_contextMenu->addAction(checkAllAction);

        QAction *uncheckAllAction = new QAction(tr("Uncheck all versions"), m_contextMenu);
        connect(uncheckAllAction, SIGNAL(triggered()), this, SLOT(uncheckAllTriggered()));
        m_contextMenu->addAction(uncheckAllAction);
    }
    if (!m_contextMenu->isEmpty())
        m_contextMenu->popup(m_ui->versionTree->mapToGlobal(position));
}

void TargetSetupPage::resetInfos()
{
    m_ui->versionTree->clear();
    foreach (const ImportInfo &info, m_infos) {
        if (info.isTemporary)
            delete info.version;
    }
    m_infos.clear();
}

QPair<QIcon, QString> TargetSetupPage::reportIssues(Qt4ProjectManager::QtVersion *version,
                                                    const QString &buildDir)
{
    if (m_proFilePath.isEmpty())
        return qMakePair(QIcon(), QString());

    const ProjectExplorer::TaskHub *taskHub = ExtensionSystem::PluginManager::instance()
                                              ->getObject<ProjectExplorer::TaskHub>();
    QTC_ASSERT(taskHub, return qMakePair(QIcon(), QString()));

    QList<ProjectExplorer::Task> issues = version->reportIssues(m_proFilePath, buildDir);

    QString text;
    QIcon icon;
    foreach (const ProjectExplorer::Task &t, issues) {
        if (!text.isEmpty())
            text.append(QLatin1String("<br>"));
        // set severity:
        QString severity;
        if (t.type == ProjectExplorer::Task::Error) {
            icon = taskHub->taskTypeIcon(t.type);
            severity = tr("<b>Error:</b> ", "Severity is Task::Error");
        } else if (t.type == ProjectExplorer::Task::Warning) {
               if (icon.isNull())
                   icon = taskHub->taskTypeIcon(t.type);
               severity = tr("<b>Warning:</b> ", "Severity is Task::Warning");
        }
        text.append(severity + t.description);
    }
    if (!text.isEmpty())
        text = QLatin1String("<nobr>") + text;
    return qMakePair(icon, text);
}

void TargetSetupPage::updateVersionItem(QTreeWidgetItem *versionItem, int index)
{
    ImportInfo &info = m_infos[index];
    QPair<QIcon, QString> issues = reportIssues(info.version, info.directory);

    //: We are going to build debug and release
    QString buildType = tr("debug and release");
    if ((info.buildConfig & QtVersion::BuildAll) == 0) {
        if (info.buildConfig & QtVersion::DebugBuild)
            //: Debug build
            buildType = tr("debug");
        else
            //: release build
            buildType = tr("release");
    }

    QString toolTip = info.version->displayName();
    //: %1: qmake used (incl. full path), %2: "debug", "release" or "debug and release"
    toolTip.append(tr("<br>using %1 (%2)").
            arg(QDir::toNativeSeparators(info.version->qmakeCommand())).
                   arg(buildType));
    if (!issues.second.isEmpty())
        toolTip.append(QString::fromLatin1("<br><br>%1").arg(issues.second));

    // Column 0:
    versionItem->setToolTip(NAME_COLUMN, toolTip);
    versionItem->setIcon(NAME_COLUMN, issues.first);
    versionItem->setText(NAME_COLUMN, info.version->displayName() + " " + buildType);
    versionItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    versionItem->setData(NAME_COLUMN, Qt::UserRole, index);

    // Column 1 (status):
    const QString status = info.isExistingBuild ?
                           //: Is this an import of an existing build or a new one?
                           tr("Import") :
                           //: Is this an import of an existing build or a new one?
                           tr("New");
    versionItem->setText(STATUS_COLUMN, status);
    versionItem->setToolTip(STATUS_COLUMN, status);

    // Column 2 (directory):
    Q_ASSERT(versionItem->parent());
    versionItem->setText(DIRECTORY_COLUMN, QDir::toNativeSeparators(info.directory));
    versionItem->setToolTip(DIRECTORY_COLUMN, QDir::toNativeSeparators(info.directory));
}
