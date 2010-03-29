/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "targetsetuppage.h"

#include "qt4project.h"
#include "qt4target.h"

#include <utils/pathchooser.h>

#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QTreeWidget>

using namespace Qt4ProjectManager::Internal;

TargetSetupPage::TargetSetupPage(QWidget *parent) :
    QWizardPage(parent)
{
    resize(500, 400);
    setTitle(tr("Set up targets for your project"));

    QVBoxLayout *vbox = new QVBoxLayout(this);

    QLabel * importLabel = new QLabel(this);
    importLabel->setText(tr("Qt Creator can set up the following targets:"));
    importLabel->setWordWrap(true);

    vbox->addWidget(importLabel);

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setColumnCount(3);
    m_treeWidget->header()->setResizeMode(0, QHeaderView::ResizeToContents);
    m_treeWidget->header()->setResizeMode(1, QHeaderView::ResizeToContents);
    m_treeWidget->setHeaderLabels(QStringList() << tr("Qt Version") << tr("Status") << tr("Directory"));
    vbox->addWidget(m_treeWidget);

    QHBoxLayout *hbox = new QHBoxLayout;
    m_directoryLabel = new QLabel(this);
    m_directoryLabel->setText(tr("Scan for builds"));
    hbox->addWidget(m_directoryLabel);

    m_directoryChooser = new Utils::PathChooser(this);
    m_directoryChooser->setPromptDialogTitle(tr("Directory to import builds from"));
    m_directoryChooser->setExpectedKind(Utils::PathChooser::Directory);
    hbox->addWidget(m_directoryChooser);
    vbox->addLayout(hbox);

    connect(m_directoryChooser, SIGNAL(changed(QString)),
            this, SLOT(importDirectoryAdded(QString)));
}

TargetSetupPage::~TargetSetupPage()
{
    resetInfos();
}

void TargetSetupPage::setImportInfos(const QList<ImportInfo> &infos)
{
    disconnect(m_treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
               this, SLOT(itemWasChanged()));

    // Clean up!
    resetInfos();

    // Find possible targets:
    QStringList targets;
    foreach (const ImportInfo &i, infos) {
        // Make sure we have no duplicate directories/version pairs:
        bool skip = false;
        foreach (const ImportInfo &j, m_infos) {
            if ((j.directory == i.directory) &&
                (j.version == i.version)) {
                skip = true;
                break;
            }
        }
        if (skip)
            continue;

        m_infos.append(i);

        QSet<QString> versionTargets = i.version->supportedTargetIds();
        foreach (const QString &t, versionTargets) {
            if (!targets.contains(t))
                targets.append(t);
        }
    }
    qSort(targets.begin(), targets.end());

    Qt4TargetFactory factory;
    foreach (const QString &t, targets) {
        QTreeWidgetItem *targetItem = new QTreeWidgetItem(m_treeWidget);
        targetItem->setText(0, factory.displayNameForId(t));
        targetItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        targetItem->setData(0, Qt::UserRole, t);
        targetItem->setExpanded(true);

        int pos = -1;
        foreach (const ImportInfo &i, infos) {
            ++pos;

            if (!i.version->supportsTargetId(t))
                continue;
            QTreeWidgetItem *versionItem = new QTreeWidgetItem(targetItem);
            // Column 0:
            versionItem->setText(0, i.version->displayName());
            versionItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            versionItem->setData(0, Qt::UserRole, pos);
            // Prefer imports to creating new builds, but precheck any
            // Qt that exists (if there is no import with that version)
            if (!i.isExistingBuild) {
                bool haveExistingBuildForQtVersion = false;
                foreach (const ImportInfo &j, m_infos) {
                    if (j.isExistingBuild && j.version == i.version) {
                        haveExistingBuildForQtVersion = true;
                        break;
                    }
                }
                versionItem->setCheckState(0, haveExistingBuildForQtVersion ? Qt::Unchecked : Qt::Checked);
            } else {
                versionItem->setCheckState(0, Qt::Checked);
            }

            // Column 1 (status):
            versionItem->setText(1, i.isExistingBuild ? tr("Import", "Is this an import of an existing build or a new one?") :
                                                        tr("New", "Is this an import of an existing build or a new one?"));

            // Column 2 (directory):
            versionItem->setText(2, QDir::toNativeSeparators(i.directory));
        }
    }

    connect(m_treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(itemWasChanged()));

    emit completeChanged();
}

QList<TargetSetupPage::ImportInfo> TargetSetupPage::importInfos() const
{
    return m_infos;
}

bool TargetSetupPage::hasSelection() const
{
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem * current = m_treeWidget->topLevelItem(i);
        for (int j = 0; j < current->childCount(); ++j) {
            QTreeWidgetItem * child = current->child(j);
            if (child->checkState(0) == Qt::Checked)
                return true;
        }
    }
    return false;
}

bool TargetSetupPage::isTargetSelected(const QString &targetid) const
{
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem * current = m_treeWidget->topLevelItem(i);
        if (current->data(0, Qt::UserRole).toString() != targetid)
            continue;
        for (int j = 0; j < current->childCount(); ++j) {
            QTreeWidgetItem * child = current->child(j);
            if (child->checkState(0) == Qt::Checked)
                return true;
        }
    }
    return false;
}

bool TargetSetupPage::setupProject(Qt4ProjectManager::Qt4Project *project)
{
    Q_ASSERT(project->targets().isEmpty());

    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *current = m_treeWidget->topLevelItem(i);
        QString targetId = current->data(0, Qt::UserRole).toString();

        QList<BuildConfigurationInfo> targetInfos;
        for (int j = 0; j < current->childCount(); ++j) {
            QTreeWidgetItem *child = current->child(j);
            if (child->checkState(0) != Qt::Checked)
                continue;

            const ImportInfo &info = m_infos.at(child->data(0, Qt::UserRole).toInt());

            targetInfos.append(BuildConfigurationInfo(info.version, QtVersion::QmakeBuildConfigs(info.buildConfig | QtVersion::DebugBuild),
                                                      info.additionalArguments, info.directory));
            targetInfos.append(BuildConfigurationInfo(info.version, info.buildConfig,
                                                      info.additionalArguments, info.directory));
        }

        // create the target:
        Qt4Target *target = 0;
        if (targetInfos.isEmpty())
            target = project->targetFactory()->create(project, targetId);
        else
            target = project->targetFactory()->create(project, targetId, targetInfos);

        if (target)
            project->addTarget(target);
    }

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
    m_directoryChooser->setEnabled(browsing);
    m_directoryChooser->setVisible(browsing);
    m_directoryLabel->setVisible(browsing);
}

void TargetSetupPage::setImportDirectoryBrowsingLocation(const QString &directory)
{
    m_directoryChooser->setInitialBrowsePathBackup(directory);
}

void TargetSetupPage::setShowLocationInformation(bool location)
{
    m_treeWidget->setColumnCount(location ? 3 : 1);
}

QList<TargetSetupPage::ImportInfo>
TargetSetupPage::importInfosForKnownQtVersions(Qt4ProjectManager::Qt4Project *project)
{
    QList<ImportInfo> results;
    QtVersionManager * vm = QtVersionManager::instance();
    QList<QtVersion *> validVersions = vm->validVersions();
    foreach (QtVersion *v, validVersions) {
        ImportInfo info;
        // ToDo: Check whether shadowbuilding is possible and use sourcedir if not:
        //       This needs a shadowbuilding patch to land
        if (project)
            info.directory = project->defaultTopLevelBuildDirectory();
        info.isExistingBuild = false;
        info.isTemporary = false;
        info.version = v;
        results.append(info);
    }
    return results;
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
TargetSetupPage::recursivelyCheckDirectoryForBuild(const QString &directory, int maxdepth)
{
    QList<ImportInfo> results;

    if (maxdepth <= 0)
        return results;

    // Check for in-source builds first:
    QString qmakeBinary =  QtVersionManager::findQMakeBinaryFromMakefile(directory);

    // Recurse into subdirectories:
    if (qmakeBinary.isNull()) {
        QStringList subDirs = QDir(directory).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        foreach (QString subDir, subDirs)
            results.append(recursivelyCheckDirectoryForBuild(directory + QChar('/') + subDir, maxdepth - 1));
        return results;
    }

    // Shiny fresh directory with a Makefile...
    QtVersionManager * vm = QtVersionManager::instance();
    TargetSetupPage::ImportInfo info;
    info.directory = directory;

    // This also means we have a build in there
    // First get the qt version
    info.version = vm->qtVersionForQMakeBinary(qmakeBinary);
    info.isExistingBuild = true;

    // Okay does not yet exist, create
    if (!info.version) {
        info.version = new QtVersion(qmakeBinary);
        info.isTemporary = true;
    }

    QPair<QtVersion::QmakeBuildConfigs, QStringList> result =
            QtVersionManager::scanMakeFile(directory, info.version->defaultBuildConfig());
    info.buildConfig = result.first;
    info.additionalArguments = Qt4BuildConfiguration::removeSpecFromArgumentList(result.second);

    QString parsedSpec = Qt4BuildConfiguration::extractSpecFromArgumentList(result.second, directory, info.version);
    QString versionSpec = info.version->mkspec();

    // Compare mkspecs and add to additional arguments
    if (parsedSpec.isEmpty() || parsedSpec == versionSpec || parsedSpec == "default") {
        // using the default spec, don't modify additional arguments
    } else {
        info.additionalArguments.prepend(parsedSpec);
        info.additionalArguments.prepend("-spec");
    }

    results.append(info);
    return results;
}

void TargetSetupPage::importDirectoryAdded(const QString &directory)
{
    QFileInfo dir(directory);
    if (!dir.exists() || !dir.isDir())
        return;
    m_directoryChooser->setPath(QString());
    QList<ImportInfo> tmp = m_infos;
    tmp.append(recursivelyCheckDirectoryForBuild(directory));
    setImportInfos(tmp);
}

void TargetSetupPage::resetInfos()
{
    m_treeWidget->clear();
    foreach (const ImportInfo &info, m_infos) {
        if (info.isTemporary)
            delete info.version;
    }
    m_infos.clear();
}
