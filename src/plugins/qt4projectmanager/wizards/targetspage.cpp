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

#include "targetspage.h"

#include "qt4projectmanager/qt4project.h"
#include "qt4projectmanager/qt4projectmanager.h"
#include "qt4projectmanager/qt4target.h"
#include "qt4projectmanager/qtversionmanager.h"

#include <extensionsystem/pluginmanager.h>

#include <QtCore/QSet>
#include <QtCore/QString>

#include <QtGui/QTreeWidget>
#include <QtGui/QLabel>
#include <QtGui/QLayout>

using namespace Qt4ProjectManager::Internal;

TargetsPage::TargetsPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Choose Qt versions"));

    QVBoxLayout *vbox = new QVBoxLayout(this);

    setTitle(tr("Select required Qt versions"));
    QLabel *label = new QLabel(tr("Select the Qt versions to use in your project."), this);
    label->setWordWrap(true);
    vbox->addWidget(label);

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderHidden(true);
    vbox->addWidget(m_treeWidget);

    QtVersionManager *vm = QtVersionManager::instance();
    QStringList targets = vm->supportedTargetIds().toList();
    qSort(targets.begin(), targets.end());

    Qt4TargetFactory factory;
    bool hasDesktop = targets.contains(QLatin1String(DESKTOP_TARGET_ID));
    bool isExpanded = false;
    bool isQtVersionChecked = false;

    foreach (const QString &t, targets) {
        QTreeWidgetItem *targetItem = new QTreeWidgetItem(m_treeWidget);
        targetItem->setText(0, factory.displayNameForId(t));
        targetItem->setFlags(Qt::ItemIsEnabled);
        targetItem->setData(0, Qt::UserRole, t);
        if (!isExpanded) {
            if ((hasDesktop && t == QLatin1String(DESKTOP_TARGET_ID)) ||
                !hasDesktop) {
                isExpanded = true;
                targetItem->setExpanded(true);
            }
        }

        foreach (QtVersion *v, vm->versionsForTargetId(t)) {
            QTreeWidgetItem *versionItem = new QTreeWidgetItem(targetItem);
            versionItem->setText(0, v->displayName());
            versionItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            versionItem->setData(0, Qt::UserRole, v->uniqueId());
            if (isExpanded && !isQtVersionChecked) {
                isQtVersionChecked = true;
                versionItem->setCheckState(0, Qt::Checked);
            } else {
                versionItem->setCheckState(0, Qt::Unchecked);
            }
        }
    }

    connect(m_treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
            this, SLOT(itemWasClicked()));

    emit completeChanged();
}

void TargetsPage::setValidTargets(const QSet<QString> &targets)
{
    if (targets.isEmpty())
        return;

    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *currentTargetItem = m_treeWidget->topLevelItem(i);
        QString currentTarget = currentTargetItem->data(0, Qt::UserRole).toString();
        if (targets.contains(currentTarget))
            currentTargetItem->setHidden(false);
        else
            currentTargetItem->setHidden(true);
    }

    // Make sure we have something checked!
    if (selectedTargets().isEmpty()) {
        for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem *currentTargetItem = m_treeWidget->topLevelItem(i);
            QString currentTarget = currentTargetItem->data(0, Qt::UserRole).toString();
            if (targets.contains(currentTarget) && currentTargetItem->childCount() >= 1) {
                currentTargetItem->child(0)->setCheckState(0, Qt::Checked);
                break;
            }
        }
    }
    emit completeChanged();
}

QSet<QString> TargetsPage::selectedTargets() const
{
    QSet<QString> result;
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem * targetItem = m_treeWidget->topLevelItem(i);
        QString target = targetItem->data(0, Qt::UserRole).toString();

        QList<int> versions = selectedQtVersionIdsForTarget(target);
        if (!versions.isEmpty())
            result.insert(target);
    }
    return result;
}

QList<int> TargetsPage::selectedQtVersionIdsForTarget(const QString &t) const
{
    QList<int> result;
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem * current = m_treeWidget->topLevelItem(i);
        QString target = current->data(0, Qt::UserRole).toString();
        if (t != target || current->isHidden())
            continue;

        for (int j = 0; j < current->childCount(); ++j) {
            QTreeWidgetItem * child = current->child(j);
            if (child->checkState(0) != Qt::Checked)
                continue;
            result.append(child->data(0, Qt::UserRole).toInt());
        }
    }
    return result;
}

void TargetsPage::itemWasClicked()
{
    emit completeChanged();
}

bool TargetsPage::isComplete() const
{
    return !selectedTargets().isEmpty();
}

bool TargetsPage::needToDisplayPage() const
{
    int targetCount = 0;
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem * current = m_treeWidget->topLevelItem(i);
        if (current->isHidden())
            continue;
        ++targetCount;
        if (targetCount > 1)
            return true;

        if (current->childCount() > 1)
            return true;
    }
    return false;
}

void TargetsPage::writeUserFile(const QString &proFileName) const
{
    Qt4Manager *manager = ExtensionSystem::PluginManager::instance()->getObject<Qt4Manager>();
    Q_ASSERT(manager);

    Qt4Project *pro = new Qt4Project(manager, proFileName);
    if (setupProject(pro))
        pro->saveSettings();
    delete pro;
}

bool TargetsPage::setupProject(Qt4ProjectManager::Qt4Project *project) const
{
    if (!project)
        return false;

    // Generate user settings:
    QSet<QString> targets = selectedTargets();
    if (targets.isEmpty())
        return false;

    QtVersionManager *vm = QtVersionManager::instance();

    foreach (const QString &targetId, targets) {
        QList<int> versionIds = selectedQtVersionIdsForTarget(targetId);
        QList<QtVersion *> versions;
        foreach (int id, versionIds)
            versions.append(vm->version(id));
        Qt4Target * target = project->targetFactory()->create(project, targetId, versions);
        project->addTarget(target);
    }
    return true;
}
