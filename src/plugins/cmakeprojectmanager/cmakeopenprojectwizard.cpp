/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "cmakeopenprojectwizard.h"
#include "cmakeprojectmanager.h"

#include <coreplugin/icore.h>
#include <utils/pathchooser.h>
#include <utils/historycompleter.h>
#include <projectexplorer/profileinformation.h>
#include <projectexplorer/profilemanager.h>
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

CMakeOpenProjectWizard::CMakeOpenProjectWizard(CMakeManager *cmakeManager, const QString &sourceDirectory, const Utils::Environment &env)
    : m_cmakeManager(cmakeManager),
      m_sourceDirectory(sourceDirectory),
      m_creatingCbpFiles(false),
      m_environment(env)
{
    int startid;
    if (hasInSourceBuild()) {
        startid = InSourcePageId;
        m_buildDirectory = m_sourceDirectory;
    } else {
        startid = ShadowBuildPageId;
        m_buildDirectory = m_sourceDirectory + QLatin1String("-build");
    }

    setPage(InSourcePageId, new InSourceBuildPage(this));
    setPage(ShadowBuildPageId, new ShadowBuildPage(this));
    setPage(CMakeRunPageId, new CMakeRunPage(this));

    Utils::WizardProgress *wp = wizardProgress();
    Utils::WizardProgressItem *inSourceItem = wp->item(InSourcePageId);
    Utils::WizardProgressItem *shadowBuildItem = wp->item(ShadowBuildPageId);
    Utils::WizardProgressItem *cmakeRunItem = wp->item(CMakeRunPageId);
    inSourceItem->setNextItems(QList<Utils::WizardProgressItem *>() << cmakeRunItem);
    shadowBuildItem->setNextItems(QList<Utils::WizardProgressItem *>() << cmakeRunItem);

    setStartId(startid);
    init();
}

CMakeOpenProjectWizard::CMakeOpenProjectWizard(CMakeManager *cmakeManager, const QString &sourceDirectory,
                                               const QString &buildDirectory, CMakeOpenProjectWizard::Mode mode,
                                               const Utils::Environment &env)
    : m_cmakeManager(cmakeManager),
      m_sourceDirectory(sourceDirectory),
      m_creatingCbpFiles(true),
      m_environment(env)
{

    CMakeRunPage::Mode rmode;
    if (mode == CMakeOpenProjectWizard::NeedToCreate)
        rmode = CMakeRunPage::Recreate;
    else if (mode == CMakeOpenProjectWizard::WantToUpdate)
        rmode = CMakeRunPage::WantToUpdate;
    else
        rmode = CMakeRunPage::NeedToUpdate;
    addPage(new CMakeRunPage(this, rmode, buildDirectory));
    init();
}

CMakeOpenProjectWizard::CMakeOpenProjectWizard(CMakeManager *cmakeManager, const QString &sourceDirectory,
                                               const QString &oldBuildDirectory,
                                               const Utils::Environment &env)
    : m_cmakeManager(cmakeManager),
      m_sourceDirectory(sourceDirectory),
      m_creatingCbpFiles(true),
      m_environment(env)
{
    m_buildDirectory = oldBuildDirectory;
    addPage(new ShadowBuildPage(this, true));
    addPage(new CMakeRunPage(this, CMakeRunPage::ChangeDirectory));
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

int CMakeOpenProjectWizard::nextId() const
{
    if (m_creatingCbpFiles)
        return QWizard::nextId();
    int cid = currentId();
    if (cid == InSourcePageId) {
        return CMakeRunPageId;
    } else if (cid == ShadowBuildPageId) {
        return CMakeRunPageId;
    }
    return -1;
}


bool CMakeOpenProjectWizard::hasInSourceBuild() const
{
    QFileInfo fi(m_sourceDirectory + "/CMakeCache.txt");
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
        QFileInfo cmakeListsFileInfo(sourceDirectory() + "/CMakeLists.txt");

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

CMakeRunPage::CMakeRunPage(CMakeOpenProjectWizard *cmakeWizard, Mode mode, const QString &buildDirectory)
    : QWizardPage(cmakeWizard),
      m_cmakeWizard(cmakeWizard),
      m_complete(false),
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

    if (m_cmakeWizard->cmakeManager()->isCMakeExecutableValid()) {
        m_cmakeExecutable = 0;
    } else {
        QString text = tr("Please specify the path to the cmake executable. No cmake executable was found in the path.");
        QString cmakeExecutable = m_cmakeWizard->cmakeManager()->cmakeExecutable();
        if (!cmakeExecutable.isEmpty()) {
            QFileInfo fi(cmakeExecutable);
            if (!fi.exists())
                text += tr(" The cmake executable (%1) does not exist.").arg(cmakeExecutable);
            else if (!fi.isExecutable())
                text += tr(" The path %1 is not a executable.").arg(cmakeExecutable);
            else
                text += tr(" The path %1 is not a valid cmake.").arg(cmakeExecutable);
        }

        QLabel *cmakeLabel = new QLabel(text);
        cmakeLabel->setWordWrap(true);
        fl->addRow(cmakeLabel);
        // Show a field for the user to enter
        m_cmakeExecutable = new Utils::PathChooser(this);
        m_cmakeExecutable->setExpectedKind(Utils::PathChooser::ExistingCommand);
        fl->addRow("cmake Executable:", m_cmakeExecutable);
    }

    // Run CMake Line (with arguments)
    m_argumentsLineEdit = new QLineEdit(this);
    m_argumentsLineEdit->setCompleter(
        new Utils::HistoryCompleter(m_argumentsLineEdit, QLatin1String("CMakeArgumentsLineEdit")));

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

void CMakeRunPage::initializePage()
{
    if (m_mode == Initial) {
        m_complete = m_cmakeWizard->existsUpToDateXmlFile();
        m_buildDirectory = m_cmakeWizard->buildDirectory();

        if (m_cmakeWizard->existsUpToDateXmlFile()) {
            m_descriptionLabel->setText(
                    tr("The directory %1 already contains a cbp file, which is recent enough. "
                       "You can pass special arguments or change the used tool chain here and rerun CMake. "
                       "Or simply finish the wizard directly.").arg(m_buildDirectory));
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


    // Try figuring out generator and toolchain from CMakeCache.txt
    QString cachedGenerator;
    QString cmakeCxxCompiler;
    QFile fi(m_buildDirectory + "/CMakeCache.txt");
    if (fi.exists()) {
        // Cache exists, then read it...
        if (fi.open(QIODevice::ReadOnly | QIODevice::Text)) {
            while (!fi.atEnd()) {
                QString line = fi.readLine();
                if (line.startsWith("CMAKE_GENERATOR:INTERNAL=")) {
                    int splitpos = line.indexOf('=');
                    if (splitpos != -1)
                        cachedGenerator = line.mid(splitpos + 1).trimmed();
                }
                if (line.startsWith("CMAKE_CXX_COMPILER:FILEPATH=")) {
                    int splitpos = line.indexOf("=");
                    if (splitpos != -1)
                        cmakeCxxCompiler = line.mid(splitpos +1).trimmed();
                }
                if (!cachedGenerator.isEmpty() && !cmakeCxxCompiler.isEmpty())
                    break;
            }
        }
    }

    // Build the list of generators/toolchains we want to offer
    // restrict toolchains based on CMAKE_CXX_COMPILER ?
    Q_UNUSED(cmakeCxxCompiler);
    m_generatorComboBox->clear();
    bool hasCodeBlocksGenerator = m_cmakeWizard->cmakeManager()->hasCodeBlocksMsvcGenerator();

    QList<ProjectExplorer::Profile *> profileList =
            ProjectExplorer::ProfileManager::instance()->profiles();

    foreach (ProjectExplorer::Profile *p, profileList) {
        QVariant profileVariant = qVariantFromValue(static_cast<void *>(p));

        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainProfileInformation::toolChain(p);
        if (!tc)
            continue;
        ProjectExplorer::Abi targetAbi = tc->targetAbi();
        if (targetAbi.os() == ProjectExplorer::Abi::WindowsOS) {
            if (targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2005Flavor
                    || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2008Flavor
                    || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2010Flavor
                    || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2012Flavor) {
                if (hasCodeBlocksGenerator && (cachedGenerator.isEmpty() || cachedGenerator == "NMake Makefiles"))
                    m_generatorComboBox->addItem(tr("NMake Generator (%1)").arg(p->displayName()), profileVariant);
             } else if (targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMSysFlavor) {
                if (cachedGenerator.isEmpty() || cachedGenerator == "MinGW Makefiles")
                    m_generatorComboBox->addItem(tr("MinGW Generator (%1)").arg(p->displayName()), profileVariant);
            }
        } else {
            // Non windows
            if (cachedGenerator.isEmpty() || cachedGenerator == "Unix Makefiles")
                m_generatorComboBox->addItem(tr("Unix Generator (%1)").arg(p->displayName()), profileVariant);
        }
    }
}

void CMakeRunPage::runCMake()
{
    if (m_cmakeExecutable)
        // We asked the user for the cmake executable
        m_cmakeWizard->cmakeManager()->setCMakeExecutable(m_cmakeExecutable->path());

    int index = m_generatorComboBox->currentIndex();

    ProjectExplorer::Profile *p = 0;
    if (index >= 0)
        p = static_cast<ProjectExplorer::Profile *>(m_generatorComboBox->itemData(index).value<void *>());
    if (!p) {
        m_output->appendPlainText(tr("No generator selected."));
        return;
    }

    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainProfileInformation::toolChain(p);

    m_runCMake->setEnabled(false);
    m_argumentsLineEdit->setEnabled(false);
    m_generatorComboBox->setEnabled(false);
    CMakeManager *cmakeManager = m_cmakeWizard->cmakeManager();

    QString generator = QLatin1String("-GCodeBlocks - Unix Makefiles");
    if (tc->targetAbi().os() == ProjectExplorer::Abi::WindowsOS) {
        if (tc->targetAbi().osFlavor() == ProjectExplorer::Abi::WindowsMSysFlavor)
            generator = QLatin1String("-GCodeBlocks - MinGW Makefiles");
        else
            generator = QLatin1String("-GCodeBlocks - NMake Makefiles");
    }

    Utils::Environment env = m_cmakeWizard->environment();
    tc->addToEnvironment(env);

    m_output->clear();

    if (m_cmakeWizard->cmakeManager()->isCMakeExecutableValid()) {
        m_cmakeProcess = new Utils::QtcProcess();
        connect(m_cmakeProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(cmakeReadyReadStandardOutput()));
        connect(m_cmakeProcess, SIGNAL(readyReadStandardError()), this, SLOT(cmakeReadyReadStandardError()));
        connect(m_cmakeProcess, SIGNAL(finished(int)), this, SLOT(cmakeFinished()));
        cmakeManager->createXmlFile(m_cmakeProcess, m_argumentsLineEdit->text(), m_cmakeWizard->sourceDirectory(), m_buildDirectory, env, generator);
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

    cursor.insertText(m_cmakeProcess->readAllStandardOutput(), tf);
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

    cursor.insertText(m_cmakeProcess->readAllStandardError(), tf);
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
