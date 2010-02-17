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

#include "qt4projectmanager/qt4target.h"
#include "qt4projectmanager/qtversionmanager.h"

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
    QSet<QString> targets = vm->supportedTargetIds();

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

    m_isComplete = isQtVersionChecked;
    emit completeChanged();
}

QSet<QString> TargetsPage::selectedTargets() const
{
    QSet<QString> result;
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QString target = m_treeWidget->topLevelItem(i)->data(0, Qt::UserRole).toString();
        QList<int> versions = selectedVersionIdsForTarget(target);
        if (!versions.isEmpty())
            result.insert(target);
    }
    return result;
}

QList<int> TargetsPage::selectedVersionIdsForTarget(const QString &t) const
{
    QList<int> result;
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem * current = m_treeWidget->topLevelItem(i);
        QString target = current->data(0, Qt::UserRole).toString();
        if (t != target)
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
