// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidtr.h"
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

#include <qtsupport/qtkitaspect.h>

#include <utils/infolabel.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

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
public:
    NoApplicationProFilePage(CreateAndroidManifestWizard *wizard);
};

NoApplicationProFilePage::NoApplicationProFilePage(CreateAndroidManifestWizard *)
{
    auto layout = new QVBoxLayout(this);
    auto label = new QLabel(this);
    label->setWordWrap(true);
    label->setText(Tr::tr("No application .pro file found in this project."));
    layout->addWidget(label);
    setTitle(Tr::tr("No Application .pro File"));
}


//
// ChooseProFilePage
//

class ChooseProFilePage : public QWizardPage
{
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
    label->setText(Tr::tr("Select the .pro file for which you want to create the Android template files."));
    fl->addRow(label);

    BuildSystem *buildSystem = wizard->buildSystem();
    QString currentBuildKey = buildSystem->target()->activeBuildKey();

    m_comboBox = new QComboBox(this);
    for (const BuildTargetInfo &bti : buildSystem->applicationTargets()) {
        const QString displayName = QDir::toNativeSeparators(bti.buildKey);
        m_comboBox->addItem(displayName, QVariant(bti.buildKey)); // TODO something more?
        if (bti.buildKey == currentBuildKey)
            m_comboBox->setCurrentIndex(m_comboBox->count() - 1);
    }

    nodeSelected(m_comboBox->currentIndex());
    connect(m_comboBox, &QComboBox::currentIndexChanged, this, &ChooseProFilePage::nodeSelected);

    fl->addRow(Tr::tr(".pro file:"), m_comboBox);
    setTitle(Tr::tr("Select a .pro File"));
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
public:
    ChooseDirectoryPage(CreateAndroidManifestWizard *wizard);

private:
    void initializePage() final;
    bool isComplete() const final;
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
    m_layout->addRow(Tr::tr("Android package source directory:"), m_androidPackageSourceDir);

    m_sourceDirectoryWarning =
            new InfoLabel(Tr::tr("The Android package source directory cannot be the same as "
                                 "the project directory."), InfoLabel::Error, this);
    m_sourceDirectoryWarning->setVisible(false);
    m_sourceDirectoryWarning->setElideMode(Qt::ElideNone);
    m_sourceDirectoryWarning->setWordWrap(true);
    m_layout->addRow(m_sourceDirectoryWarning);

    connect(m_androidPackageSourceDir, &PathChooser::textChanged, m_wizard, [this] {
        m_wizard->setDirectory(m_androidPackageSourceDir->rawFilePath());
    });

    if (wizard->copyGradle()) {
        auto checkBox = new QCheckBox(this);
        connect(checkBox, &QCheckBox::toggled, wizard, &CreateAndroidManifestWizard::setCopyGradle);
        checkBox->setChecked(false);
        checkBox->setText(Tr::tr("Copy the Gradle files to Android directory"));
        checkBox->setToolTip(Tr::tr("It is highly recommended if you are planning to extend the Java part of your Qt application."));
        m_layout->addRow(checkBox);
    }
}

void ChooseDirectoryPage::checkPackageSourceDir()
{
    const QString buildKey = m_wizard->buildKey();
    const BuildTargetInfo bti = m_wizard->buildSystem()->buildTarget(buildKey);
    const FilePath projectDir = bti.projectFilePath.absolutePath();

    const FilePath newDir = m_androidPackageSourceDir->filePath();
    bool isComplete = projectDir.canonicalPath() != newDir.canonicalPath();

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

    FilePath androidPackageDir;
    if (const ProjectNode *node = target->project()->findNodeForBuildKey(buildKey))
        androidPackageDir = FilePath::fromVariant(node->data(Android::Constants::AndroidPackageSourceDir));

    if (androidPackageDir.isEmpty()) {
        m_label->setText(Tr::tr("Select the Android package source directory.\n\n"
                                "The files in the Android package source directory are copied to the build directory's "
                                "Android directory and the default files are overwritten."));

        const FilePath projectPath = bti.projectFilePath.isFile()
                ? bti.projectFilePath.parentDir() : bti.projectFilePath;

        m_androidPackageSourceDir->setFilePath(projectPath / "android");
        connect(m_androidPackageSourceDir, &PathChooser::rawPathChanged,
                this, &ChooseDirectoryPage::checkPackageSourceDir);
    } else {
        m_label->setText(Tr::tr("The Android template files will be created in the %1 set in the .pro "
                                "file.").arg(QLatin1String(Constants::ANDROID_PACKAGE_SOURCE_DIR)));
        m_androidPackageSourceDir->setFilePath(androidPackageDir);
        m_androidPackageSourceDir->setReadOnly(true);
    }


    m_wizard->setDirectory(m_androidPackageSourceDir->filePath());
}

//
// CreateAndroidManifestWizard
//
CreateAndroidManifestWizard::CreateAndroidManifestWizard(BuildSystem *buildSystem)
    : m_buildSystem(buildSystem)
{
    setWindowTitle(Tr::tr("Create Android Template Files Wizard"));

    const QList<BuildTargetInfo> buildTargets = buildSystem->applicationTargets();
    QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(buildSystem->kit());
    m_copyGradle = version && version->qtVersion() >= QVersionNumber(5, 4, 0);

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

void CreateAndroidManifestWizard::setDirectory(const FilePath &directory)
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

void CreateAndroidManifestWizard::createAndroidTemplateFiles()
{
    if (m_directory.isEmpty())
        return;

    FileUtils::CopyAskingForOverwrite copy(this);
    Target *target = m_buildSystem->target();
    QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(target->kit());
    if (!version)
        return;
    if (version->qtVersion() < QVersionNumber(5, 4, 0)) {
        FileUtils::copyRecursively(version->prefix() / "src/android/java/AndroidManifest.xml",
                                   m_directory / "AndroidManifest.xml",
                                   nullptr,
                                   copy);
    } else {
        FileUtils::copyRecursively(version->prefix() / "src/android/templates",
                                   m_directory,
                                   nullptr,
                                   copy);

        if (m_copyGradle) {
            FilePath gradlePath = version->prefix() / "src/3rdparty/gradle";
            QTC_ASSERT(gradlePath.exists(), return);
            FileUtils::copyRecursively(gradlePath, m_directory, nullptr, copy);
        }
    }

    QString androidPackageDir;
    ProjectNode *node = target->project()->findNodeForBuildKey(m_buildKey);
    if (node) {
        node->addFiles(copy.files());
        androidPackageDir = node->data(Android::Constants::AndroidPackageSourceDir).toString();

        if (androidPackageDir.isEmpty()) {
            // and now time for some magic
            const BuildTargetInfo bti = target->buildTarget(m_buildKey);
            const QString value = "$$PWD/"
                                  + bti.projectFilePath.toFileInfo().absoluteDir().relativeFilePath(
                                      m_directory.toString());
            bool result = node->setData(Android::Constants::AndroidPackageSourceDir, value);

            if (!result) {
                QMessageBox::warning(this,
                                     Tr::tr("Project File not Updated"),
                                     Tr::tr("Could not update the project file %1.")
                                     .arg(bti.projectFilePath.toUserOutput()));
            }
        }
    }
    Core::EditorManager::openEditor(m_directory / "AndroidManifest.xml");
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
