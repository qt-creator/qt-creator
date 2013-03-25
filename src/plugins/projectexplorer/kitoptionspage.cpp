/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "kitoptionspage.h"

#include "kitmodel.h"
#include "kit.h"
#include "projectexplorerconstants.h"
#include "kitmanagerconfigwidget.h"
#include "kitmanager.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// KitOptionsPage:
// --------------------------------------------------------------------------

KitOptionsPage::KitOptionsPage() :
    m_model(0), m_selectionModel(0), m_currentWidget(0), m_toShow(0)
{
    setId(Constants::KITS_SETTINGS_PAGE_ID);
    setDisplayName(tr("Kits"));
    setCategory(Constants::PROJECTEXPLORER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
                                       Constants::PROJECTEXPLORER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::PROJECTEXPLORER_SETTINGS_CATEGORY_ICON));
}

QWidget *KitOptionsPage::createPage(QWidget *parent)
{
    m_configWidget = new QWidget(parent);

    m_kitsView = new QTreeView(m_configWidget);
    m_kitsView->setUniformRowHeights(true);
    m_kitsView->header()->setStretchLastSection(true);
    m_kitsView->setSizePolicy(m_kitsView->sizePolicy().horizontalPolicy(),
                                  QSizePolicy::Ignored);

    m_addButton = new QPushButton(tr("Add"), m_configWidget);
    m_cloneButton = new QPushButton(tr("Clone"), m_configWidget);
    m_delButton = new QPushButton(tr("Remove"), m_configWidget);
    m_makeDefaultButton = new QPushButton(tr("Make Default"), m_configWidget);

    QVBoxLayout *buttonLayout = new QVBoxLayout();
    buttonLayout->setSpacing(6);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_cloneButton);
    buttonLayout->addWidget(m_delButton);
    buttonLayout->addWidget(m_makeDefaultButton);
    buttonLayout->addStretch();

    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(m_kitsView);
    horizontalLayout->addLayout(buttonLayout);

    QVBoxLayout *verticalLayout = new QVBoxLayout(m_configWidget);
    verticalLayout->addLayout(horizontalLayout);

    m_model = new Internal::KitModel(verticalLayout);
    connect(m_model, SIGNAL(kitStateChanged()), this, SLOT(updateState()));
    verticalLayout->setStretch(0, 1);
    verticalLayout->setStretch(1, 0);

    m_kitsView->setModel(m_model);
    m_kitsView->header()->setResizeMode(0, QHeaderView::Stretch);
    m_kitsView->expandAll();

    m_selectionModel = m_kitsView->selectionModel();
    connect(m_selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(kitSelectionChanged()));
    connect(KitManager::instance(), SIGNAL(kitAdded(ProjectExplorer::Kit*)),
            this, SLOT(kitSelectionChanged()));
    connect(KitManager::instance(), SIGNAL(kitRemoved(ProjectExplorer::Kit*)),
            this, SLOT(kitSelectionChanged()));
    connect(KitManager::instance(), SIGNAL(kitUpdated(ProjectExplorer::Kit*)),
            this, SLOT(kitSelectionChanged()));

    // Set up add menu:
    connect(m_addButton, SIGNAL(clicked()), this, SLOT(addNewKit()));
    connect(m_cloneButton, SIGNAL(clicked()), this, SLOT(cloneKit()));
    connect(m_delButton, SIGNAL(clicked()), this, SLOT(removeKit()));
    connect(m_makeDefaultButton, SIGNAL(clicked()), this, SLOT(makeDefaultKit()));

    m_searchKeywords = tr("Kits");

    updateState();

    if (m_toShow) {
        QModelIndex index = m_model->indexOf(m_toShow);
        m_selectionModel->select(index,
                                 QItemSelectionModel::Clear
                                 | QItemSelectionModel::SelectCurrent
                                 | QItemSelectionModel::Rows);
        m_kitsView->scrollTo(index);
    }
    m_toShow = 0;

    return m_configWidget;
}

void KitOptionsPage::apply()
{
    if (m_model)
        m_model->apply();
}

void KitOptionsPage::finish()
{
    if (m_model) {
        delete m_model;
        m_model = 0;
    }

    m_configWidget = 0; // deleted by settingsdialog
    m_selectionModel = 0; // child of m_configWidget
    m_kitsView = 0; // child of m_configWidget
    m_currentWidget = 0; // deleted by the model
    m_toShow = 0;
}

bool KitOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

void KitOptionsPage::showKit(Kit *k)
{
    m_toShow = k;
}

void KitOptionsPage::kitSelectionChanged()
{
    QModelIndex current = currentIndex();
    QWidget *newWidget = current.isValid() ? m_model->widget(current) : 0;
    if (newWidget == m_currentWidget)
        return;

    if (m_currentWidget)
        m_currentWidget->setVisible(false);

    m_currentWidget = newWidget;

    if (m_currentWidget) {
        m_currentWidget->setVisible(true);
        m_kitsView->scrollTo(current);
    }
    updateState();
}

void KitOptionsPage::addNewKit()
{
    Kit *k = m_model->markForAddition(0);

    QModelIndex newIdx = m_model->indexOf(k);
    m_selectionModel->select(newIdx,
                             QItemSelectionModel::Clear
                             | QItemSelectionModel::SelectCurrent
                             | QItemSelectionModel::Rows);
}

void KitOptionsPage::cloneKit()
{
    Kit *current = m_model->kit(currentIndex());
    if (!current)
        return;

    Kit *k = m_model->markForAddition(current);
    QModelIndex newIdx = m_model->indexOf(k);
    m_kitsView->scrollTo(newIdx);
    m_selectionModel->select(newIdx,
                             QItemSelectionModel::Clear
                             | QItemSelectionModel::SelectCurrent
                             | QItemSelectionModel::Rows);
}

void KitOptionsPage::removeKit()
{
    Kit *k = m_model->kit(currentIndex());
    if (!k)
        return;
    m_model->markForRemoval(k);
}

void KitOptionsPage::makeDefaultKit()
{
    m_model->setDefaultKit(currentIndex());
    updateState();
}

void KitOptionsPage::updateState()
{
    if (!m_kitsView)
        return;

    bool canCopy = false;
    bool canDelete = false;
    bool canMakeDefault = false;
    QModelIndex index = currentIndex();
    Kit *k = m_model->kit(index);
    if (k) {
        canCopy = true;
        canDelete = !k->isAutoDetected();
        canMakeDefault = !m_model->isDefaultKit(index);
    }

    m_cloneButton->setEnabled(canCopy);
    m_delButton->setEnabled(canDelete);
    m_makeDefaultButton->setEnabled(canMakeDefault);
}

QModelIndex KitOptionsPage::currentIndex() const
{
    if (!m_selectionModel)
        return QModelIndex();

    QModelIndexList idxs = m_selectionModel->selectedRows();
    if (idxs.count() != 1)
        return QModelIndex();
    return idxs.at(0);
}

} // namespace ProjectExplorer
