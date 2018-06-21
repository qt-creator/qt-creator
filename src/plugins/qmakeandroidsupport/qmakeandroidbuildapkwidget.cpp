/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "createandroidmanifestwizard.h"
#include "qmakeandroidbuildapkwidget.h"

#include <android/androidbuildapkwidget.h>
#include <android/androidmanager.h>

#include <projectexplorer/project.h>

#include <utils/utilsicons.h>

#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

namespace QmakeAndroidSupport {
namespace Internal {

QmakeAndroidBuildApkWidget::QmakeAndroidBuildApkWidget(Android::AndroidBuildApkStep *step) :
    m_step(step)
{
    m_extraLibraryListModel = new AndroidExtraLibraryListModel(m_step->target(), this);

    auto base = new Android::AndroidBuildApkWidget(step);
    base->layout()->setContentsMargins(0, 0, 0, 0);

    auto createTemplatesGroupBox = new QGroupBox(tr("Android"));
    createTemplatesGroupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto createAndroidTemplatesButton = new QPushButton(tr("Create Templates"));

    auto horizontalLayout = new QHBoxLayout(createTemplatesGroupBox);
    horizontalLayout->addWidget(createAndroidTemplatesButton);
    horizontalLayout->addStretch(1);

    auto additionalLibrariesGroupBox = new QGroupBox(tr("Additional Libraries"));
    additionalLibrariesGroupBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    m_androidExtraLibsListView = new QListView;
    m_androidExtraLibsListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_androidExtraLibsListView->setToolTip(tr("List of extra libraries to include in Android package and load on startup."));
    m_androidExtraLibsListView->setModel(m_extraLibraryListModel);

    auto addAndroidExtraLibButton = new QToolButton;
    addAndroidExtraLibButton->setText(tr("Add..."));
    addAndroidExtraLibButton->setToolTip(tr("Select library to include in package."));
    addAndroidExtraLibButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    addAndroidExtraLibButton->setToolButtonStyle(Qt::ToolButtonTextOnly);

    m_removeAndroidExtraLibButton = new QToolButton;
    m_removeAndroidExtraLibButton->setText(tr("Remove"));
    m_removeAndroidExtraLibButton->setToolTip(tr("Remove currently selected library from list."));

    auto androidExtraLibsButtonLayout = new QVBoxLayout();
    androidExtraLibsButtonLayout->addWidget(addAndroidExtraLibButton);
    androidExtraLibsButtonLayout->addWidget(m_removeAndroidExtraLibButton);
    androidExtraLibsButtonLayout->addStretch(1);

    auto androidExtraLibsLayout = new QHBoxLayout(additionalLibrariesGroupBox);
    androidExtraLibsLayout->addWidget(m_androidExtraLibsListView);
    androidExtraLibsLayout->addLayout(androidExtraLibsButtonLayout);

    auto topLayout = new QVBoxLayout(this);
    topLayout->addWidget(base);
    topLayout->addWidget(createTemplatesGroupBox);
    topLayout->addWidget(additionalLibrariesGroupBox);

    connect(createAndroidTemplatesButton, &QAbstractButton::clicked,
            this, &QmakeAndroidBuildApkWidget::createAndroidTemplatesButton);

    connect(addAndroidExtraLibButton, &QAbstractButton::clicked,
            this, &QmakeAndroidBuildApkWidget::addAndroidExtraLib);

    connect(m_removeAndroidExtraLibButton, &QAbstractButton::clicked,
            this, &QmakeAndroidBuildApkWidget::removeAndroidExtraLib);

    connect(m_androidExtraLibsListView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &QmakeAndroidBuildApkWidget::checkEnableRemoveButton);

    connect(m_extraLibraryListModel, &AndroidExtraLibraryListModel::enabledChanged,
            additionalLibrariesGroupBox, &QWidget::setEnabled);

    additionalLibrariesGroupBox->setEnabled(m_extraLibraryListModel->isEnabled());
}

void QmakeAndroidBuildApkWidget::createAndroidTemplatesButton()
{
    CreateAndroidManifestWizard wizard(m_step->target());
    wizard.exec();
}

void QmakeAndroidBuildApkWidget::addAndroidExtraLib()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this,
                                                          tr("Select additional libraries"),
                                                          QDir::homePath(),
                                                          tr("Libraries (*.so)"));

    if (!fileNames.isEmpty())
        m_extraLibraryListModel->addEntries(fileNames);
}

void QmakeAndroidBuildApkWidget::removeAndroidExtraLib()
{
    QModelIndexList removeList = m_androidExtraLibsListView->selectionModel()->selectedIndexes();
    m_extraLibraryListModel->removeEntries(removeList);
}

void QmakeAndroidBuildApkWidget::checkEnableRemoveButton()
{
    m_removeAndroidExtraLibButton->setEnabled(m_androidExtraLibsListView->selectionModel()->hasSelection());
}

QString QmakeAndroidBuildApkWidget::summaryText() const
{
    return tr("<b>Build Android APK</b>");
}

QString QmakeAndroidBuildApkWidget::displayName() const
{
    return summaryText();
}

} // namespace Internal
} // namespace QmakeAndroidSupport


