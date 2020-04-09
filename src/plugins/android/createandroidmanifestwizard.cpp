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

#include <android/androidconfigurations.h>
#include <android/androidconstants.h>
#include <android/androidmanager.h>

#include <coreplugin/editormanager/editormanager.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/infolabel.h>
#include <utils/pathchooser.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace Android {
namespace Internal {

//
// NoApplicationProFilePage
//

class NoApplicationProFilePage : public QWizardPage
{
    Q_DECLARE_TR_FUNCTIONS(Android::NoApplicationProFilePage)

public:
    NoApplicationProFilePage(CreateAndroidManifestWizard *wizard);
};

NoApplicationProFilePage::NoApplicationProFilePage(CreateAndroidManifestWizard *)
{
    auto layout = new QVBoxLayout(this);
    auto label = new QLabel(this);
    label->setWordWrap(true);
    label->setText(tr("No application .pro file found in this project."));
    layout->addWidget(label);
    setTitle(tr("No Application .pro File"));
}


//
// ChooseProFilePage
//

class ChooseProFilePage : public QWizardPage
{
    Q_DECLARE_TR_FUNCTIONS(Android::ChooseProfilePage)

public:
    explicit ChooseProFilePage(CreateAndroidManifestWizard *wizard);

private:
    void nodeSelected(int index);

    CreateAndroidManifestWizard *m_wizard;
    QComboBox *m_comboBox;
};

ChooseProFilePage::ChooseProFilePage(CreateAndroidManifestWizard *wizard)
    : m_wizard(wizard)
{
    auto fl = new QFormLayout(this);
    QLabel *label = new QLabel(this);
    label->setWordWrap(true);
    label->setText(tr("Select the .pro file for which you want to create the Android template files."));
    fl->addRow(label);

    BuildSystem *buildSystem = wizard->buildSystem();
    QString currentBuildKey = buildSystem->target()->activeBuildKey();

    m_comboBox = new QComboBox(this);
    for (const BuildTargetInfo &bti : buildSystem->applicationTargets()) {
        const QString displayName = bti.buildKey;
        m_comboBox->addItem(displayName, QVariant(bti.buildKey)); // TODO something more?
        if (bti.buildKey == currentBuildKey)
            m_comboBox->setCurrentIndex(m_comboBox->count() - 1);
    }

    nodeSelected(m_comboBox->currentIndex());
    connect(m_comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChooseProFilePage::nodeSelected);

    fl->addRow(tr(".pro file:"), m_comboBox);
    setTitle(tr("Select a .pro File"));
}

void ChooseProFilePage::nodeSelected(int index)
{
    Q_UNUSED(index)
    m_wizard->setBuildKey(m_comboBox->itemData(m_comboBox->currentIndex()).toString());
}


//
// ChooseDirectoryPage
//

class ChooseDirectoryPage : public QWizardPage
{
    Q_DECLARE_TR_FUNCTIONS(Android::ChooseDirectoryPage)

public:
    ChooseDirectoryPage(CreateAndroidManifestWizard *wizard);

private:
    void initializePage();
    bool isComplete() const;
    void checkPackageSourceDir();

    CreateAndroidManifestWizard *m_wizard;
    PathChooser *m_androidPackageSourceDir = nullptr;
    InfoLabel *m_sourceDirectoryWarning = nullptr;
    QLabel *m_label;
    QFormLayout *m_layout;
    bool m_complete = true;
};

ChooseDirectoryPage::ChooseDirectoryPage(CreateAndroidManifestWizard *wizard)
    : m_wizard(wizard)
{
    m_layout = new QFormLayout(this);
    m_label = new QLabel(this);
    m_label->setWordWrap(true);
    m_layout->addRow(m_label);

    m_androidPackageSourceDir = new PathChooser(this);
    m_androidPackageSourceDir->setExpectedKind(PathChooser::Directory);
    m_layout->addRow(tr("Android package source directory:"), m_androidPackageSourceDir);

    m_sourceDirectoryWarning =
            new InfoLabel(tr("The Android package source directory cannot be the same as "
                             "the project directory."), InfoLabel::Error, this);
    m_sourceDirectoryWarning->setVisible(false);
    m_sourceDirectoryWarning->setElideMode(Qt::ElideNone);
    m_sourceDirectoryWarning->setWordWrap(true);
    m_layout->addRow(m_sourceDirectoryWarning);

    connect(m_androidPackageSourceDir, &PathChooser::pathChanged,
            m_wizard, &CreateAndroidManifestWizard::setDirectory);

    if (wizard->copyGradle()) {
        auto checkBox = new QCheckBox(this);
        checkBox->setChecked(true);
        connect(checkBox, &QCheckBox::toggled, wizard, &CreateAndroidManifestWizard::setCopyGradle);
        checkBox->setText(tr("Copy the Gradle files to Android directory"));
        checkBox->setToolTip(tr("It is highly recommended if you are planning to extend the Java part of your Qt application."));
        m_layout->addRow(checkBox);
    }
}

void ChooseDirectoryPage::checkPackageSourceDir()
{
    const QString buildKey = m_wizard->buildKey();
    const BuildTargetInfo bti = m_wizard->buildSystem()->buildTarget(buildKey);
    const QString projectDir = bti.projectFilePath.toFileInfo().absolutePath();

    const QString newDir = m_androidPackageSourceDir->filePath().toString();
    bool isComplete = QFileInfo(projectDir) != QFileInfo(newDir);

    m_sourceDirectoryWarning->setVisible(!isComplete);

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
    const Target *target = m_wizard->buildSystem()->target();
    const QString buildKey = m_wizard->buildKey();
    const BuildTargetInfo bti = target->buildTarget(buildKey);
    const QString projectDir = bti.projectFilePath.toFileInfo().absolutePath();

    QString androidPackageDir;
    if (const ProjectNode *node = target->project()->findNodeForBuildKey(buildKey))
        androidPackageDir = node->data(Android::Constants::AndroidPackageSourceDir).toString();

    if (androidPackageDir.isEmpty()) {
        m_label->setText(tr("Select the Android package source directory.\n\n"
                          "The files in the Android package source directory are copied to the build directory's "
                          "Android directory and the default files are overwritten."));

        m_androidPackageSourceDir->setPath(projectDir + "/android");
        connect(m_androidPackageSourceDir, &PathChooser::rawPathChanged,
                this, &ChooseDirectoryPage::checkPackageSourceDir);
    } else {
        m_label->setText(tr("The Android template files will be created in the ANDROID_PACKAGE_SOURCE_DIR set in the .pro file."));
        m_androidPackageSourceDir->setPath(androidPackageDir);
        m_androidPackageSourceDir->setReadOnly(true);
    }


    m_wizard->setDirectory(m_androidPackageSourceDir->filePath().toString());
}

//
// CreateAndroidManifestWizard
//
CreateAndroidManifestWizard::CreateAndroidManifestWizard(BuildSystem *buildSystem)
    : m_buildSystem(buildSystem), m_copyState(Ask)
{
    setWindowTitle(tr("Create Android Template Files Wizard"));

    const QList<BuildTargetInfo> buildTargets = buildSystem->applicationTargets();
    QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(buildSystem->kit());
    m_copyGradle = version && version->qtVersion() >= QtSupport::QtVersionNumber(5, 4, 0);

    if (buildTargets.isEmpty()) {
        // oh uhm can't create anything
        addPage(new NoApplicationProFilePage(this));
    } else if (buildTargets.size() == 1) {
        setBuildKey(buildTargets.first().buildKey);
        addPage(new ChooseDirectoryPage(this));
    } else {
        addPage(new ChooseProFilePage(this));
        addPage(new ChooseDirectoryPage(this));
    }
}

QString CreateAndroidManifestWizard::buildKey() const
{
    return m_buildKey;
}

void CreateAndroidManifestWizard::setBuildKey(const QString &buildKey)
{
    m_buildKey = buildKey;
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
    Target *target = m_buildSystem->target();
    QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(target->kit());
    if (!version)
        return;
    if (version->qtVersion() < QtSupport::QtVersionNumber(5, 4, 0)) {
        const QString src = version->prefix().toString() + "/src/android/java/AndroidManifest.xml";
        FileUtils::copyRecursively(FilePath::fromString(src),
                                   FilePath::fromString(m_directory + QLatin1String("/AndroidManifest.xml")),
                                   nullptr, [this, &addedFiles](QFileInfo src, QFileInfo dst, QString *){return copy(src, dst, &addedFiles);});
    } else {
        const QString src = version->prefix().toString() + "/src/android/templates";

        FileUtils::copyRecursively(FilePath::fromString(src),
                                   FilePath::fromString(m_directory),
                                   nullptr, [this, &addedFiles](QFileInfo src, QFileInfo dst, QString *){return copy(src, dst, &addedFiles);});

        if (m_copyGradle) {
            FilePath gradlePath = version->prefix().pathAppended("src/3rdparty/gradle");
            if (!gradlePath.exists())
                gradlePath = AndroidConfigurations::currentConfig().sdkLocation().pathAppended("/tools/templates/gradle/wrapper");
            FileUtils::copyRecursively(gradlePath, FilePath::fromString(m_directory),
                                       nullptr, [this, &addedFiles](QFileInfo src, QFileInfo dst, QString *){return copy(src, dst, &addedFiles);});
        }

        AndroidManager::updateGradleProperties(target, m_buildKey);
    }


    QString androidPackageDir;
    ProjectNode *node = target->project()->findNodeForBuildKey(m_buildKey);
    if (node) {
        node->addFiles(addedFiles);
        androidPackageDir = node->data(Android::Constants::AndroidPackageSourceDir).toString();

        if (androidPackageDir.isEmpty()) {
            // and now time for some magic
            const BuildTargetInfo bti = target->buildTarget(m_buildKey);
            const QString value = "$$PWD/"
                                  + bti.projectFilePath.toFileInfo().absoluteDir().relativeFilePath(
                                      m_directory);
            bool result = node->setData(Android::Constants::AndroidPackageSourceDir, value);

            if (!result) {
                QMessageBox::warning(this,
                                     tr("Project File not Updated"),
                                     tr("Could not update the project file %1.")
                                         .arg(bti.projectFilePath.toUserOutput()));
            }
        }
    }
    Core::EditorManager::openEditor(m_directory + QLatin1String("/AndroidManifest.xml"));
}

BuildSystem *CreateAndroidManifestWizard::buildSystem() const
{
    return m_buildSystem;
}

void CreateAndroidManifestWizard::accept()
{
    createAndroidTemplateFiles();
    Wizard::accept();
}

} // namespace Internal
} // namespace Android
