/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include "qmakeandroidrunconfiguration.h"

#include <android/androidconfigurations.h>
#include <android/androidmanager.h>
#include <android/androidqtsupport.h>

#include <coreplugin/editormanager/editormanager.h>

#include <projectexplorer/target.h>

#include <qmakeprojectmanager/qmakeproject.h>

#include <proparser/prowriter.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

using namespace Android;
using namespace QmakeAndroidSupport::Internal;
using namespace Utils;

using QmakeProjectManager::QmakeProject;
using QmakeProjectManager::QmakeProFile;

//
// NoApplicationProFilePage
//
NoApplicationProFilePage::NoApplicationProFilePage(CreateAndroidManifestWizard *wizard)
    : m_wizard(wizard)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel(this);
    label->setWordWrap(true);
    label->setText(tr("No application .pro file found in this project."));
    layout->addWidget(label);
    setTitle(tr("No Application .pro File"));
}

//
// ChooseProFilePage
//
ChooseProFilePage::ChooseProFilePage(CreateAndroidManifestWizard *wizard,
                                     const QList<QmakeProFile *> &files,
                                     const QmakeProFile *select) :
    m_wizard(wizard)
{
    QFormLayout *fl = new QFormLayout(this);
    QLabel *label = new QLabel(this);
    label->setWordWrap(true);
    label->setText(tr("Select the .pro file for which you want to create the Android template files."));
    fl->addRow(label);

    m_comboBox = new QComboBox(this);
    for (QmakeProFile *file : files) {
        m_comboBox->addItem(file->displayName(), QVariant::fromValue(static_cast<void *>(file))); // TODO something more?
        if (file == select)
            m_comboBox->setCurrentIndex(m_comboBox->count() - 1);
    }

    nodeSelected(m_comboBox->currentIndex());
    connect(m_comboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ChooseProFilePage::nodeSelected);

    fl->addRow(tr(".pro file:"), m_comboBox);
    setTitle(tr("Select a .pro File"));
}

void ChooseProFilePage::nodeSelected(int index)
{
    Q_UNUSED(index)
    m_wizard->setProFile(static_cast<QmakeProFile *>(m_comboBox->itemData(m_comboBox->currentIndex()).value<void *>()));
}


//
// ChooseDirectoryPage
//
ChooseDirectoryPage::ChooseDirectoryPage(CreateAndroidManifestWizard *wizard)
    : m_wizard(wizard), m_androidPackageSourceDir(0), m_complete(true)
{
    m_layout = new QFormLayout(this);
    m_label = new QLabel(this);
    m_label->setWordWrap(true);
    m_layout->addRow(m_label);

    m_androidPackageSourceDir = new PathChooser(this);
    m_androidPackageSourceDir->setExpectedKind(PathChooser::Directory);
    m_layout->addRow(tr("Android package source directory:"), m_androidPackageSourceDir);

    m_sourceDirectoryWarning = new QLabel(this);
    m_sourceDirectoryWarning->setVisible(false);
    m_sourceDirectoryWarning->setText(tr("The Android package source directory cannot be the same as the project directory."));
    m_sourceDirectoryWarning->setWordWrap(true);
    m_warningIcon = new QLabel(this);
    m_warningIcon->setVisible(false);
    m_warningIcon->setPixmap(Utils::Icons::CRITICAL.pixmap());
    m_warningIcon->setWordWrap(true);
    m_warningIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addWidget(m_warningIcon);
    hbox->addWidget(m_sourceDirectoryWarning);
    hbox->setAlignment(m_warningIcon, Qt::AlignTop);

    m_layout->addRow(hbox);

    connect(m_androidPackageSourceDir, &PathChooser::pathChanged,
            m_wizard, &CreateAndroidManifestWizard::setDirectory);

    if (wizard->copyGradle()) {
        QCheckBox *checkBox = new QCheckBox(this);
        checkBox->setChecked(true);
        connect(checkBox, &QCheckBox::toggled, wizard, &CreateAndroidManifestWizard::setCopyGradle);
        checkBox->setText(tr("Copy the Gradle files to Android directory"));
        checkBox->setToolTip(tr("It is highly recommended if you are planning to extend the Java part of your Qt application."));
        m_layout->addRow(checkBox);
    }
}

void ChooseDirectoryPage::checkPackageSourceDir()
{
    QString projectDir = m_wizard->proFile()->filePath().toFileInfo().absolutePath();
    QString newDir = m_androidPackageSourceDir->path();
    bool isComplete = QFileInfo(projectDir) != QFileInfo(newDir);

    m_sourceDirectoryWarning->setVisible(!isComplete);
    m_warningIcon->setVisible(!isComplete);

    if (isComplete != m_complete) {
        m_complete = isComplete;
        emit completeChanged();
    }
}

bool ChooseDirectoryPage::isComplete() const
{
    return m_complete;
}

void ChooseDirectoryPage::initializePage()
{
    QString androidPackageDir = m_wizard->proFile()->singleVariableValue(QmakeProjectManager::Variable::AndroidPackageSourceDir);
    if (androidPackageDir.isEmpty()) {
        m_label->setText(tr("Select the Android package source directory.\n\n"
                          "The files in the Android package source directory are copied to the build directory's "
                          "Android directory and the default files are overwritten."));

        m_androidPackageSourceDir->setPath(m_wizard->proFile()->filePath().toFileInfo().absolutePath().append(QLatin1String("/android")));
        connect(m_androidPackageSourceDir, &PathChooser::rawPathChanged,
                this, &ChooseDirectoryPage::checkPackageSourceDir);
    } else {
        m_label->setText(tr("The Android template files will be created in the ANDROID_PACKAGE_SOURCE_DIR set in the .pro file."));
        m_androidPackageSourceDir->setPath(androidPackageDir);
        m_androidPackageSourceDir->setReadOnly(true);
    }


    m_wizard->setDirectory(m_androidPackageSourceDir->path());
}

//
// CreateAndroidManifestWizard
//
CreateAndroidManifestWizard::CreateAndroidManifestWizard(ProjectExplorer::Target *target)
    : m_target(target), m_proFile(0), m_copyState(Ask)
{
    setWindowTitle(tr("Create Android Template Files Wizard"));

    QmakeProject *project = static_cast<QmakeProject *>(target->project());
    QList<QmakeProFile *> files = project->applicationProFiles();
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target->kit());
    m_copyGradle = version && version->qtVersion() >= QtSupport::QtVersionNumber(5, 4, 0);

    const QmakeProFile *currentRunNode = nullptr;
    ProjectExplorer::RunConfiguration *rc = target->activeRunConfiguration();
    if (auto qrc = qobject_cast<QmakeAndroidRunConfiguration *>(rc))
        currentRunNode = project->rootProFile()->findProFile(qrc->proFilePath());

    if (files.isEmpty()) {
        // oh uhm can't create anything
        addPage(new NoApplicationProFilePage(this));
    } else if (files.size() == 1) {
        setProFile(files.first());
        addPage(new ChooseDirectoryPage(this));
    } else {
        addPage(new ChooseProFilePage(this, files, currentRunNode));
        addPage(new ChooseDirectoryPage(this));
    }
}

QmakeProjectManager::QmakeProFile *CreateAndroidManifestWizard::proFile() const
{
    return m_proFile;
}

void CreateAndroidManifestWizard::setProFile(QmakeProjectManager::QmakeProFile *node)
{
    m_proFile = node;
}

void CreateAndroidManifestWizard::setDirectory(const QString &directory)
{
    m_directory = directory;
}

bool CreateAndroidManifestWizard::copyGradle()
{
    return m_copyGradle;
}

void CreateAndroidManifestWizard::setCopyGradle(bool copy)
{
    m_copyGradle = copy;
}

bool CreateAndroidManifestWizard::copy(const QFileInfo &src, const QFileInfo &dst, QStringList * addedFiles)
{
    bool copyFile = true;
    if (dst.exists()) {
        switch (m_copyState) {
        case Ask:
            {
                int res = QMessageBox::question(this,
                                                tr("Overwrite %1 file").arg(dst.fileName()),
                                                tr("Overwrite existing \"%1\"?")
                                                    .arg(QDir(m_directory).relativeFilePath(dst.absoluteFilePath())),
                                                QMessageBox::Yes | QMessageBox::YesToAll |
                                                QMessageBox::No | QMessageBox::NoToAll |
                                                QMessageBox::Cancel);
                switch (res) {
                case QMessageBox::YesToAll:
                    m_copyState = OverwriteAll;
                    break;

                case QMessageBox::Yes:
                    break;

                case QMessageBox::NoToAll:
                    m_copyState = SkipAll;
                    copyFile = false;
                    break;

                case QMessageBox::No:
                    copyFile = false;
                    break;
                default:
                    return false;
                }
            }
            break;
        case SkipAll:
            copyFile = false;
            break;
        default:
            break;
        }
        if (copyFile)
            QFile::remove(dst.filePath());
    }

    if (!dst.absoluteDir().exists())
        dst.absoluteDir().mkpath(dst.absolutePath());

    if (copyFile && !QFile::copy(src.filePath(), dst.filePath())) {
        QMessageBox::warning(this, tr("File Creation Error"),
                             tr("Could not copy file \"%1\" to \"%2\".")
                                .arg(src.filePath()).arg(dst.filePath()));
        return false;
    }
    addedFiles->append(dst.absoluteFilePath());
    return true;
}

void CreateAndroidManifestWizard::createAndroidTemplateFiles()
{
    if (m_directory.isEmpty())
        return;

    QStringList addedFiles;
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(m_target->kit());
    if (!version)
        return;
    if (version->qtVersion() < QtSupport::QtVersionNumber(5, 4, 0)) {
        const QString src(version->qmakeProperty("QT_INSTALL_PREFIX")
                .append(QLatin1String("/src/android/java/AndroidManifest.xml")));
        FileUtils::copyRecursively(FileName::fromString(src),
                                   FileName::fromString(m_directory + QLatin1String("/AndroidManifest.xml")),
                                   0, [this, &addedFiles](QFileInfo src, QFileInfo dst, QString *){return copy(src, dst, &addedFiles);});
    } else {
        const QString src(version->qmakeProperty("QT_INSTALL_PREFIX")
                          .append(QLatin1String("/src/android/templates")));

        FileUtils::copyRecursively(FileName::fromString(src),
                                   FileName::fromString(m_directory),
                                   0, [this, &addedFiles](QFileInfo src, QFileInfo dst, QString *){return copy(src, dst, &addedFiles);});

        if (m_copyGradle)
            FileUtils::copyRecursively(AndroidConfigurations::currentConfig().sdkLocation().appendPath(QLatin1String("/tools/templates/gradle/wrapper")),
                                       FileName::fromString(m_directory),
                                       0, [this, &addedFiles](QFileInfo src, QFileInfo dst, QString *){return copy(src, dst, &addedFiles);});

        AndroidManager::updateGradleProperties(m_target);
    }
    m_proFile->addFiles(addedFiles);

    if (m_proFile->singleVariableValue(QmakeProjectManager::Variable::AndroidPackageSourceDir).isEmpty()) {
        // and now time for some magic
        QString value = QLatin1String("$$PWD/")
                + m_proFile->filePath().toFileInfo().absoluteDir().relativeFilePath(m_directory);
        bool result =
                m_proFile->setProVariable(QLatin1String("ANDROID_PACKAGE_SOURCE_DIR"), QStringList(value));

        if (!result) {
            QMessageBox::warning(this, tr("Project File not Updated"),
                                 tr("Could not update the .pro file %1.")
                                 .arg(m_proFile->filePath().toUserOutput()));
        }
    }
    Core::EditorManager::openEditor(m_directory + QLatin1String("/AndroidManifest.xml"));
}

void CreateAndroidManifestWizard::accept()
{
    createAndroidTemplateFiles();
    Wizard::accept();
}
