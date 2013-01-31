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

#include "cmakeopenprojectwizard.h"
#include "cmakeprojectmanager.h"
#include "cmakebuildconfiguration.h"

#include <coreplugin/icore.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>
#include <utils/fancylineedit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/abi.h>
#include <texteditor/fontsettings.h>

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QDateTime>
#include <QSettings>
#include <QStringList>
#include <QApplication>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

///////
//  Page Flow:
//   Start (No .user file)
//    |
//    |---> In Source Build --> Page: Tell the user about that
//                               |--> Already existing cbp file (and new enough) --> Page: Ready to load the project
//                               |--> Page: Ask for cmd options, run generator
//    |---> No in source Build --> Page: Ask the user for the build directory
//                                   |--> Already existing cbp file (and new enough) --> Page: Ready to load the project
//                                   |--> Page: Ask for cmd options, run generator


namespace CMakeProjectManager {
namespace Internal {
    class GeneratorInfo
    {
        Q_DECLARE_TR_FUNCTIONS(CMakeProjectManager::Internal::GeneratorInfo)
    public:
        enum Ninja { NoNinja, OfferNinja, ForceNinja };
        static QList<GeneratorInfo> generatorInfosFor(ProjectExplorer::Kit *k, Ninja n, bool hasCodeBlocks);

        GeneratorInfo();
        explicit GeneratorInfo(ProjectExplorer::Kit *kit, bool ninja = false);

        ProjectExplorer::Kit *kit() const;
        bool isNinja() const;

        QString displayName() const;
        QByteArray generatorArgument() const;
        QByteArray generator() const;

    private:
        ProjectExplorer::Kit *m_kit;
        bool m_isNinja;
    };

}
}

Q_DECLARE_METATYPE(CMakeProjectManager::Internal::GeneratorInfo);

GeneratorInfo::GeneratorInfo()
    : m_kit(0), m_isNinja(false)
{}

GeneratorInfo::GeneratorInfo(ProjectExplorer::Kit *kit, bool ninja)
    : m_kit(kit), m_isNinja(ninja)
{}

ProjectExplorer::Kit *GeneratorInfo::kit() const
{
    return m_kit;
}

bool GeneratorInfo::isNinja() const {
    return m_isNinja;
}

QByteArray GeneratorInfo::generator() const
{
    if (!m_kit)
        return QByteArray();
    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(m_kit);
    ProjectExplorer::Abi targetAbi = tc->targetAbi();
    if (m_isNinja) {
        return "Ninja";
    } else if (targetAbi.os() == ProjectExplorer::Abi::WindowsOS) {
        if (targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2005Flavor
                || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2008Flavor
                || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2010Flavor
                || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2012Flavor) {
            return "NMake Makefiles";
        } else if (targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMSysFlavor) {
            if (Utils::HostOsInfo::isWindowsHost())
                return "MinGW Makefiles";
            else
                return "Unix Makefiles";
        }
    }
    return "Unix Makefiles";
}

QByteArray GeneratorInfo::generatorArgument() const
{
    QByteArray tmp = generator();
    if (tmp.isEmpty())
        return tmp;
    return QByteArray("-GCodeBlocks - ") + tmp;
}

QString GeneratorInfo::displayName() const
{
    if (!m_kit)
        return QString();
    if (m_isNinja)
        return tr("Ninja (%1)").arg(m_kit->displayName());
    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(m_kit);
    ProjectExplorer::Abi targetAbi = tc->targetAbi();
    if (targetAbi.os() == ProjectExplorer::Abi::WindowsOS) {
        if (targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2005Flavor
                || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2008Flavor
                || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2010Flavor
                || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2012Flavor) {
            return tr("NMake Generator (%1)").arg(m_kit->displayName());
        } else if (targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMSysFlavor) {
            if (Utils::HostOsInfo::isWindowsHost())
                return tr("MinGW Generator (%1)").arg(m_kit->displayName());
            else
                return tr("Unix Generator (%1)").arg(m_kit->displayName());
        }
    } else {
        // Non windows
        return tr("Unix Generator (%1)").arg(m_kit->displayName());
    }
    return QString();
}

QList<GeneratorInfo> GeneratorInfo::generatorInfosFor(ProjectExplorer::Kit *k, Ninja n, bool hasCodeBlocks)
{
    QList<GeneratorInfo> results;
    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);
    if (!tc)
        return results;
    ProjectExplorer::Abi targetAbi = tc->targetAbi();
    if (n != ForceNinja) {
        if (targetAbi.os() == ProjectExplorer::Abi::WindowsOS) {
            if (targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2005Flavor
                    || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2008Flavor
                    || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2010Flavor
                    || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2012Flavor) {
                if (hasCodeBlocks)
                    results << GeneratorInfo(k);
            } else if (targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMSysFlavor) {
                results << GeneratorInfo(k);
            }
        } else {
            // Non windows
            results << GeneratorInfo(k);
        }
    }
    if (n != NoNinja)
        results << GeneratorInfo(k, true);
    return results;
}

//////////////
/// CMakeOpenProjectWizard
//////////////

CMakeOpenProjectWizard::CMakeOpenProjectWizard(CMakeManager *cmakeManager, const QString &sourceDirectory, Utils::Environment env)
    : m_cmakeManager(cmakeManager),
      m_sourceDirectory(sourceDirectory),
      m_environment(env),
      m_useNinja(false),
      m_kit(0)
{
    if (hasInSourceBuild()) {
        m_buildDirectory = m_sourceDirectory;
        addPage(new InSourceBuildPage(this));
    } else {
        m_buildDirectory = m_sourceDirectory + QLatin1String("-build");
        addPage(new ShadowBuildPage(this));
    }

    if (!m_cmakeManager->isCMakeExecutableValid())
        addPage(new ChooseCMakePage(this));

    addPage(new CMakeRunPage(this));

    init();
}

CMakeOpenProjectWizard::CMakeOpenProjectWizard(CMakeManager *cmakeManager, CMakeOpenProjectWizard::Mode mode,
                                               const BuildInfo &info)
    : m_cmakeManager(cmakeManager),
      m_sourceDirectory(info.sourceDirectory),
      m_environment(info.environment),
      m_useNinja(info.useNinja),
      m_kit(info.kit)
{
    CMakeRunPage::Mode rmode;
    if (mode == CMakeOpenProjectWizard::NeedToCreate)
        rmode = CMakeRunPage::Recreate;
    else if (mode == CMakeOpenProjectWizard::WantToUpdate)
        rmode = CMakeRunPage::WantToUpdate;
    else if (mode == CMakeOpenProjectWizard::NeedToUpdate)
        rmode = CMakeRunPage::NeedToUpdate;
    else
        rmode = CMakeRunPage::ChangeDirectory;

    if (mode == CMakeOpenProjectWizard::ChangeDirectory) {
        m_buildDirectory = info.buildDirectory;
        addPage(new ShadowBuildPage(this, true));
    }
    if (!m_cmakeManager->isCMakeExecutableValid())
        addPage(new ChooseCMakePage(this));

    addPage(new CMakeRunPage(this, rmode, info.buildDirectory));
    init();
}

void CMakeOpenProjectWizard::init()
{
    setOption(QWizard::NoBackButtonOnStartPage);
    setWindowTitle(tr("CMake Wizard"));
}

CMakeManager *CMakeOpenProjectWizard::cmakeManager() const
{
    return m_cmakeManager;
}

bool CMakeOpenProjectWizard::hasInSourceBuild() const
{
    QFileInfo fi(m_sourceDirectory + QLatin1String("/CMakeCache.txt"));
    if (fi.exists())
        return true;
    return false;
}

bool CMakeOpenProjectWizard::existsUpToDateXmlFile() const
{
    QString cbpFile = CMakeManager::findCbpFile(QDir(buildDirectory()));
    if (!cbpFile.isEmpty()) {
        // We already have a cbp file
        QFileInfo cbpFileInfo(cbpFile);
        QFileInfo cmakeListsFileInfo(sourceDirectory() + QLatin1String("/CMakeLists.txt"));

        if (cbpFileInfo.lastModified() > cmakeListsFileInfo.lastModified())
            return true;
    }
    return false;
}

QString CMakeOpenProjectWizard::buildDirectory() const
{
    return m_buildDirectory;
}

QString CMakeOpenProjectWizard::sourceDirectory() const
{
    return m_sourceDirectory;
}

void CMakeOpenProjectWizard::setBuildDirectory(const QString &directory)
{
    m_buildDirectory = directory;
}

bool CMakeOpenProjectWizard::useNinja() const
{
    return m_useNinja;
}

void CMakeOpenProjectWizard::setUseNinja(bool b)
{
    m_useNinja = b;
}

QString CMakeOpenProjectWizard::arguments() const
{
    return m_arguments;
}

void CMakeOpenProjectWizard::setArguments(const QString &args)
{
    m_arguments = args;
}

Utils::Environment CMakeOpenProjectWizard::environment() const
{
    return m_environment;
}

ProjectExplorer::Kit *CMakeOpenProjectWizard::kit() const
{
    return m_kit;
}

void CMakeOpenProjectWizard::setKit(ProjectExplorer::Kit *kit)
{
    m_kit = kit;
}

InSourceBuildPage::InSourceBuildPage(CMakeOpenProjectWizard *cmakeWizard)
    : QWizardPage(cmakeWizard), m_cmakeWizard(cmakeWizard)
{
    setLayout(new QVBoxLayout);
    QLabel *label = new QLabel(this);
    label->setWordWrap(true);
    label->setText(tr("Qt Creator has detected an <b>in-source-build in %1</b> "
                   "which prevents shadow builds. Qt Creator will not allow you to change the build directory. "
                   "If you want a shadow build, clean your source directory and re-open the project.")
                   .arg(m_cmakeWizard->buildDirectory()));
    layout()->addWidget(label);
    setTitle(tr("Build Location"));
}

ShadowBuildPage::ShadowBuildPage(CMakeOpenProjectWizard *cmakeWizard, bool change)
    : QWizardPage(cmakeWizard), m_cmakeWizard(cmakeWizard)
{
    QFormLayout *fl = new QFormLayout;
    this->setLayout(fl);

    QLabel *label = new QLabel(this);
    label->setWordWrap(true);
    if (change)
        label->setText(tr("Please enter the directory in which you want to build your project. "));
    else
        label->setText(tr("Please enter the directory in which you want to build your project. "
                          "Qt Creator recommends to not use the source directory for building. "
                          "This ensures that the source directory remains clean and enables multiple builds "
                          "with different settings."));
    fl->addRow(label);
    m_pc = new Utils::PathChooser(this);
    m_pc->setBaseDirectory(m_cmakeWizard->sourceDirectory());
    m_pc->setPath(m_cmakeWizard->buildDirectory());
    m_pc->setExpectedKind(Utils::PathChooser::Directory);
    connect(m_pc, SIGNAL(changed(QString)), this, SLOT(buildDirectoryChanged()));
    fl->addRow(tr("Build directory:"), m_pc);
    setTitle(tr("Build Location"));
}

void ShadowBuildPage::buildDirectoryChanged()
{
    m_cmakeWizard->setBuildDirectory(m_pc->path());
}

ChooseCMakePage::ChooseCMakePage(CMakeOpenProjectWizard *cmakeWizard)
    : QWizardPage(cmakeWizard), m_cmakeWizard(cmakeWizard)
{
    QFormLayout *fl = new QFormLayout;
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);

    m_cmakeLabel = new QLabel;
    m_cmakeLabel->setWordWrap(true);
    fl->addRow(m_cmakeLabel);
    // Show a field for the user to enter
    m_cmakeExecutable = new Utils::PathChooser(this);
    m_cmakeExecutable->setExpectedKind(Utils::PathChooser::ExistingCommand);
    fl->addRow(tr("cmake Executable:"), m_cmakeExecutable);

    connect(m_cmakeExecutable, SIGNAL(editingFinished()),
            this, SLOT(cmakeExecutableChanged()));
    connect(m_cmakeExecutable, SIGNAL(browsingFinished()),
            this, SLOT(cmakeExecutableChanged()));

    setTitle(tr("Choose Cmake Executable"));
}

void ChooseCMakePage::updateErrorText()
{
    QString cmakeExecutable = m_cmakeWizard->cmakeManager()->cmakeExecutable();
    if (m_cmakeWizard->cmakeManager()->isCMakeExecutableValid()) {
        m_cmakeLabel->setText(tr("The cmake executable is valid."));
    } else {
        QString text = tr("Please specify the path to the cmake executable. No cmake executable was found in the path.");
        if (!cmakeExecutable.isEmpty()) {
            QFileInfo fi(cmakeExecutable);
            if (!fi.exists())
                text += tr(" The cmake executable (%1) does not exist.").arg(cmakeExecutable);
            else if (!fi.isExecutable())
                text += tr(" The path %1 is not a executable.").arg(cmakeExecutable);
            else
                text += tr(" The path %1 is not a valid cmake.").arg(cmakeExecutable);
        }
        m_cmakeLabel->setText(text);
    }
}

void ChooseCMakePage::cmakeExecutableChanged()
{
    m_cmakeWizard->cmakeManager()->setCMakeExecutable(m_cmakeExecutable->path());
    updateErrorText();
    emit completeChanged();
}

bool ChooseCMakePage::isComplete() const
{
    return m_cmakeWizard->cmakeManager()->isCMakeExecutableValid();
}

CMakeRunPage::CMakeRunPage(CMakeOpenProjectWizard *cmakeWizard, Mode mode, const QString &buildDirectory)
    : QWizardPage(cmakeWizard),
      m_cmakeWizard(cmakeWizard),
      m_complete(false),
      m_optionalCMake(false),
      m_mode(mode),
      m_buildDirectory(buildDirectory)
{
    initWidgets();
}

void CMakeRunPage::initWidgets()
{
    QFormLayout *fl = new QFormLayout;
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);
    // Description Label
    m_descriptionLabel = new QLabel(this);
    m_descriptionLabel->setWordWrap(true);

    fl->addRow(m_descriptionLabel);

    // Run CMake Line (with arguments)
    m_argumentsLineEdit = new Utils::FancyLineEdit(this);
    m_argumentsLineEdit->setHistoryCompleter(QLatin1String("CMakeArgumentsLineEdit"));

    connect(m_argumentsLineEdit,SIGNAL(returnPressed()), this, SLOT(runCMake()));
    fl->addRow(tr("Arguments:"), m_argumentsLineEdit);

    m_generatorComboBox = new QComboBox(this);
    fl->addRow(tr("Generator:"), m_generatorComboBox);

    m_runCMake = new QPushButton(this);
    m_runCMake->setText(tr("Run CMake"));
    connect(m_runCMake, SIGNAL(clicked()), this, SLOT(runCMake()));

    QHBoxLayout *hbox2 = new QHBoxLayout;
    hbox2->addStretch(10);
    hbox2->addWidget(m_runCMake);
    fl->addRow(hbox2);

    // Bottom output window
    m_output = new QPlainTextEdit(this);
    m_output->setReadOnly(true);
    // set smaller minimum size to avoid vanishing descriptions if all of the
    // above is shown and the dialog not vertically resizing to fit stuff in (Mac)
    m_output->setMinimumHeight(15);
    QFont f(TextEditor::FontSettings::defaultFixedFontFamily());
    f.setStyleHint(QFont::TypeWriter);
    m_output->setFont(f);
    QSizePolicy pl = m_output->sizePolicy();
    pl.setVerticalStretch(1);
    m_output->setSizePolicy(pl);
    fl->addRow(m_output);

    m_exitCodeLabel = new QLabel(this);
    m_exitCodeLabel->setVisible(false);
    fl->addRow(m_exitCodeLabel);

    setTitle(tr("Run CMake"));
    setMinimumSize(600, 400);
}

QByteArray CMakeRunPage::cachedGeneratorFromFile(const QString &cache)
{
    QFile fi(cache);
    if (fi.exists()) {
        // Cache exists, then read it...
        if (fi.open(QIODevice::ReadOnly | QIODevice::Text)) {
            while (!fi.atEnd()) {
                QByteArray line = fi.readLine();
                if (line.startsWith("CMAKE_GENERATOR:INTERNAL=")) {
                    int splitpos = line.indexOf('=');
                    if (splitpos != -1) {
                        QByteArray cachedGenerator = line.mid(splitpos + 1).trimmed();
                        if (!cachedGenerator.isEmpty())
                            return cachedGenerator;
                    }
                }
            }
        }
    }
    return QByteArray();
}

void CMakeRunPage::initializePage()
{
    if (m_mode == Initial) {
        bool upToDateXmlFile = m_cmakeWizard->existsUpToDateXmlFile();;
        m_buildDirectory = m_cmakeWizard->buildDirectory();

        if (upToDateXmlFile) {
            m_descriptionLabel->setText(
                    tr("The directory %1 already contains a cbp file, which is recent enough. "
                       "You can pass special arguments and rerun CMake. "
                       "Or simply finish the wizard directly.").arg(m_buildDirectory));
            m_optionalCMake = true;
            m_complete = true;
        } else {
            m_descriptionLabel->setText(
                    tr("The directory %1 does not contain a cbp file. Qt Creator needs to create this file by running CMake. "
                       "Some projects require command line arguments to the initial CMake call.").arg(m_buildDirectory));
        }
    } else if (m_mode == CMakeRunPage::NeedToUpdate) {
        m_descriptionLabel->setText(tr("The directory %1 contains an outdated .cbp file. Qt "
                                       "Creator needs to update this file by running CMake. "
                                       "If you want to add additional command line arguments, "
                                       "add them below. Note that CMake remembers command "
                                       "line arguments from the previous runs.").arg(m_buildDirectory));
    } else if (m_mode == CMakeRunPage::Recreate) {
        m_descriptionLabel->setText(tr("The directory %1 specified in a build-configuration, "
                                       "does not contain a cbp file. Qt Creator needs to "
                                       "recreate this file, by running CMake. "
                                       "Some projects require command line arguments to "
                                       "the initial CMake call. Note that CMake remembers command "
                                       "line arguments from the previous runs.").arg(m_buildDirectory));
    } else if (m_mode == CMakeRunPage::ChangeDirectory) {
        m_buildDirectory = m_cmakeWizard->buildDirectory();
        m_descriptionLabel->setText(tr("Qt Creator needs to run CMake in the new build directory. "
                                       "Some projects require command line arguments to the "
                                       "initial CMake call."));
    } else if (m_mode == CMakeRunPage::WantToUpdate) {
        m_descriptionLabel->setText(tr("Refreshing cbp file in %1.").arg(m_buildDirectory));
    }

    // Build the list of generators/toolchains we want to offer
    m_generatorComboBox->clear();

    bool hasCodeBlocksGenerator = m_cmakeWizard->cmakeManager()->hasCodeBlocksMsvcGenerator();
    bool hasNinjaGenerator = m_cmakeWizard->cmakeManager()->hasCodeBlocksNinjaGenerator();

    if (m_mode == Initial) {
        // Try figuring out generator and toolchain from CMakeCache.txt
        QByteArray cachedGenerator = cachedGeneratorFromFile(m_buildDirectory + QLatin1String("/CMakeCache.txt"));

        m_generatorComboBox->show();
        QList<ProjectExplorer::Kit *> kitList =
                ProjectExplorer::KitManager::instance()->kits();

        foreach (ProjectExplorer::Kit *k, kitList) {
            QList<GeneratorInfo> infos = GeneratorInfo::generatorInfosFor(k,
                                                                          hasNinjaGenerator ? GeneratorInfo::OfferNinja : GeneratorInfo::NoNinja,
                                                                          hasCodeBlocksGenerator);

            foreach (const GeneratorInfo &info, infos)
                if (cachedGenerator.isEmpty() || info.generator() == cachedGenerator)
                    m_generatorComboBox->addItem(info.displayName(), qVariantFromValue(info));
        }
    } else {
        // Note: We don't compare the actually cached generator to what is set in the buildconfiguration
        // We assume that the buildconfiguration is correct
        GeneratorInfo::Ninja ninja;
        if (m_mode == CMakeRunPage::NeedToUpdate || m_mode == CMakeRunPage::WantToUpdate) {
            ninja = m_cmakeWizard->useNinja() ? GeneratorInfo::ForceNinja : GeneratorInfo::NoNinja;
        } else { // Recreate, ChangeDirectory
            // Note: ReCreate is technically just a removed .cbp file, we assume the cache
            // got removed too. If the cache still exists the error message from cmake should
            // be a good hint to change the generator
            ninja = hasNinjaGenerator ? GeneratorInfo::OfferNinja : GeneratorInfo::NoNinja;
        }

        QList<GeneratorInfo> infos = GeneratorInfo::generatorInfosFor(m_cmakeWizard->kit(),
                                                                      ninja,
                                                                      true);
        foreach (const GeneratorInfo &info, infos)
            m_generatorComboBox->addItem(info.displayName(), qVariantFromValue(info));
    }
}

bool CMakeRunPage::validatePage()
{
    if (m_optionalCMake) {
        int index = m_generatorComboBox->currentIndex();
        if (index == -1)
            return false;
        GeneratorInfo generatorInfo = m_generatorComboBox->itemData(index).value<GeneratorInfo>();
        m_cmakeWizard->setKit(generatorInfo.kit());
        m_cmakeWizard->setUseNinja(generatorInfo.isNinja());
    }
    return QWizardPage::validatePage();
}

void CMakeRunPage::runCMake()
{
    m_optionalCMake = false;
    m_complete = false;

    Utils::Environment env = m_cmakeWizard->environment();
    int index = m_generatorComboBox->currentIndex();

    if (index == -1) {
        m_output->appendPlainText(tr("No generator selected."));
        return;
    }
    GeneratorInfo generatorInfo = m_generatorComboBox->itemData(index).value<GeneratorInfo>();
    m_cmakeWizard->setKit(generatorInfo.kit());
    m_cmakeWizard->setUseNinja(generatorInfo.isNinja());

    // If mode is initial the user chooses the kit, otherwise it's already choosen
    // and the environment already contains the kit
    if (m_mode == Initial)
        generatorInfo.kit()->addToEnvironment(env);

    m_runCMake->setEnabled(false);
    m_argumentsLineEdit->setEnabled(false);
    m_generatorComboBox->setEnabled(false);

    m_output->clear();

    CMakeManager *cmakeManager = m_cmakeWizard->cmakeManager();
    if (m_cmakeWizard->cmakeManager()->isCMakeExecutableValid()) {
        m_cmakeProcess = new Utils::QtcProcess();
        connect(m_cmakeProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(cmakeReadyReadStandardOutput()));
        connect(m_cmakeProcess, SIGNAL(readyReadStandardError()), this, SLOT(cmakeReadyReadStandardError()));
        connect(m_cmakeProcess, SIGNAL(finished(int)), this, SLOT(cmakeFinished()));
        cmakeManager->createXmlFile(m_cmakeProcess, m_argumentsLineEdit->text(), m_cmakeWizard->sourceDirectory(),
                                    m_buildDirectory, env, QString::fromLatin1(generatorInfo.generatorArgument()));
    } else {
        m_runCMake->setEnabled(true);
        m_argumentsLineEdit->setEnabled(true);
        m_generatorComboBox->setEnabled(true);
        m_output->appendPlainText(tr("No valid cmake executable specified."));
    }
}

static QColor mix_colors(QColor a, QColor b)
{
    return QColor((a.red() + 2 * b.red()) / 3, (a.green() + 2 * b.green()) / 3,
                  (a.blue() + 2* b.blue()) / 3, (a.alpha() + 2 * b.alpha()) / 3);
}

void CMakeRunPage::cmakeReadyReadStandardOutput()
{
    QTextCursor cursor(m_output->document());
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat tf;

    QFont font = m_output->font();
    tf.setFont(font);
    tf.setForeground(m_output->palette().color(QPalette::Text));

    cursor.insertText(QString::fromLocal8Bit(m_cmakeProcess->readAllStandardOutput()), tf);
}

void CMakeRunPage::cmakeReadyReadStandardError()
{
    QTextCursor cursor(m_output->document());
    QTextCharFormat tf;

    QFont font = m_output->font();
    QFont boldFont = font;
    boldFont.setBold(true);
    tf.setFont(boldFont);
    tf.setForeground(mix_colors(m_output->palette().color(QPalette::Text), QColor(Qt::red)));

    cursor.insertText(QString::fromLocal8Bit(m_cmakeProcess->readAllStandardError()), tf);
}

void CMakeRunPage::cmakeFinished()
{
    m_runCMake->setEnabled(true);
    m_argumentsLineEdit->setEnabled(true);
    m_generatorComboBox->setEnabled(true);

    if (m_cmakeProcess->exitCode() != 0) {
        m_exitCodeLabel->setVisible(true);
        m_exitCodeLabel->setText(tr("CMake exited with errors. Please check CMake output."));
        m_complete = false;
    } else {
        m_exitCodeLabel->setVisible(false);
        m_complete = true;
    }
    m_cmakeProcess->deleteLater();
    m_cmakeProcess = 0;
    m_cmakeWizard->setArguments(m_argumentsLineEdit->text());
    emit completeChanged();
}

void CMakeRunPage::cleanupPage()
{
    m_output->clear();
    m_complete = false;
    m_exitCodeLabel->setVisible(false);
    emit completeChanged();
}

bool CMakeRunPage::isComplete() const
{
    return m_complete;
}
