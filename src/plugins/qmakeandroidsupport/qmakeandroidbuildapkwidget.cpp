/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "createandroidmanifestwizard.h"
#include "qmakeandroidbuildapkstep.h"
#include "qmakeandroidbuildapkwidget.h"
#include "ui_qmakeandroidbuildapkwidget.h"

#include <android/androidbuildapkwidget.h>
#include <android/androidmanager.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <qmakeprojectmanager/qmakeproject.h>

#include <QFileDialog>

using QmakeProjectManager::QmakeProject;
using QmakeProjectManager::QmakeProFileNode;

namespace QmakeAndroidSupport {
namespace Internal {

QmakeAndroidBuildApkWidget::QmakeAndroidBuildApkWidget(QmakeAndroidBuildApkStep *step) :
    ProjectExplorer::BuildStepConfigWidget(),
    m_ui(new Ui::QmakeAndroidBuildApkWidget),
    m_step(step),
    m_extraLibraryListModel(0),
    m_ignoreChange(false)
{
    QVBoxLayout *topLayout = new QVBoxLayout;

    QHBoxLayout *qt51WarningLayout = new QHBoxLayout();
    QLabel *oldFilesWarningIcon = new QLabel(this);
    oldFilesWarningIcon->setObjectName(QStringLiteral("oldFilesWarningIcon"));
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(oldFilesWarningIcon->sizePolicy().hasHeightForWidth());
    oldFilesWarningIcon->setSizePolicy(sizePolicy);
    oldFilesWarningIcon->setPixmap(QPixmap(QLatin1String(":/core/images/warning.png")));
    oldFilesWarningIcon->setAlignment(Qt::Alignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop));
    qt51WarningLayout->addWidget(oldFilesWarningIcon);

    QLabel *oldFilesWarningLabel = new QLabel(this);
    oldFilesWarningLabel->setObjectName(QStringLiteral("oldFilesWarningLabel"));
    oldFilesWarningLabel->setWordWrap(true);
    qt51WarningLayout->addWidget(oldFilesWarningLabel);

    topLayout->addWidget(new Android::AndroidBuildApkWidget(step));

    QWidget *widget = new QWidget(this);
    m_ui->setupUi(widget);
    topLayout->addWidget(widget);
    setLayout(topLayout);

    bool oldFiles = Android::AndroidManager::checkForQt51Files(m_step->project()->projectDirectory());
    oldFilesWarningIcon->setVisible(oldFiles);
    oldFilesWarningLabel->setVisible(oldFiles);

    m_extraLibraryListModel = new AndroidExtraLibraryListModel(static_cast<QmakeProject *>(m_step->project()), this);
    m_ui->androidExtraLibsListView->setModel(m_extraLibraryListModel);

    updateInputFileUi();
    connect(m_step, SIGNAL(inputFileChanged()),
            SLOT(updateInputFileUi()));

    connect(m_ui->inputFileComboBox, SIGNAL(currentIndexChanged(int)),
            SLOT(inputFileComboBoxIndexChanged()));

    connect(m_ui->createAndroidManifestButton, SIGNAL(clicked()),
            SLOT(createManifestButton()));

    connect(m_ui->addAndroidExtraLibButton, SIGNAL(clicked()),
            SLOT(addAndroidExtraLib()));

    connect(m_ui->removeAndroidExtraLibButton, SIGNAL(clicked()),
            SLOT(removeAndroidExtraLib()));

    connect(m_ui->androidExtraLibsListView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            SLOT(checkEnableRemoveButton()));

    connect(m_extraLibraryListModel, SIGNAL(enabledChanged(bool)),
            m_ui->additionalLibrariesGroupBox, SLOT(setEnabled(bool)));

    m_ui->additionalLibrariesGroupBox->setEnabled(m_extraLibraryListModel->isEnabled());
}

QmakeAndroidBuildApkWidget::~QmakeAndroidBuildApkWidget()
{
    delete m_ui;
}

void QmakeAndroidBuildApkWidget::updateInputFileUi()
{
    QmakeProject *project
            = static_cast<QmakeProject *>(m_step->project());
    QList<QmakeProFileNode *> nodes = project->applicationProFiles();
    int size = nodes.size();
    if (size == 0 || size == 1) {
        // there's nothing to select, e.g. before parsing
        m_ui->inputFileLabel->setVisible(false);
        m_ui->inputFileComboBox->setVisible(false);
    } else {
        m_ignoreChange = true;
        m_ui->inputFileLabel->setVisible(true);
        m_ui->inputFileComboBox->setVisible(true);

        m_ui->inputFileComboBox->clear();
        foreach (QmakeProFileNode *node, nodes)
            m_ui->inputFileComboBox->addItem(node->displayName(), node->path());

        int index = m_ui->inputFileComboBox->findData(m_step->proFilePathForInputFile());
        m_ui->inputFileComboBox->setCurrentIndex(index);
        m_ignoreChange = false;
    }
}

void QmakeAndroidBuildApkWidget::inputFileComboBoxIndexChanged()
{
    if (m_ignoreChange)
        return;

    QString proFilePath = m_ui->inputFileComboBox->itemData(m_ui->inputFileComboBox->currentIndex()).toString();
    m_step->setProFilePathForInputFile(proFilePath);
}

void QmakeAndroidBuildApkWidget::createManifestButton()
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
    QModelIndexList removeList = m_ui->androidExtraLibsListView->selectionModel()->selectedIndexes();
    m_extraLibraryListModel->removeEntries(removeList);
}

void QmakeAndroidBuildApkWidget::checkEnableRemoveButton()
{
    m_ui->removeAndroidExtraLibButton->setEnabled(m_ui->androidExtraLibsListView->selectionModel()->hasSelection());
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
} // namespace QmakeProjectManager


