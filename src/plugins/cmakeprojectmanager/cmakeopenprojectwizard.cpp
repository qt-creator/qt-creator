/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cmakeprojectconstants.h"
#include "cmakeopenprojectwizard.h"
#include "cmakeprojectmanager.h"
#include "cmaketoolmanager.h"
#include "cmakebuildconfiguration.h"
#include "cmakebuildinfo.h"
#include "cmakekitinformation.h"
#include "cmaketool.h"
#include "generatorinfo.h"

#include <coreplugin/icore.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>
#include <utils/fancylineedit.h>
#include <utils/historycompleter.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <texteditor/fontsettings.h>
#include <remotelinux/remotelinux_constants.h>

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QDateTime>
#include <QSettings>
#include <QStringList>
#include <QApplication>
#include <QCheckBox>
#include <QDir>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;
using namespace ProjectExplorer;

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


//////////////
/// CMakeOpenProjectWizard
//////////////
CMakeOpenProjectWizard::CMakeOpenProjectWizard(QWidget *parent,
                                               CMakeOpenProjectWizard::Mode mode,
                                               const CMakeBuildInfo *info) :
    Utils::Wizard(parent),
    m_sourceDirectory(info->sourceDirectory),
    m_environment(info->environment),
    m_kit(KitManager::find(info->kitId))
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
        m_buildDirectory = info->buildDirectory.toString();
        addPage(new ShadowBuildPage(this, true));
    }

    if (CMakeToolManager::cmakeTools().isEmpty())
        addPage(new NoCMakePage(this));

    addPage(new CMakeRunPage(this, rmode, info->buildDirectory.toString(), info->arguments,
                             m_kit->displayName(), info->displayName));

    setWindowTitle(tr("CMake Wizard"));
}

bool CMakeOpenProjectWizard::hasInSourceBuild() const
{
    return QFileInfo::exists(m_sourceDirectory + QLatin1String("/CMakeCache.txt"));
}

bool CMakeOpenProjectWizard::compatibleKitExist() const
{
    bool preferNinja = CMakeManager::preferNinja();
    const QList<Kit *> kitList = KitManager::kits();

    foreach (Kit *k, kitList) {
        CMakeTool *cmake = CMakeKitInformation::cmakeTool(k);
        if (!cmake)
            continue;

        bool hasCodeBlocksGenerator = cmake->hasCodeBlocksMsvcGenerator();
        bool hasNinjaGenerator = cmake->hasCodeBlocksNinjaGenerator();

        // OfferNinja and ForceNinja differ in what they return
        // but not whether the list is empty or not, which is what we
        // are interested in here
        QList<GeneratorInfo> infos = GeneratorInfo::generatorInfosFor(k, hasNinjaGenerator,
                                                                      preferNinja,
                                                                      hasCodeBlocksGenerator);
        if (!infos.isEmpty())
            return true;
    }
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

Kit *CMakeOpenProjectWizard::kit() const
{
    return m_kit;
}

void CMakeOpenProjectWizard::setKit(Kit *kit)
{
    m_kit = kit;
}

//////
// NoKitPage
/////

NoKitPage::NoKitPage(CMakeOpenProjectWizard *cmakeWizard)
    : QWizardPage(cmakeWizard), m_cmakeWizard(cmakeWizard)
{
    auto layout = new QVBoxLayout;
    setLayout(layout);

    m_descriptionLabel = new QLabel(this);
    m_descriptionLabel->setWordWrap(true);
    layout->addWidget(m_descriptionLabel);

    m_optionsButton = new QPushButton;
    m_optionsButton->setText(Core::ICore::msgShowOptionsDialog());

    connect(m_optionsButton, &QAbstractButton::clicked, this, &NoKitPage::showOptions);

    auto hbox = new QHBoxLayout;
    hbox->addWidget(m_optionsButton);
    hbox->addStretch();

    layout->addLayout(hbox);

    setTitle(tr("Check Kits"));

    connect(KitManager::instance(), &KitManager::kitsChanged, this, &NoKitPage::kitsChanged);

    kitsChanged();
}

void NoKitPage::kitsChanged()
{
    if (isComplete()) {
        m_descriptionLabel->setText(tr("There are compatible kits."));
        m_optionsButton->setVisible(false);
    } else {
        m_descriptionLabel->setText(tr("Qt Creator has no kits that are suitable for CMake projects. Please configure a kit."));
        m_optionsButton->setVisible(true);
    }
    emit completeChanged();
}

bool NoKitPage::isComplete() const
{
    return m_cmakeWizard->compatibleKitExist();
}

void NoKitPage::initializePage()
{
    //if the NoCMakePage was added, we need to recheck if kits exist
    kitsChanged();
}

void NoKitPage::showOptions()
{
    Core::ICore::showOptionsDialog(ProjectExplorer::Constants::KITS_SETTINGS_PAGE_ID, this);
}

InSourceBuildPage::InSourceBuildPage(CMakeOpenProjectWizard *cmakeWizard)
    : QWizardPage(cmakeWizard), m_cmakeWizard(cmakeWizard)
{
    setLayout(new QVBoxLayout);
    auto label = new QLabel(this);
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
    auto fl = new QFormLayout;
    this->setLayout(fl);

    auto label = new QLabel(this);
    label->setWordWrap(true);
    if (change)
        label->setText(tr("Please enter the directory in which you want to build your project.") + QLatin1Char(' '));
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
    m_pc->setHistoryCompleter(QLatin1String("Cmake.BuildDir.History"));
    connect(m_pc, &Utils::PathChooser::rawPathChanged, this, &ShadowBuildPage::buildDirectoryChanged);
    fl->addRow(tr("Build directory:"), m_pc);
    setTitle(tr("Build Location"));
}

void ShadowBuildPage::buildDirectoryChanged()
{
    m_cmakeWizard->setBuildDirectory(m_pc->path());
}

//////
// NoCMakePage
/////

NoCMakePage::NoCMakePage(CMakeOpenProjectWizard *cmakeWizard) : QWizardPage(cmakeWizard)
{
    auto layout = new QVBoxLayout;
    setLayout(layout);

    m_descriptionLabel = new QLabel(this);
    m_descriptionLabel->setWordWrap(true);
    layout->addWidget(m_descriptionLabel);

    m_optionsButton = new QPushButton;
    m_optionsButton->setText(Core::ICore::msgShowOptionsDialog());

    connect(m_optionsButton, &QAbstractButton::clicked, this, &NoCMakePage::showOptions);

    auto hbox = new QHBoxLayout;
    hbox->addWidget(m_optionsButton);
    hbox->addStretch();

    layout->addLayout(hbox);

    setTitle(tr("Check CMake Tools"));

    connect(CMakeToolManager::instance(), &CMakeToolManager::cmakeToolsChanged,
            this, &NoCMakePage::cmakeToolsChanged);

    cmakeToolsChanged();
}

void NoCMakePage::cmakeToolsChanged()
{
    if (isComplete()) {
        m_descriptionLabel->setText(tr("There are CMake Tools registered."));
        m_optionsButton->setVisible(false);
    } else {
        m_descriptionLabel->setText(tr("Qt Creator has no CMake Tools that are required for CMake projects. Please configure at least one."));
        m_optionsButton->setVisible(true);
    }
    emit completeChanged();
}

bool NoCMakePage::isComplete() const
{
    return !CMakeToolManager::cmakeTools().isEmpty();
}

void NoCMakePage::showOptions()
{
    Core::ICore::showOptionsDialog(Constants::CMAKE_SETTINGSPAGE_ID, this);
}

CMakeRunPage::CMakeRunPage(CMakeOpenProjectWizard *cmakeWizard, Mode mode,
                           const QString &buildDirectory, const QString &initialArguments,
                           const QString &kitName, const QString &buildConfigurationName) :
    QWizardPage(cmakeWizard),
    m_cmakeWizard(cmakeWizard),
    m_descriptionLabel(new QLabel(this)),
    m_argumentsLineEdit(new Utils::FancyLineEdit(this)),
    m_generatorComboBox(new QComboBox(this)),
    m_generatorExtraText(new QLabel(this)),
    m_runCMake(new QPushButton(this)),
    m_output(new QPlainTextEdit(this)),
    m_exitCodeLabel(new QLabel(this)),
    m_continueCheckBox(new QCheckBox(this)),
    m_mode(mode),
    m_buildDirectory(buildDirectory),
    m_kitName(kitName),
    m_buildConfigurationName(buildConfigurationName)
{
    auto fl = new QFormLayout;
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);
    // Description Label
    m_descriptionLabel->setWordWrap(true);

    fl->addRow(m_descriptionLabel);

    // Run CMake Line (with arguments)
    m_argumentsLineEdit->setHistoryCompleter(QLatin1String("CMakeArgumentsLineEdit"));
    m_argumentsLineEdit->selectAll();

    connect(m_argumentsLineEdit, &QLineEdit::returnPressed, this, &CMakeRunPage::runCMake);
    fl->addRow(tr("Arguments:"), m_argumentsLineEdit);
    fl->addRow(tr("Generator:"), m_generatorComboBox);
    fl->addRow(m_generatorExtraText);

    m_runCMake->setText(tr("Run CMake"));
    connect(m_runCMake, &QAbstractButton::clicked, this, &CMakeRunPage::runCMake);

    auto hbox2 = new QHBoxLayout;
    hbox2->addStretch(10);
    hbox2->addWidget(m_runCMake);
    fl->addRow(hbox2);

    // Bottom output window
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

    m_exitCodeLabel->setVisible(false);
    fl->addRow(m_exitCodeLabel);

    m_continueCheckBox->setVisible(false);
    m_continueCheckBox->setText(tr("Open project with errors."));
    connect(m_continueCheckBox, &QCheckBox::toggled, this, &CMakeRunPage::completeChanged);
    fl->addRow(m_continueCheckBox);

    setTitle(tr("Run CMake"));
    setMinimumSize(600, 400);

    m_argumentsLineEdit->setText(initialArguments);
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
    if (m_mode == CMakeRunPage::NeedToUpdate) {
        m_descriptionLabel->setText(tr("The build directory \"%1\" for build configuration \"%2\" "
                                       "for target \"%3\" contains an outdated .cbp file. Qt "
                                       "Creator needs to update this file by running CMake. "
                                       "You can add command line arguments below. Note that "
                                       "CMake remembers command line arguments from the "
                                       "previous runs.")
                                    .arg(QDir::toNativeSeparators(m_buildDirectory))
                                    .arg(m_buildConfigurationName)
                                    .arg(m_kitName));
    } else if (m_mode == CMakeRunPage::Recreate) {
        m_descriptionLabel->setText(tr("The directory \"%1\" specified in build configuration \"%2\", "
                                       "for target \"%3\" does not contain a .cbp file. "
                                       "Qt Creator needs to recreate this file by running CMake. "
                                       "Some projects require command line arguments to "
                                       "the initial CMake call. Note that CMake remembers command "
                                       "line arguments from the previous runs.")
                                    .arg(QDir::toNativeSeparators(m_buildDirectory))
                                    .arg(m_buildConfigurationName)
                                    .arg(m_kitName));
    } else if (m_mode == CMakeRunPage::ChangeDirectory) {
        m_buildDirectory = m_cmakeWizard->buildDirectory();
        m_descriptionLabel->setText(tr("Qt Creator needs to run CMake in the new build directory. "
                                       "Some projects require command line arguments to the "
                                       "initial CMake call."));
    } else if (m_mode == CMakeRunPage::WantToUpdate) {
        m_descriptionLabel->setText(tr("Refreshing the .cbp file in \"%1\" for build configuration \"%2\" "
                                       "for target \"%3\".")
                                    .arg(QDir::toNativeSeparators(m_buildDirectory))
                                    .arg(m_buildConfigurationName)
                                    .arg(m_kitName));
    }

    // Build the list of generators/toolchains we want to offer
    m_generatorComboBox->clear();

    bool preferNinja = CMakeManager::preferNinja();

    QList<GeneratorInfo> infos;
    CMakeTool *cmake = CMakeKitInformation::cmakeTool(m_cmakeWizard->kit());
    if (cmake) {
        // Note: We don't compare the actually cached generator to what is set in the buildconfiguration
        // We assume that the buildconfiguration is correct
        infos = GeneratorInfo::generatorInfosFor(m_cmakeWizard->kit(), true, preferNinja, true);
    }
    foreach (const GeneratorInfo &info, infos)
        m_generatorComboBox->addItem(info.displayName(), qVariantFromValue(info));

}

bool CMakeRunPage::validatePage()
{
    int index = m_generatorComboBox->currentIndex();
    if (index == -1)
        return false;
    GeneratorInfo generatorInfo = m_generatorComboBox->itemData(index).value<GeneratorInfo>();
    m_cmakeWizard->setKit(generatorInfo.kit());
    return QWizardPage::validatePage();
}

void CMakeRunPage::runCMake()
{
    QTC_ASSERT(!m_cmakeProcess, return);
    m_haveCbpFile = false;

    Utils::Environment env = m_cmakeWizard->environment();
    int index = m_generatorComboBox->currentIndex();

    if (index == -1) {
        m_output->appendPlainText(tr("No generator selected."));
        return;
    }
    GeneratorInfo generatorInfo = m_generatorComboBox->itemData(index).value<GeneratorInfo>();
    m_cmakeWizard->setKit(generatorInfo.kit());

    m_runCMake->setEnabled(false);
    m_argumentsLineEdit->setEnabled(false);
    m_generatorComboBox->setEnabled(false);

    m_output->clear();

    CMakeTool *cmake = CMakeKitInformation::cmakeTool(generatorInfo.kit());
    if (cmake && cmake->isValid()) {
        m_cmakeProcess = new Utils::QtcProcess();
        connect(m_cmakeProcess, &QProcess::readyReadStandardOutput,
                this, &CMakeRunPage::cmakeReadyReadStandardOutput);
        connect(m_cmakeProcess, &QProcess::readyReadStandardError,
                this, &CMakeRunPage::cmakeReadyReadStandardError);
        connect(m_cmakeProcess, static_cast<void(QProcess::*)(int)>(&QProcess::finished),
                this, &CMakeRunPage::cmakeFinished);

        QString arguments = m_argumentsLineEdit->text();

        m_output->appendPlainText(tr("Running: '%1' with arguments '%2' in '%3'.\n")
                                     .arg(cmake->cmakeExecutable().toUserOutput())
                                     .arg(arguments)
                                     .arg(QDir::toNativeSeparators(m_buildDirectory)));
        CMakeManager::createXmlFile(m_cmakeProcess, cmake->cmakeExecutable().toString(),
                                    arguments, m_cmakeWizard->sourceDirectory(),
                                    m_buildDirectory, env,
                                    QString::fromLatin1(generatorInfo.generatorArgument()),
                                    generatorInfo.preLoadCacheFileArgument());
    } else {
        m_runCMake->setEnabled(true);
        m_argumentsLineEdit->setEnabled(true);
        m_generatorComboBox->setEnabled(true);
        m_output->appendPlainText(tr("Selected kit has no valid CMake executable specified."));
    }
}

static QColor mix_colors(const QColor &a, const QColor &b)
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
    cursor.movePosition(QTextCursor::End);
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
        static_cast<Utils::HistoryCompleter *>(m_argumentsLineEdit->completer())->removeHistoryItem(0);
        m_haveCbpFile = false;
        m_continueCheckBox->setVisible(true);
    } else {
        m_exitCodeLabel->setVisible(false);
        m_continueCheckBox->setVisible(false);
        m_haveCbpFile = true;
    }
    m_cmakeProcess->deleteLater();
    m_cmakeProcess = 0;
    m_cmakeWizard->setArguments(m_argumentsLineEdit->text());
    emit completeChanged();
}

void CMakeRunPage::cleanupPage()
{
    m_output->clear();
    m_haveCbpFile = false;
    m_exitCodeLabel->setVisible(false);
    emit completeChanged();
}

bool CMakeRunPage::isComplete() const
{
    int index = m_generatorComboBox->currentIndex();
    return index != -1 && (m_haveCbpFile || m_continueCheckBox->isChecked());
}
