/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "profileoptionspage.h"

#include "profilemodel.h"
#include "profile.h"
#include "projectexplorerconstants.h"
#include "profileconfigwidget.h"
#include "profilemanager.h"

#include <coreplugin/icore.h>

#include <utils/qtcassert.h>

#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// ProfileOptionsPage:
// --------------------------------------------------------------------------

ProfileOptionsPage::ProfileOptionsPage() :
    m_model(0), m_selectionModel(0), m_currentWidget(0), m_toShow(0)
{
    setId(Constants::PROFILE_SETTINGS_PAGE_ID);
    setDisplayName(tr("Targets"));
    setCategory(QLatin1String(Constants::PROJECTEXPLORER_SETTINGS_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
                                       Constants::PROJECTEXPLORER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::PROJECTEXPLORER_SETTINGS_CATEGORY_ICON));
}

QWidget *ProfileOptionsPage::createPage(QWidget *parent)
{
    m_configWidget = new QWidget(parent);

    m_profilesView = new QTreeView(m_configWidget);
    m_profilesView->setUniformRowHeights(true);
    m_profilesView->header()->setStretchLastSection(true);

    m_addButton = new QPushButton(tr("Add"), m_configWidget);
    m_cloneButton = new QPushButton(tr("Clone"), m_configWidget);
    m_delButton = new QPushButton(tr("Remove"), m_configWidget);
    m_makeDefaultButton = new QPushButton(tr("Make Default"), m_configWidget);

    QVBoxLayout *buttonLayout = new QVBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_cloneButton);
    buttonLayout->addWidget(m_delButton);
    buttonLayout->addWidget(m_makeDefaultButton);
    buttonLayout->addItem(new QSpacerItem(10, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

    QVBoxLayout *verticalLayout = new QVBoxLayout();
    verticalLayout->addWidget(m_profilesView);

    QHBoxLayout *horizontalLayout = new QHBoxLayout(m_configWidget);
    horizontalLayout->addLayout(verticalLayout);
    horizontalLayout->addLayout(buttonLayout);

    Q_ASSERT(!m_model);
    m_model = new Internal::ProfileModel(verticalLayout);
    connect(m_model, SIGNAL(profileStateChanged()), this, SLOT(updateState()));

    m_profilesView->setModel(m_model);
    m_profilesView->header()->setResizeMode(0, QHeaderView::Stretch);
    m_profilesView->expandAll();

    m_selectionModel = m_profilesView->selectionModel();
    connect(m_selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(profileSelectionChanged()));
    connect(ProfileManager::instance(), SIGNAL(profileAdded(ProjectExplorer::Profile*)),
            this, SLOT(profileSelectionChanged()));
    connect(ProfileManager::instance(), SIGNAL(profileRemoved(ProjectExplorer::Profile*)),
            this, SLOT(profileSelectionChanged()));
    connect(ProfileManager::instance(), SIGNAL(profileUpdated(ProjectExplorer::Profile*)),
            this, SLOT(profileSelectionChanged()));

    // Set up add menu:
    connect(m_addButton, SIGNAL(clicked()), this, SLOT(addNewProfile()));
    connect(m_cloneButton, SIGNAL(clicked()), this, SLOT(cloneProfile()));
    connect(m_delButton, SIGNAL(clicked()), this, SLOT(removeProfile()));
    connect(m_makeDefaultButton, SIGNAL(clicked()), this, SLOT(makeDefaultProfile()));

    m_searchKeywords = tr("Profiles");

    updateState();

    if (m_toShow)
        m_selectionModel->select(m_model->indexOf(m_toShow),
                                 QItemSelectionModel::Clear
                                 | QItemSelectionModel::SelectCurrent
                                 | QItemSelectionModel::Rows);
    m_toShow = 0;

    return m_configWidget;
}

void ProfileOptionsPage::apply()
{
    if (m_model)
        m_model->apply();
}

void ProfileOptionsPage::finish()
{
    if (m_model) {
        delete m_model;
        m_model = 0;
    }

    m_configWidget = 0; // deleted by settingsdialog
    m_selectionModel = 0; // child of m_configWidget
    m_profilesView = 0; // child of m_configWidget
    m_currentWidget = 0; // deleted by the model
    m_toShow = 0;
}

bool ProfileOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

void ProfileOptionsPage::showProfile(Profile *p)
{
    m_toShow = p;
}

void ProfileOptionsPage::profileSelectionChanged()
{
    if (m_currentWidget)
        m_currentWidget->setVisible(false);

    QModelIndex current = currentIndex();
    m_currentWidget = current.isValid() ? m_model->widget(current) : 0;

    if (m_currentWidget)
        m_currentWidget->setVisible(true);
    updateState();
}

void ProfileOptionsPage::addNewProfile()
{
    Profile *p = new Profile;
    m_model->markForAddition(p);

    QModelIndex newIdx = m_model->indexOf(p);
    m_selectionModel->select(newIdx,
                             QItemSelectionModel::Clear
                             | QItemSelectionModel::SelectCurrent
                             | QItemSelectionModel::Rows);
}

void ProfileOptionsPage::cloneProfile()
{
    Profile *current = m_model->profile(currentIndex());
    if (!current)
        return;

    Profile *p = current->clone();

    m_model->markForAddition(p);

    QModelIndex newIdx = m_model->indexOf(p);
    m_selectionModel->select(newIdx,
                             QItemSelectionModel::Clear
                             | QItemSelectionModel::SelectCurrent
                             | QItemSelectionModel::Rows);
}

void ProfileOptionsPage::removeProfile()
{
    Profile *p = m_model->profile(currentIndex());
    if (!p)
        return;
    m_model->markForRemoval(p);
}

void ProfileOptionsPage::makeDefaultProfile()
{
    m_model->setDefaultProfile(currentIndex());
    updateState();
}

void ProfileOptionsPage::updateState()
{
    if (!m_profilesView)
        return;

    bool canCopy = false;
    bool canDelete = false;
    bool canMakeDefault = false;
    QModelIndex index = currentIndex();
    Profile *p = m_model->profile(index);
    if (p) {
        canCopy = p->isValid();
        canDelete = !p->isAutoDetected();
        canMakeDefault = !m_model->isDefaultProfile(index);
    }

    m_cloneButton->setEnabled(canCopy);
    m_delButton->setEnabled(canDelete);
    m_makeDefaultButton->setEnabled(canMakeDefault);
}

QModelIndex ProfileOptionsPage::currentIndex() const
{
    if (!m_selectionModel)
        return QModelIndex();

    QModelIndexList idxs = m_selectionModel->selectedRows();
    if (idxs.count() != 1)
        return QModelIndex();
    return idxs.at(0);
}

} // namespace ProjectExplorer
