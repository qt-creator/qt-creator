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

#include "librarydetailscontroller.h"
#include "ui_librarydetailswidget.h"
#include "findqmakeprofiles.h"
#include "qmakenodes.h"
#include "qmakebuildconfiguration.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>

#include <QFileInfo>
#include <QDir>
#include <QTextStream>

using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace QmakeProjectManager::Internal;

LibraryDetailsController::LibraryDetailsController(
        Ui::LibraryDetailsWidget *libraryDetails,
        const QString &proFile, QObject *parent) :
    QObject(parent),
    m_proFile(proFile),
    m_libraryDetailsWidget(libraryDetails)
{
    switch (Utils::HostOsInfo::hostOs()) {
    case Utils::OsTypeMac:
        m_creatorPlatform = CreatorMac;
        break;
    case Utils::OsTypeLinux:
        m_creatorPlatform = CreatorLinux;
        break;
    case Utils::OsTypeWindows:
        m_creatorPlatform = CreatorWindows;
        break;
    default:
        break;
    }

    if (!Utils::HostOsInfo::isLinuxHost()) {
        // project for which we are going to insert the snippet
        const Project *project = SessionManager::projectForFile(Utils::FileName::fromString(proFile));
        if (project && project->activeTarget()) {
            // if its tool chain is maemo behave the same as we would be on linux
            ProjectExplorer::ToolChain *tc = ToolChainKitInformation::toolChain(project->activeTarget()->kit());
            if (tc) {
                switch (tc->targetAbi().os()) {
                case Abi::WindowsOS:
                    m_creatorPlatform = CreatorWindows;
                    break;
                case Abi::MacOS:
                    m_creatorPlatform = CreatorMac;
                    break;
                default:
                    m_creatorPlatform = CreatorLinux;
                    break;
                }
            }
        }
    }
    setPlatformsVisible(true);
    setLinkageGroupVisible(true);
    setMacLibraryGroupVisible(true);
    setPackageLineEditVisible(false);

    if (creatorPlatform() == CreatorMac)
        setMacLibraryRadiosVisible(false);

    if (creatorPlatform() != CreatorWindows)
        setLinkageRadiosVisible(false);

    connect(m_libraryDetailsWidget->includePathChooser, SIGNAL(rawPathChanged(QString)),
            this, SLOT(slotIncludePathChanged()));
    connect(m_libraryDetailsWidget->frameworkRadio, SIGNAL(clicked(bool)),
            this, SLOT(slotMacLibraryTypeChanged()));
    connect(m_libraryDetailsWidget->libraryRadio, SIGNAL(clicked(bool)),
            this, SLOT(slotMacLibraryTypeChanged()));
    connect(m_libraryDetailsWidget->useSubfoldersCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotUseSubfoldersChanged(bool)));
    connect(m_libraryDetailsWidget->addSuffixCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotAddSuffixChanged(bool)));
    connect(m_libraryDetailsWidget->linCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(slotPlatformChanged()));
    connect(m_libraryDetailsWidget->macCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(slotPlatformChanged()));
    connect(m_libraryDetailsWidget->winCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(slotPlatformChanged()));
}

LibraryDetailsController::CreatorPlatform LibraryDetailsController::creatorPlatform() const
{
    return m_creatorPlatform;
}

Ui::LibraryDetailsWidget *LibraryDetailsController::libraryDetailsWidget() const
{
    return m_libraryDetailsWidget;
}

AddLibraryWizard::Platforms LibraryDetailsController::platforms() const
{
    return m_platforms;
}

AddLibraryWizard::LinkageType LibraryDetailsController::linkageType() const
{
    return m_linkageType;
}

AddLibraryWizard::MacLibraryType LibraryDetailsController::macLibraryType() const
{
    return m_macLibraryType;
}

void LibraryDetailsController::updateGui()
{
    // read values from gui
    m_platforms = 0;
    if (libraryDetailsWidget()->linCheckBox->isChecked())
        m_platforms |= AddLibraryWizard::LinuxPlatform;
    if (libraryDetailsWidget()->macCheckBox->isChecked())
        m_platforms |= AddLibraryWizard::MacPlatform;
    if (libraryDetailsWidget()->winCheckBox->isChecked())
        m_platforms |= AddLibraryWizard::WindowsMinGWPlatform
                | AddLibraryWizard::WindowsMSVCPlatform;

    bool macLibraryTypeUpdated = false;
    if (!m_linkageRadiosVisible) {
        m_linkageType = suggestedLinkageType();
        if (m_linkageType == AddLibraryWizard::StaticLinkage) {
            m_macLibraryType = AddLibraryWizard::LibraryType;
            macLibraryTypeUpdated = true;
        }
    } else {
        m_linkageType = AddLibraryWizard::DynamicLinkage; // the default
        if (libraryDetailsWidget()->staticRadio->isChecked())
            m_linkageType = AddLibraryWizard::StaticLinkage;
    }

    if (!macLibraryTypeUpdated) {
        if (!m_macLibraryRadiosVisible) {
            m_macLibraryType = suggestedMacLibraryType();
        } else {
            m_macLibraryType = AddLibraryWizard::LibraryType; // the default
            if (libraryDetailsWidget()->frameworkRadio->isChecked())
                m_macLibraryType = AddLibraryWizard::FrameworkType;
        }
    }

    // enable or disable some parts of gui
    libraryDetailsWidget()->macGroupBox->setEnabled(platforms()
                                & AddLibraryWizard::MacPlatform);
    updateWindowsOptionsEnablement();
    const bool macRadiosEnabled = m_linkageRadiosVisible ||
            linkageType() != AddLibraryWizard::StaticLinkage;
    libraryDetailsWidget()->libraryRadio->setEnabled(macRadiosEnabled);
    libraryDetailsWidget()->frameworkRadio->setEnabled(macRadiosEnabled);

    // update values in gui
    setIgnoreGuiSignals(true);

    showLinkageType(linkageType());
    showMacLibraryType(macLibraryType());
    if (!m_includePathChanged)
        libraryDetailsWidget()->includePathChooser->setPath(suggestedIncludePath());

    setIgnoreGuiSignals(false);
}

QString LibraryDetailsController::proFile() const
{
    return m_proFile;
}

bool LibraryDetailsController::isIncludePathChanged() const
{
    return m_includePathChanged;
}

void LibraryDetailsController::setIgnoreGuiSignals(bool ignore)
{
    m_ignoreGuiSignals = ignore;
}

bool LibraryDetailsController::guiSignalsIgnored() const
{
    return m_ignoreGuiSignals;
}

void LibraryDetailsController::showLinkageType(
        AddLibraryWizard::LinkageType linkageType)
{
    const QString linkage(tr("Linkage:"));
    QString linkageTitle;
    switch (linkageType) {
    case AddLibraryWizard::DynamicLinkage:
        libraryDetailsWidget()->dynamicRadio->setChecked(true);
        linkageTitle = tr("%1 Dynamic").arg(linkage);
        break;
    case AddLibraryWizard::StaticLinkage:
        libraryDetailsWidget()->staticRadio->setChecked(true);
        linkageTitle = tr("%1 Static").arg(linkage);
        break;
    default:
        libraryDetailsWidget()->dynamicRadio->setChecked(false);
        libraryDetailsWidget()->staticRadio->setChecked(false);
        linkageTitle = linkage;
        break;
        }
    libraryDetailsWidget()->linkageGroupBox->setTitle(linkageTitle);
}

void LibraryDetailsController::showMacLibraryType(
        AddLibraryWizard::MacLibraryType libType)
{
    const QString libraryType(tr("Mac:"));
    QString libraryTypeTitle;
    switch (libType) {
    case AddLibraryWizard::FrameworkType:
        libraryDetailsWidget()->frameworkRadio->setChecked(true);
        libraryTypeTitle = tr("%1 Framework").arg(libraryType);
        break;
    case AddLibraryWizard::LibraryType:
        libraryDetailsWidget()->libraryRadio->setChecked(true);
        libraryTypeTitle = tr("%1 Library").arg(libraryType);
        break;
    default:
        libraryDetailsWidget()->frameworkRadio->setChecked(false);
        libraryDetailsWidget()->libraryRadio->setChecked(false);
        libraryTypeTitle = libraryType;
        break;
    }
    libraryDetailsWidget()->macGroupBox->setTitle(libraryTypeTitle);
}

void LibraryDetailsController::setPlatformsVisible(bool ena)
{
    libraryDetailsWidget()->platformGroupBox->setVisible(ena);
}

void LibraryDetailsController::setLinkageRadiosVisible(bool ena)
{
    m_linkageRadiosVisible = ena;
    libraryDetailsWidget()->staticRadio->setVisible(ena);
    libraryDetailsWidget()->dynamicRadio->setVisible(ena);
}

void LibraryDetailsController::setLinkageGroupVisible(bool ena)
{
    setLinkageRadiosVisible(ena);
    libraryDetailsWidget()->linkageGroupBox->setVisible(ena);
}

void LibraryDetailsController::setMacLibraryRadiosVisible(bool ena)
{
    m_macLibraryRadiosVisible = ena;
    libraryDetailsWidget()->frameworkRadio->setVisible(ena);
    libraryDetailsWidget()->libraryRadio->setVisible(ena);
}

void LibraryDetailsController::setMacLibraryGroupVisible(bool ena)
{
    setMacLibraryRadiosVisible(ena);
    libraryDetailsWidget()->macGroupBox->setVisible(ena);
}

void LibraryDetailsController::setLibraryPathChooserVisible(bool ena)
{
    libraryDetailsWidget()->libraryPathChooser->setVisible(ena);
    libraryDetailsWidget()->libraryFileLabel->setVisible(ena);
}

void LibraryDetailsController::setLibraryComboBoxVisible(bool ena)
{
    libraryDetailsWidget()->libraryComboBox->setVisible(ena);
    libraryDetailsWidget()->libraryLabel->setVisible(ena);
}

void LibraryDetailsController::setPackageLineEditVisible(bool ena)
{
    libraryDetailsWidget()->packageLineEdit->setVisible(ena);
    libraryDetailsWidget()->packageLabel->setVisible(ena);
}

void LibraryDetailsController::setIncludePathVisible(bool ena)
{
    m_includePathVisible = ena;
    libraryDetailsWidget()->includeLabel->setVisible(ena);
    libraryDetailsWidget()->includePathChooser->setVisible(ena);
}

void LibraryDetailsController::setWindowsGroupVisible(bool ena)
{
    m_windowsGroupVisible = ena;
    libraryDetailsWidget()->winGroupBox->setVisible(ena);
}

void LibraryDetailsController::setRemoveSuffixVisible(bool ena)
{
    libraryDetailsWidget()->removeSuffixCheckBox->setVisible(ena);
}

bool LibraryDetailsController::isMacLibraryRadiosVisible() const
{
    return m_macLibraryRadiosVisible;
}

bool LibraryDetailsController::isIncludePathVisible() const
{
    return m_includePathVisible;
}

bool LibraryDetailsController::isWindowsGroupVisible() const
{
    return m_windowsGroupVisible;
}

void LibraryDetailsController::slotIncludePathChanged()
{
    if (m_ignoreGuiSignals)
        return;
    m_includePathChanged = true;
}

void LibraryDetailsController::slotPlatformChanged()
{
    updateGui();
    emit completeChanged();
}

void LibraryDetailsController::slotMacLibraryTypeChanged()
{
    if (guiSignalsIgnored())
        return;

    if (m_linkageRadiosVisible
            && libraryDetailsWidget()->frameworkRadio->isChecked()) {
        setIgnoreGuiSignals(true);
        libraryDetailsWidget()->dynamicRadio->setChecked(true);
        setIgnoreGuiSignals(true);
    }

    updateGui();
}

void LibraryDetailsController::slotUseSubfoldersChanged(bool ena)
{
    if (ena) {
        libraryDetailsWidget()->addSuffixCheckBox->setChecked(false);
        libraryDetailsWidget()->removeSuffixCheckBox->setChecked(false);
    }
}

void LibraryDetailsController::slotAddSuffixChanged(bool ena)
{
    if (ena) {
        libraryDetailsWidget()->useSubfoldersCheckBox->setChecked(false);
        libraryDetailsWidget()->removeSuffixCheckBox->setChecked(false);
    }
}

// quote only when the string contains spaces
static QString smartQuote(const QString &aString)
{
    // The OS type is not important in that case, but use always the same
    // in order not to generate different quoting depending on host platform
    return Utils::QtcProcess::quoteArg(aString, Utils::OsTypeLinux);
}

static QString appendSeparator(const QString &aString)
{
    if (aString.isEmpty())
        return aString;
    if (aString.at(aString.size() - 1) == QLatin1Char('/'))
        return aString;
    return aString + QLatin1Char('/');
}

static QString windowsScopes(AddLibraryWizard::Platforms scopes)
{
    QString scopesString;
    QTextStream str(&scopesString);
    AddLibraryWizard::Platforms windowsPlatforms = scopes
            & (AddLibraryWizard::WindowsMinGWPlatform | AddLibraryWizard::WindowsMSVCPlatform);
    if (windowsPlatforms == AddLibraryWizard::WindowsMinGWPlatform)
        str << "win32-g++"; // mingw only
    else if (windowsPlatforms == AddLibraryWizard::WindowsMSVCPlatform)
        str << "win32:!win32-g++"; // msvc only
    else if (windowsPlatforms)
        str << "win32"; // both mingw and msvc
    return scopesString;
}

static QString commonScopes(AddLibraryWizard::Platforms scopes,
                            AddLibraryWizard::Platforms excludedScopes)
{
    QString scopesString;
    QTextStream str(&scopesString);
    AddLibraryWizard::Platforms common = scopes | excludedScopes;
    bool unixLikeScopes = false;
    if (scopes & ~QFlags<AddLibraryWizard::Platform>(AddLibraryWizard::WindowsMinGWPlatform
                                                     | AddLibraryWizard::WindowsMSVCPlatform)) {
        unixLikeScopes = true;
        if (common & AddLibraryWizard::LinuxPlatform) {
            str << "unix";
            if (!(common & AddLibraryWizard::MacPlatform))
                str << ":!macx";
        } else {
            if (scopes & AddLibraryWizard::MacPlatform)
                str << "macx";
        }
    }
    AddLibraryWizard::Platforms windowsPlatforms = scopes
            & (AddLibraryWizard::WindowsMinGWPlatform | AddLibraryWizard::WindowsMSVCPlatform);
    if (windowsPlatforms) {
        if (unixLikeScopes)
            str << "|";
        str << windowsScopes(windowsPlatforms);
    }
    return scopesString;
}

static QString generateLibsSnippet(AddLibraryWizard::Platforms platforms,
                     AddLibraryWizard::MacLibraryType macLibraryType,
                     const QString &libName,
                     const QString &targetRelativePath, const QString &pwd,
                     bool useSubfolders, bool addSuffix, bool generateLibPath)
{
    // it contains: $$[pwd]/
    const QString libraryPathSnippet = QLatin1String("$$") + pwd + QLatin1Char('/');

    AddLibraryWizard::Platforms commonPlatforms = platforms;
    if (macLibraryType == AddLibraryWizard::FrameworkType) // we will generate a separate -F -framework line
        commonPlatforms &= ~QFlags<AddLibraryWizard::Platform>(AddLibraryWizard::MacPlatform);
    if (useSubfolders || addSuffix) // we will generate a separate debug/release conditions
        commonPlatforms &= ~QFlags<AddLibraryWizard::Platform>(AddLibraryWizard::WindowsMinGWPlatform
                                                               | AddLibraryWizard::WindowsMSVCPlatform);

    AddLibraryWizard::Platforms diffPlatforms = platforms ^ commonPlatforms;
    AddLibraryWizard::Platforms generatedPlatforms = 0;

    QString snippetMessage;
    QTextStream str(&snippetMessage);

    AddLibraryWizard::Platforms windowsPlatforms = diffPlatforms
            & (AddLibraryWizard::WindowsMinGWPlatform | AddLibraryWizard::WindowsMSVCPlatform);
    if (windowsPlatforms) {
        QString windowsString = windowsScopes(windowsPlatforms);
        str << windowsString << ":CONFIG(release, debug|release): LIBS += ";
        if (useSubfolders) {
            if (generateLibPath)
                str << "-L" << libraryPathSnippet << smartQuote(targetRelativePath + QLatin1String("release/")) << ' ';
            str << "-l" << libName << "\n";
        } else if (addSuffix) {
            if (generateLibPath)
                str << "-L" << libraryPathSnippet << smartQuote(targetRelativePath) << ' ';
            str << "-l" << libName << "\n";
        }

        str << "else:" << windowsString << ":CONFIG(debug, debug|release): LIBS += ";
        if (useSubfolders) {
            if (generateLibPath)
                str << "-L" << libraryPathSnippet << smartQuote(targetRelativePath + QLatin1String("debug/")) << ' ';
            str << "-l" << libName << "\n";
        } else if (addSuffix) {
            if (generateLibPath)
                str << "-L" << libraryPathSnippet << smartQuote(targetRelativePath) << ' ';
            str << "-l" << libName << "d\n";
        }
        generatedPlatforms |= windowsPlatforms;
    }
    if (diffPlatforms & AddLibraryWizard::MacPlatform) {
        if (generatedPlatforms)
            str << "else:";
        str << "mac: LIBS += ";
        if (generateLibPath)
            str << "-F" << libraryPathSnippet << smartQuote(targetRelativePath) << ' ';
        str << "-framework " << libName << "\n";
        generatedPlatforms |= AddLibraryWizard::MacPlatform;
    }

    if (commonPlatforms) {
        if (generatedPlatforms)
            str << "else:";
        str << commonScopes(commonPlatforms, generatedPlatforms) << ": LIBS += ";
        if (generateLibPath)
            str << "-L" << libraryPathSnippet << smartQuote(targetRelativePath) << ' ';
        str << "-l" << libName << "\n";
    }
    return snippetMessage;
}

static QString generateIncludePathSnippet(const QString &includeRelativePath)
{
    return QLatin1String("\nINCLUDEPATH += $$PWD/")
            + smartQuote(includeRelativePath) + QLatin1Char('\n')
            + QLatin1String("DEPENDPATH += $$PWD/")
            + smartQuote(includeRelativePath) + QLatin1Char('\n');
}

static QString generatePreTargetDepsSnippet(AddLibraryWizard::Platforms platforms,
                                            AddLibraryWizard::LinkageType linkageType,
                                            const QString &libName,
                                            const QString &targetRelativePath, const QString &pwd,
                                            bool useSubfolders, bool addSuffix)
{
    if (linkageType != AddLibraryWizard::StaticLinkage)
        return QString();

    // it contains: PRE_TARGETDEPS += $$[pwd]/
    const QString preTargetDepsSnippet = QLatin1String("PRE_TARGETDEPS += $$") +
            pwd + QLatin1Char('/');

    QString snippetMessage;
    QTextStream str(&snippetMessage);
    str << "\n";
    AddLibraryWizard::Platforms generatedPlatforms = 0;
    AddLibraryWizard::Platforms windowsPlatforms = platforms
            & (AddLibraryWizard::WindowsMinGWPlatform | AddLibraryWizard::WindowsMSVCPlatform);
    AddLibraryWizard::Platforms commonPlatforms = platforms;
    if (useSubfolders || addSuffix) // we will generate a separate debug/release conditions, otherwise mingw is unix like
        commonPlatforms &= ~QFlags<AddLibraryWizard::Platform>(AddLibraryWizard::WindowsMinGWPlatform);
    commonPlatforms &= ~QFlags<AddLibraryWizard::Platform>(AddLibraryWizard::WindowsMSVCPlatform); // this case is different from all platforms
    if (windowsPlatforms) {
        if (useSubfolders || addSuffix) {
            if (windowsPlatforms & AddLibraryWizard::WindowsMinGWPlatform) {
                str << "win32-g++:CONFIG(release, debug|release): "
                    << preTargetDepsSnippet;
                if (useSubfolders)
                    str << smartQuote(targetRelativePath + QLatin1String("release/lib") + libName + QLatin1String(".a")) << '\n';
                else if (addSuffix)
                    str << smartQuote(targetRelativePath + QLatin1String("lib") + libName + QLatin1String(".a")) << '\n';

                str << "else:win32-g++:CONFIG(debug, debug|release): "
                    << preTargetDepsSnippet;
                if (useSubfolders)
                    str << smartQuote(targetRelativePath + QLatin1String("debug/lib") + libName + QLatin1String(".a")) << '\n';
                else if (addSuffix)
                    str << smartQuote(targetRelativePath + QLatin1String("lib") + libName + QLatin1String("d.a")) << '\n';
            }
            if (windowsPlatforms & AddLibraryWizard::WindowsMSVCPlatform) {
                if (windowsPlatforms & AddLibraryWizard::WindowsMinGWPlatform)
                    str << "else:";
                str << "win32:!win32-g++:CONFIG(release, debug|release): "
                    << preTargetDepsSnippet;
                if (useSubfolders)
                    str << smartQuote(targetRelativePath + QLatin1String("release/") + libName + QLatin1String(".lib")) << '\n';
                else if (addSuffix)
                    str << smartQuote(targetRelativePath + libName + QLatin1String(".lib")) << '\n';

                str << "else:win32:!win32-g++:CONFIG(debug, debug|release): "
                    << preTargetDepsSnippet;
                if (useSubfolders)
                    str << smartQuote(targetRelativePath + QLatin1String("debug/") + libName + QLatin1String(".lib")) << '\n';
                else if (addSuffix)
                    str << smartQuote(targetRelativePath + libName + QLatin1String("d.lib")) << '\n';
            }
            generatedPlatforms |= windowsPlatforms;
        } else {
            if (windowsPlatforms & AddLibraryWizard::WindowsMSVCPlatform) {
                str << "win32:!win32-g++: " << preTargetDepsSnippet
                    << smartQuote(targetRelativePath + libName + QLatin1String(".lib")) << "\n";
                generatedPlatforms |= AddLibraryWizard::WindowsMSVCPlatform; // mingw will be handled with common scopes
            }
            // mingw not generated yet, will be joined with unix like
        }
    }
    if (commonPlatforms) {
        if (generatedPlatforms)
            str << "else:";
        str << commonScopes(commonPlatforms, generatedPlatforms) << ": "
            << preTargetDepsSnippet << smartQuote(targetRelativePath + QLatin1String("lib") + libName + QLatin1String(".a")) << "\n";
    }
    return snippetMessage;
}

NonInternalLibraryDetailsController::NonInternalLibraryDetailsController(
        Ui::LibraryDetailsWidget *libraryDetails,
        const QString &proFile, QObject *parent) :
    LibraryDetailsController(libraryDetails, proFile, parent)
{
    setLibraryComboBoxVisible(false);
    setLibraryPathChooserVisible(true);

    libraryDetailsWidget()->libraryPathChooser
            ->setHistoryCompleter(QLatin1String("Qmake.LibDir.History"));

    if (creatorPlatform() == CreatorWindows) {
        libraryDetailsWidget()->libraryPathChooser->setPromptDialogFilter(
                QLatin1String("Library file (*.lib lib*.a)"));
        setLinkageRadiosVisible(true);
        setRemoveSuffixVisible(true);
    } else {
        setLinkageRadiosVisible(false);
        setRemoveSuffixVisible(false);
    }

    if (creatorPlatform() == CreatorLinux)
        libraryDetailsWidget()->libraryPathChooser->setPromptDialogFilter(
                QLatin1String("Library file (lib*.so lib*.a)"));

    if (creatorPlatform() == CreatorMac) {
        libraryDetailsWidget()->libraryPathChooser->setPromptDialogFilter(
                QLatin1String("Library file (*.dylib *.a *.framework)"));
           // QLatin1String("Library file (lib*.dylib lib*.a *.framework)"));
        libraryDetailsWidget()->libraryPathChooser->setExpectedKind(Utils::PathChooser::Any);
    } else {
        libraryDetailsWidget()->libraryPathChooser->setExpectedKind(Utils::PathChooser::File);
    }

    connect(libraryDetailsWidget()->libraryPathChooser, SIGNAL(validChanged(bool)),
            this, SIGNAL(completeChanged()));
    connect(libraryDetailsWidget()->libraryPathChooser, SIGNAL(rawPathChanged(QString)),
            this, SLOT(slotLibraryPathChanged()));
    connect(libraryDetailsWidget()->removeSuffixCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotRemoveSuffixChanged(bool)));
    connect(libraryDetailsWidget()->dynamicRadio, SIGNAL(clicked(bool)),
            this, SLOT(slotLinkageTypeChanged()));
    connect(libraryDetailsWidget()->staticRadio, SIGNAL(clicked(bool)),
            this, SLOT(slotLinkageTypeChanged()));
}

AddLibraryWizard::LinkageType NonInternalLibraryDetailsController::suggestedLinkageType() const
{
    AddLibraryWizard::LinkageType type = AddLibraryWizard::NoLinkage;
    if (creatorPlatform() != CreatorWindows) {
        if (libraryDetailsWidget()->libraryPathChooser->isValid()) {
            QFileInfo fi(libraryDetailsWidget()->libraryPathChooser->path());
            if (fi.suffix() == QLatin1String("a"))
                type = AddLibraryWizard::StaticLinkage;
            else
                type = AddLibraryWizard::DynamicLinkage;
        }
    }
    return type;
}

AddLibraryWizard::MacLibraryType NonInternalLibraryDetailsController::suggestedMacLibraryType() const
{
    AddLibraryWizard::MacLibraryType type = AddLibraryWizard::NoLibraryType;
    if (creatorPlatform() == CreatorMac) {
        if (libraryDetailsWidget()->libraryPathChooser->isValid()) {
            QFileInfo fi(libraryDetailsWidget()->libraryPathChooser->path());
            if (fi.suffix() == QLatin1String("framework"))
                type = AddLibraryWizard::FrameworkType;
            else
                type = AddLibraryWizard::LibraryType;
        }
    }
    return type;
}

QString NonInternalLibraryDetailsController::suggestedIncludePath() const
{
    QString includePath;
    if (libraryDetailsWidget()->libraryPathChooser->isValid()) {
        QFileInfo fi(libraryDetailsWidget()->libraryPathChooser->path());
        includePath = fi.absolutePath();
        QFileInfo dfi(includePath);
        // TODO: Win: remove debug or release folder first if appropriate
        if (dfi.fileName() == QLatin1String("lib")) {
            QDir dir = dfi.absoluteDir();
            includePath = dir.absolutePath();
            QDir includeDir(dir.absoluteFilePath(QLatin1String("include")));
            if (includeDir.exists())
                includePath = includeDir.absolutePath();
        }
    }
    return includePath;
}

void NonInternalLibraryDetailsController::updateWindowsOptionsEnablement()
{
    bool ena = platforms() & (AddLibraryWizard::WindowsMinGWPlatform | AddLibraryWizard::WindowsMSVCPlatform);
    if (creatorPlatform() == CreatorWindows) {
        libraryDetailsWidget()->addSuffixCheckBox->setEnabled(ena);
        ena = true;
    }
    libraryDetailsWidget()->winGroupBox->setEnabled(ena);
}

void NonInternalLibraryDetailsController::slotLinkageTypeChanged()
{
    if (guiSignalsIgnored())
        return;

    if (isMacLibraryRadiosVisible()
            && libraryDetailsWidget()->staticRadio->isChecked()) {
        setIgnoreGuiSignals(true);
        libraryDetailsWidget()->libraryRadio->setChecked(true);
        setIgnoreGuiSignals(true);
    }

    updateGui();
}

void NonInternalLibraryDetailsController::slotRemoveSuffixChanged(bool ena)
{
    if (ena) {
        libraryDetailsWidget()->useSubfoldersCheckBox->setChecked(false);
        libraryDetailsWidget()->addSuffixCheckBox->setChecked(false);
    }
}

void NonInternalLibraryDetailsController::slotLibraryPathChanged()
{
    if (creatorPlatform() == CreatorWindows) {
        bool subfoldersEnabled = true;
        bool removeSuffixEnabled = true;
        if (libraryDetailsWidget()->libraryPathChooser->isValid()) {
            QFileInfo fi(libraryDetailsWidget()->libraryPathChooser->path());
            QFileInfo dfi(fi.absolutePath());
            const QString parentFolderName = dfi.fileName().toLower();
            if (parentFolderName != QLatin1String("debug") &&
                parentFolderName != QLatin1String("release"))
                subfoldersEnabled = false;
            const QString baseName = fi.completeBaseName();

            if (baseName.isEmpty() || baseName.at(baseName.size() - 1).toLower() != QLatin1Char('d'))
                removeSuffixEnabled = false;

            if (subfoldersEnabled)
                libraryDetailsWidget()->useSubfoldersCheckBox->setChecked(true);
            else if (removeSuffixEnabled)
                libraryDetailsWidget()->removeSuffixCheckBox->setChecked(true);
            else
                libraryDetailsWidget()->addSuffixCheckBox->setChecked(true);
        }
    }

    updateGui();

    emit completeChanged();
}

bool NonInternalLibraryDetailsController::isComplete() const
{
    return libraryDetailsWidget()->libraryPathChooser->isValid() &&
           platforms();
}

QString NonInternalLibraryDetailsController::snippet() const
{
    QString libPath = libraryDetailsWidget()->libraryPathChooser->path();
    QFileInfo fi(libPath);
    QString libName;
    const bool removeSuffix = isWindowsGroupVisible()
            && libraryDetailsWidget()->removeSuffixCheckBox->isChecked();
    if (creatorPlatform() == CreatorWindows) {
        libName = fi.completeBaseName();
        if (removeSuffix && !libName.isEmpty()) // remove last letter which needs to be "d"
            libName = libName.left(libName.size() - 1);
        if (fi.completeSuffix() == QLatin1String("a")) // the mingw lib case
            libName = libName.mid(3); // cut the "lib" prefix
    } else if (creatorPlatform() == CreatorMac) {
        if (macLibraryType() == AddLibraryWizard::FrameworkType)
            libName = fi.completeBaseName();
        else
            libName = fi.completeBaseName().mid(3); // cut the "lib" prefix
    } else {
        libName = fi.completeBaseName().mid(3); // cut the "lib" prefix
    }

    bool useSubfolders = false;
    bool addSuffix = false;
    if (isWindowsGroupVisible()) {
        // when we are on Win but we don't generate the code for Win
        // we still need to remove "debug" or "release" subfolder
        const bool useSubfoldersCondition = (creatorPlatform() == CreatorWindows)
                                            ? true : platforms() & (AddLibraryWizard::WindowsMinGWPlatform
                                                                    | AddLibraryWizard::WindowsMSVCPlatform);
        if (useSubfoldersCondition)
            useSubfolders = libraryDetailsWidget()->useSubfoldersCheckBox->isChecked();
        if (platforms() & (AddLibraryWizard::WindowsMinGWPlatform | AddLibraryWizard::WindowsMSVCPlatform))
            addSuffix = libraryDetailsWidget()->addSuffixCheckBox->isChecked() || removeSuffix;
    }

    QString targetRelativePath;
    QString includeRelativePath;
    if (isIncludePathVisible()) { // generate also the path to lib
        QFileInfo pfi(proFile());
        QDir pdir = pfi.absoluteDir();
        QString absoluteLibraryPath = fi.absolutePath();
        if (creatorPlatform() == CreatorWindows && useSubfolders) { // drop last subfolder which needs to be "debug" or "release"
            QFileInfo libfi(absoluteLibraryPath);
            absoluteLibraryPath = libfi.absolutePath();
        }
        targetRelativePath = appendSeparator(pdir.relativeFilePath(absoluteLibraryPath));

        const QString includePath = libraryDetailsWidget()->includePathChooser->path();
        if (!includePath.isEmpty())
            includeRelativePath = pdir.relativeFilePath(includePath);
    }

    QString snippetMessage;
    QTextStream str(&snippetMessage);
    str << "\n";
    str << generateLibsSnippet(platforms(), macLibraryType(), libName,
                               targetRelativePath, QLatin1String("PWD"),
                               useSubfolders, addSuffix, isIncludePathVisible());
    if (isIncludePathVisible()) {
        str << generateIncludePathSnippet(includeRelativePath);
        str << generatePreTargetDepsSnippet(platforms(), linkageType(), libName,
                               targetRelativePath, QLatin1String("PWD"),
                               useSubfolders, addSuffix);
    }
    return snippetMessage;
}

/////////////

PackageLibraryDetailsController::PackageLibraryDetailsController(
    Ui::LibraryDetailsWidget *libraryDetails,
    const QString &proFile, QObject *parent)
    : NonInternalLibraryDetailsController(libraryDetails, proFile, parent)
{
    setPlatformsVisible(false);
    setIncludePathVisible(false);
    setWindowsGroupVisible(false);
    setLinkageGroupVisible(false);
    setMacLibraryGroupVisible(false);
    setLibraryPathChooserVisible(false);
    setPackageLineEditVisible(true);

    connect(libraryDetailsWidget()->packageLineEdit, SIGNAL(textChanged(QString)),
            this, SIGNAL(completeChanged()));

    updateGui();
}

bool PackageLibraryDetailsController::isComplete() const
{
    return !libraryDetailsWidget()->packageLineEdit->text().isEmpty();
}

QString PackageLibraryDetailsController::snippet() const
{
    QString snippetMessage;
    QTextStream str(&snippetMessage);
    str << "\n";
    if (!isLinkPackageGenerated())
        str << "unix: CONFIG += link_pkgconfig\n";
    str << "unix: PKGCONFIG += " << libraryDetailsWidget()->packageLineEdit->text() << "\n";
    return snippetMessage;
}

bool PackageLibraryDetailsController::isLinkPackageGenerated() const
{
    const Project *project = SessionManager::projectForFile(Utils::FileName::fromString(proFile()));
    if (!project)
        return false;

    const QmakeProFileNode *rootProject = dynamic_cast<const QmakeProFileNode *>(project->rootProjectNode());
    if (!rootProject)
        return false;

    const QmakeProFileNode *currentProject =
            rootProject->findProFileFor(Utils::FileName::fromString(proFile()));
    if (!currentProject)
        return false;

    const QStringList configVar = currentProject->variableValue(ConfigVar);
    if (configVar.contains(QLatin1String("link_pkgconfig")))
        return true;

    return false;
}

/////////////

SystemLibraryDetailsController::SystemLibraryDetailsController(
    Ui::LibraryDetailsWidget *libraryDetails,
    const QString &proFile, QObject *parent)
    : NonInternalLibraryDetailsController(libraryDetails, proFile, parent)
{
    setIncludePathVisible(false);
    setWindowsGroupVisible(false);

    updateGui();
}

/////////////

ExternalLibraryDetailsController::ExternalLibraryDetailsController(
    Ui::LibraryDetailsWidget *libraryDetails,
    const QString &proFile, QObject *parent)
    : NonInternalLibraryDetailsController(libraryDetails, proFile, parent)
{
    setIncludePathVisible(true);
    setWindowsGroupVisible(true);

    updateGui();
}

void ExternalLibraryDetailsController::updateWindowsOptionsEnablement()
{
    NonInternalLibraryDetailsController::updateWindowsOptionsEnablement();

    if (creatorPlatform() != CreatorWindows)
        return;

    bool subfoldersEnabled = true;
    bool removeSuffixEnabled = true;
    if (libraryDetailsWidget()->libraryPathChooser->isValid()) {
        QFileInfo fi(libraryDetailsWidget()->libraryPathChooser->path());
        QFileInfo dfi(fi.absolutePath());
        const QString parentFolderName = dfi.fileName().toLower();
        if (parentFolderName != QLatin1String("debug") &&
                parentFolderName != QLatin1String("release"))
            subfoldersEnabled = false;
        const QString baseName = fi.completeBaseName();

        if (baseName.isEmpty() || baseName.at(baseName.size() - 1).toLower() != QLatin1Char('d'))
            removeSuffixEnabled = false;

    }
    libraryDetailsWidget()->useSubfoldersCheckBox->setEnabled(subfoldersEnabled);
    libraryDetailsWidget()->removeSuffixCheckBox->setEnabled(removeSuffixEnabled);
}

/////////////

InternalLibraryDetailsController::InternalLibraryDetailsController(
        Ui::LibraryDetailsWidget *libraryDetails,
        const QString &proFile, QObject *parent)
    : LibraryDetailsController(libraryDetails, proFile, parent)
{
    setLinkageRadiosVisible(false);
    setLibraryPathChooserVisible(false);
    setLibraryComboBoxVisible(true);
    setIncludePathVisible(true);
    setWindowsGroupVisible(true);
    setRemoveSuffixVisible(false);

    if (creatorPlatform() == CreatorWindows)
        libraryDetailsWidget()->useSubfoldersCheckBox->setEnabled(true);

    connect(libraryDetailsWidget()->libraryComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotCurrentLibraryChanged()));

    updateProFile();
    updateGui();
}

AddLibraryWizard::LinkageType InternalLibraryDetailsController::suggestedLinkageType() const
{
    const int currentIndex = libraryDetailsWidget()->libraryComboBox->currentIndex();
    AddLibraryWizard::LinkageType type = AddLibraryWizard::NoLinkage;
    if (currentIndex >= 0) {
        QmakeProFileNode *proFileNode = m_proFileNodes.at(currentIndex);
        const QStringList configVar = proFileNode->variableValue(ConfigVar);
        if (configVar.contains(QLatin1String("staticlib"))
                || configVar.contains(QLatin1String("static")))
            type = AddLibraryWizard::StaticLinkage;
        else
            type = AddLibraryWizard::DynamicLinkage;
    }
    return type;
}

AddLibraryWizard::MacLibraryType InternalLibraryDetailsController::suggestedMacLibraryType() const
{
    const int currentIndex = libraryDetailsWidget()->libraryComboBox->currentIndex();
    AddLibraryWizard::MacLibraryType type = AddLibraryWizard::NoLibraryType;
    if (currentIndex >= 0) {
        QmakeProFileNode *proFileNode = m_proFileNodes.at(currentIndex);
        const QStringList configVar = proFileNode->variableValue(ConfigVar);
        if (configVar.contains(QLatin1String("lib_bundle")))
            type = AddLibraryWizard::FrameworkType;
        else
            type = AddLibraryWizard::LibraryType;
    }
    return type;
}

QString InternalLibraryDetailsController::suggestedIncludePath() const
{
    const int currentIndex = libraryDetailsWidget()->libraryComboBox->currentIndex();
    if (currentIndex >= 0) {
        QmakeProFileNode *proFileNode = m_proFileNodes.at(currentIndex);
        return proFileNode->filePath().toFileInfo().absolutePath();
    }
    return QString();
}

void InternalLibraryDetailsController::updateWindowsOptionsEnablement()
{
    if (creatorPlatform() == CreatorWindows)
        libraryDetailsWidget()->addSuffixCheckBox->setEnabled(true);
    libraryDetailsWidget()->winGroupBox->setEnabled(platforms()
                                & (AddLibraryWizard::WindowsMinGWPlatform | AddLibraryWizard::WindowsMSVCPlatform));
}

void InternalLibraryDetailsController::updateProFile()
{
    m_rootProjectPath.clear();
    m_proFileNodes.clear();
    libraryDetailsWidget()->libraryComboBox->clear();

    const Project *project = SessionManager::projectForFile(Utils::FileName::fromString(proFile()));
    if (!project)
        return;

    setIgnoreGuiSignals(true);

    ProjectExplorer::ProjectNode *rootProject = project->rootProjectNode();
    m_rootProjectPath = rootProject->filePath().toFileInfo().absolutePath();
    QDir rootDir(m_rootProjectPath);
    FindQmakeProFiles findQt4ProFiles;
    QList<QmakeProFileNode *> proFiles = findQt4ProFiles(rootProject);
    foreach (QmakeProFileNode *proFileNode, proFiles) {
        const QString proFilePath = proFileNode->filePath().toString();
        QmakeProjectManager::QmakeProjectType type = proFileNode->projectType();
        if (type == SharedLibraryTemplate || type == StaticLibraryTemplate) {
            const QStringList configVar = proFileNode->variableValue(ConfigVar);
            if (!configVar.contains(QLatin1String("plugin"))) {
                const QString relProFilePath = rootDir.relativeFilePath(proFilePath);
                TargetInformation targetInfo = proFileNode->targetInformation();
                const QString itemToolTip = QString::fromLatin1("%1 (%2)").arg(targetInfo.target).arg(relProFilePath);
                m_proFileNodes.append(proFileNode);
                libraryDetailsWidget()->libraryComboBox->addItem(targetInfo.target);
                libraryDetailsWidget()->libraryComboBox->setItemData(
                            libraryDetailsWidget()->libraryComboBox->count() - 1,
                            itemToolTip, Qt::ToolTipRole);
            }
        }
    }

    setIgnoreGuiSignals(false);
}

void InternalLibraryDetailsController::slotCurrentLibraryChanged()
{
    const int currentIndex = libraryDetailsWidget()->libraryComboBox->currentIndex();
    if (currentIndex >= 0) {
        libraryDetailsWidget()->libraryComboBox->setToolTip(
                    libraryDetailsWidget()->libraryComboBox->itemData(
                        currentIndex, Qt::ToolTipRole).toString());
        QmakeProFileNode *proFileNode = m_proFileNodes.at(currentIndex);
        const QStringList configVar = proFileNode->variableValue(ConfigVar);
        if (creatorPlatform() == CreatorWindows) {
            bool useSubfolders = false;
            if (configVar.contains(QLatin1String("debug_and_release"))
                && configVar.contains(QLatin1String("debug_and_release_target")))
                useSubfolders = true;
            libraryDetailsWidget()->useSubfoldersCheckBox->setChecked(useSubfolders);
            libraryDetailsWidget()->addSuffixCheckBox->setChecked(!useSubfolders);
        }
    }

    if (guiSignalsIgnored())
        return;

    updateGui();

    emit completeChanged();
}

bool InternalLibraryDetailsController::isComplete() const
{
    return libraryDetailsWidget()->libraryComboBox->count() && platforms();
}

QString InternalLibraryDetailsController::snippet() const
{
    const int currentIndex = libraryDetailsWidget()->libraryComboBox->currentIndex();

    if (currentIndex < 0)
        return QString();

    if (m_rootProjectPath.isEmpty())
        return QString();


    // dir of the root project
    QDir rootDir(m_rootProjectPath);

    // relative path for the project for which we insert the snippet,
    // it's relative to the root project
    const QString proRelavitePath = rootDir.relativeFilePath(proFile());

    // project for which we insert the snippet
    const Project *project = SessionManager::projectForFile(Utils::FileName::fromString(proFile()));

    // the build directory of the active build configuration
    QDir rootBuildDir = rootDir; // If the project is unconfigured use the project dir
    if (ProjectExplorer::Target *t = project->activeTarget())
        if (ProjectExplorer::BuildConfiguration *bc = t->activeBuildConfiguration())
            rootBuildDir = bc->buildDirectory().toString();

    // the project for which we insert the snippet inside build tree
    QFileInfo pfi(rootBuildDir.filePath(proRelavitePath));
    // the project dir for which we insert the snippet inside build tree
    QDir projectBuildDir(pfi.absolutePath());

    // current project node from combobox
    QFileInfo fi(proFile());
    QDir projectSrcDir(fi.absolutePath());

    // project node which we want to link against
    QmakeProFileNode *proFileNode = m_proFileNodes.at(currentIndex);
    TargetInformation targetInfo = proFileNode->targetInformation();

    const QString targetRelativePath = appendSeparator(projectBuildDir.relativeFilePath(targetInfo.buildDir));
    const QString includeRelativePath = projectSrcDir.relativeFilePath(libraryDetailsWidget()->includePathChooser->path());

    const bool useSubfolders = libraryDetailsWidget()->useSubfoldersCheckBox->isChecked();
    const bool addSuffix = libraryDetailsWidget()->addSuffixCheckBox->isChecked();

    QString snippetMessage;
    QTextStream str(&snippetMessage);
    str << "\n";

    // replace below to "PRI_OUT_PWD" when task QTBUG-13057 is done
    // (end enable adding libraries into .pri files as well).
    const QString outPwd = QLatin1String("OUT_PWD");
    str << generateLibsSnippet(platforms(), macLibraryType(), targetInfo.target,
                               targetRelativePath, outPwd,
                               useSubfolders, addSuffix, true);
    str << generateIncludePathSnippet(includeRelativePath);
    str << generatePreTargetDepsSnippet(platforms(), linkageType(), targetInfo.target,
                               targetRelativePath, outPwd,
                               useSubfolders, addSuffix);
    return snippetMessage;
}
