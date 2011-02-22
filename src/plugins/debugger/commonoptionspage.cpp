/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "commonoptionspage.h"

#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"

#include <coreplugin/icore.h>
#include <coreplugin/manhattanstyle.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/abi.h>

#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>

using namespace Core;
using namespace Debugger::Constants;
using namespace ProjectExplorer;

namespace Debugger {
namespace Internal {

static const char *DEBUGGER_MAPPING_ARRAY = "GdbMapping";
static const char *DEBUGGER_ABI_KEY = "Abi";
static const char *DEBUGGER_BINARY_KEY = "Binary";

///////////////////////////////////////////////////////////////////////
//
// CommonOptionsPage
//
///////////////////////////////////////////////////////////////////////

CommonOptionsPage::CommonOptionsPage()
{
    m_abiToDebuggerMapChanged = true;
}

QString CommonOptionsPage::id() const
{
    return _(DEBUGGER_COMMON_SETTINGS_ID);
}

QString CommonOptionsPage::displayName() const
{
    return QCoreApplication::translate("Debugger", DEBUGGER_COMMON_SETTINGS_NAME);}

QString CommonOptionsPage::category() const
{
    return _(DEBUGGER_SETTINGS_CATEGORY);
}

QString CommonOptionsPage::displayCategory() const
{
    return QCoreApplication::translate("Debugger", DEBUGGER_SETTINGS_TR_CATEGORY);}

QIcon CommonOptionsPage::categoryIcon() const
{
    return QIcon(QLatin1String(DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

void CommonOptionsPage::apply()
{
    m_group.apply(ICore::instance()->settings());

    if (m_ui.debuggerChooserWidget->isDirty()) {
        m_abiToDebuggerMap = m_ui.debuggerChooserWidget->debuggerMapping();
        //m_ui.debuggerChooserWidget->setDebuggerMapping(m_abiToDebuggerMap);
        m_abiToDebuggerMapChanged = true;
    }
}

void CommonOptionsPage::finish()
{
    m_group.finish();
}

QString CommonOptionsPage::debuggerForAbi(const QString &abi) const
{
    return m_abiToDebuggerMap.value(abi);
}

QWidget *CommonOptionsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);
    m_group.clear();

    m_group.insert(debuggerCore()->action(ListSourceFiles),
        m_ui.checkBoxListSourceFiles);
    m_group.insert(debuggerCore()->action(UseAlternatingRowColors),
        m_ui.checkBoxUseAlternatingRowColors);
    m_group.insert(debuggerCore()->action(UseToolTipsInMainEditor),
        m_ui.checkBoxUseToolTipsInMainEditor);
    m_group.insert(debuggerCore()->action(CloseBuffersOnExit),
        m_ui.checkBoxCloseBuffersOnExit);
    m_group.insert(debuggerCore()->action(SwitchModeOnExit),
        m_ui.checkBoxSwitchModeOnExit);
    m_group.insert(debuggerCore()->action(AutoDerefPointers), 0);
    m_group.insert(debuggerCore()->action(UseToolTipsInLocalsView), 0);
    m_group.insert(debuggerCore()->action(UseToolTipsInBreakpointsView), 0);
    m_group.insert(debuggerCore()->action(UseAddressInBreakpointsView), 0);
    m_group.insert(debuggerCore()->action(UseAddressInStackView), 0);
    m_group.insert(debuggerCore()->action(MaximalStackDepth),
        m_ui.spinBoxMaximalStackDepth);
    m_group.insert(debuggerCore()->action(ShowStdNamespace), 0);
    m_group.insert(debuggerCore()->action(ShowQtNamespace), 0);
    m_group.insert(debuggerCore()->action(SortStructMembers), 0);
    m_group.insert(debuggerCore()->action(LogTimeStamps), 0);
    m_group.insert(debuggerCore()->action(VerboseLog), 0);
    m_group.insert(debuggerCore()->action(BreakOnThrow), 0);
    m_group.insert(debuggerCore()->action(BreakOnCatch), 0);
#ifdef Q_OS_WIN
    Utils::SavedAction *registerAction = debuggerCore()->action(RegisterForPostMortem);
    m_group.insert(registerAction,
        m_ui.checkBoxRegisterForPostMortem);
    connect(registerAction, SIGNAL(toggled(bool)),
            m_ui.checkBoxRegisterForPostMortem, SLOT(setChecked(bool)));
#endif

    if (m_searchKeywords.isEmpty()) {
        QLatin1Char sep(' ');
        QTextStream(&m_searchKeywords)
                << sep << m_ui.checkBoxUseAlternatingRowColors->text()
                << sep << m_ui.checkBoxUseToolTipsInMainEditor->text()
                << sep << m_ui.checkBoxListSourceFiles->text()
#ifdef Q_OS_WIN
                << sep << m_ui.checkBoxRegisterForPostMortem->text()
#endif
                << sep << m_ui.checkBoxCloseBuffersOnExit->text()
                << sep << m_ui.checkBoxSwitchModeOnExit->text()
                << sep << m_ui.labelMaximalStackDepth->text()
                   ;
        m_searchKeywords.remove(QLatin1Char('&'));
    }
#ifndef Q_OS_WIN
    m_ui.checkBoxRegisterForPostMortem->setVisible(false);
#endif

    // Tool
    connect(ToolChainManager::instance(),
        SIGNAL(toolChainAdded(ProjectExplorer::ToolChain*)),
        SLOT(handleToolChainAdditions(ProjectExplorer::ToolChain*)));

    connect(ToolChainManager::instance(),
        SIGNAL(toolChainRemoved(ProjectExplorer::ToolChain*)),
        SLOT(handleToolChainRemovals(ProjectExplorer::ToolChain*)));

    // Update mapping now that toolchains are available
    QList<ToolChain *> tcs = ToolChainManager::instance()->toolChains();

    QStringList abiList;
    foreach (ToolChain *tc, tcs) {
        const QString abi = tc->targetAbi().toString();
        if (!abiList.contains(abi))
            abiList.append(abi);
        if (!m_abiToDebuggerMap.contains(abi))
            handleToolChainAdditions(tc);
    }

    QStringList toRemove;
    for (QMap<QString, QString>::const_iterator i = m_abiToDebuggerMap.constBegin();
         i != m_abiToDebuggerMap.constEnd(); ++i) {
        if (!abiList.contains(i.key()))
            toRemove.append(i.key());
    }

    foreach (const QString &key, toRemove)
        m_abiToDebuggerMap.remove(key);

    m_ui.debuggerChooserWidget->setDebuggerMapping(m_abiToDebuggerMap);

    return w;
}

bool CommonOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

void CommonOptionsPage::readSettings() /* static */
{
    // FIXME: Convert old settings!
    QSettings *settings = Core::ICore::instance()->settings();

    m_abiToDebuggerMap.clear();

    int size = settings->beginReadArray(DEBUGGER_MAPPING_ARRAY);
    for (int i = 0; i < size; ++i) {
        settings->setArrayIndex(i);
        Abi abi(settings->value(DEBUGGER_ABI_KEY).toString());
        if (!abi.isValid())
            continue;
        QString binary = settings->value(DEBUGGER_BINARY_KEY).toString();
        if (binary.isEmpty())
            continue;
        m_abiToDebuggerMap.insert(abi.toString(), binary);
    }
    settings->endArray();

    // Map old settings (pre 2.2):
    const QChar separator = QLatin1Char(',');
    const QString keyRoot = QLatin1String("GdbBinaries/GdbBinaries");
    for (int i = 1; ; i++) {
        const QString value = settings->value(keyRoot + QString::number(i)).toString();
        if (value.isEmpty())
            break;
        // Split apart comma-separated binary and its numerical toolchains.
        QStringList tokens = value.split(separator);
        if (tokens.size() < 2)
            break;

        const QString binary = tokens.front();
        // Skip non-existent absolute binaries allowing for upgrades by the installer.
        // Force a rewrite of the settings file.
        const QFileInfo binaryInfo(binary);
        if (binaryInfo.isAbsolute() && !binaryInfo.isExecutable()) {
            const QString msg = QString::fromLatin1("Warning: The gdb binary '%1' does not exist, skipping.\n").arg(binary);
            qWarning("%s", qPrintable(msg));
            continue;
        }

        // Create entries for all toolchains.
        tokens.pop_front();
        foreach (const QString &t, tokens) {
            // Paranoia: Check if the there is already a binary configured for the toolchain.
            QString abi;
            switch (t.toInt())
            {
            case 0: // GCC
            case 1: // Linux ICC
#ifndef Q_OS_WIN
                abi = Abi::hostAbi().toString();
#endif
                break;
            case 2: // MinGW
            case 3: // MSVC
            case 4: // WINCE
#ifdef Q_OS_WIN
                abi = Abi::hostAbi().toString();
#endif
                break;
            case 5: // WINSCW
                abi = Abi(Abi::ARM, Abi::Symbian,
                                           Abi::Symbian_emulator,
                                           Abi::Format_ELF,
                                           32).toString();
                break;
            case 6: // GCCE
            case 7: // RVCT 2, ARM v5
            case 8: // RVCT 2, ARM v6
            case 11: // RVCT GNUPOC
            case 12: // RVCT 4, ARM v5
            case 13: // RVCT 4, ARM v6
                abi = Abi(Abi::ARM, Abi::Symbian,
                                           Abi::Symbian_device,
                                           Abi::Format_ELF,
                                           32).toString();
                break;
            case 9: // GCC Maemo5
                abi = Abi(Abi::ARM, Abi::Linux,
                                           Abi::Linux_maemo,
                                           Abi::Format_ELF,
                                           32).toString();

                break;
            case 14: // GCC Harmattan
                abi = Abi(Abi::ARM, Abi::Linux,
                                           Abi::Linux_harmattan,
                                           Abi::Format_ELF,
                                           32).toString();
                break;
            case 15: // GCC Meego
                abi = Abi(Abi::ARM, Abi::Linux,
                                           Abi::Linux_meego,
                                           Abi::Format_ELF,
                                           32).toString();
                break;
            default:
                break;
            }
            if (abi.isEmpty() || m_abiToDebuggerMap.contains(abi))
                continue;

            m_abiToDebuggerMap.insert(abi, binary);
        }
    }

    m_abiToDebuggerMapChanged = false;
}

void CommonOptionsPage::writeSettings() /* static */
{
    if (!m_abiToDebuggerMapChanged)
        return;

    QSettings *settings = Core::ICore::instance()->settings();

    settings->beginWriteArray(DEBUGGER_MAPPING_ARRAY);

    int index = 0;
    for (QMap<QString, QString>::const_iterator i = m_abiToDebuggerMap.constBegin();
         i != m_abiToDebuggerMap.constEnd(); ++i) {
        if (i.value().isEmpty())
            continue;

        settings->setArrayIndex(index);
        ++index;

        settings->setValue(DEBUGGER_ABI_KEY, i.key());
        settings->setValue(DEBUGGER_BINARY_KEY, i.value());
    }
    settings->endArray();

    m_abiToDebuggerMapChanged = false;
}

void CommonOptionsPage::handleToolChainAdditions(ToolChain *tc)
{
    Abi tcAbi = tc->targetAbi();

    if (tcAbi.binaryFormat() != Abi::Format_ELF
            && tcAbi.binaryFormat() != Abi::Format_Mach_O
            && !( tcAbi.os() == Abi::Windows
                  && tcAbi.osFlavor() == Abi::Windows_msys ))
        return;
    if (m_abiToDebuggerMap.contains(tcAbi.toString()))
        return;

    QString binary;
#ifdef Q_OS_UNIX
    Abi hostAbi = Abi::hostAbi();
    if (hostAbi == tcAbi)
        binary = QLatin1String("gdb");
#endif
    m_abiToDebuggerMap.insert(tc->targetAbi().toString(), binary);
}

void CommonOptionsPage::handleToolChainRemovals(ToolChain *tc)
{
    QList<ToolChain *> tcs = ToolChainManager::instance()->toolChains();
    foreach (ToolChain *current, tcs) {
        if (current->targetAbi() == tc->targetAbi())
            return;
    }

    m_abiToDebuggerMap.remove(tc->targetAbi().toString());
}


///////////////////////////////////////////////////////////////////////
//
// DebuggingHelperOptionPage
//
///////////////////////////////////////////////////////////////////////

static bool oxygenStyle()
{
    const ManhattanStyle *ms = qobject_cast<const ManhattanStyle *>(qApp->style());
    return ms && !qstrcmp("OxygenStyle", ms->baseStyle()->metaObject()->className());
}

QString DebuggingHelperOptionPage::id() const
{
    return _("Z.DebuggingHelper");
}

QString DebuggingHelperOptionPage::displayName() const
{
    return tr("Debugging Helper");
}

QString DebuggingHelperOptionPage::category() const
{
    return _(DEBUGGER_SETTINGS_CATEGORY);
}

QString DebuggingHelperOptionPage::displayCategory() const
{
    return QCoreApplication::translate("Debugger", DEBUGGER_SETTINGS_TR_CATEGORY);
}

QIcon DebuggingHelperOptionPage::categoryIcon() const
{
    return QIcon(QLatin1String(DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

void DebuggingHelperOptionPage::apply()
{
    m_group.apply(ICore::instance()->settings());
}

void DebuggingHelperOptionPage::finish()
{
    m_group.finish();
}

QWidget *DebuggingHelperOptionPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);

    m_ui.dumperLocationChooser->setExpectedKind(Utils::PathChooser::Command);
    m_ui.dumperLocationChooser->setPromptDialogTitle(tr("Choose DebuggingHelper Location"));
    m_ui.dumperLocationChooser->setInitialBrowsePathBackup(
        ICore::instance()->resourcePath() + "../../lib");

    m_group.clear();
    m_group.insert(debuggerCore()->action(UseDebuggingHelpers),
        m_ui.debuggingHelperGroupBox);
    m_group.insert(debuggerCore()->action(UseCustomDebuggingHelperLocation),
        m_ui.customLocationGroupBox);
    // Suppress Oxygen style's giving flat group boxes bold titles.
    if (oxygenStyle())
        m_ui.customLocationGroupBox->setStyleSheet(_("QGroupBox::title { font: ; }"));

    m_group.insert(debuggerCore()->action(CustomDebuggingHelperLocation),
        m_ui.dumperLocationChooser);

    m_group.insert(debuggerCore()->action(UseCodeModel),
        m_ui.checkBoxUseCodeModel);
    m_group.insert(debuggerCore()->action(ShowThreadNames),
        m_ui.checkBoxShowThreadNames);


#ifndef QT_DEBUG
#if 0
    cmd = am->registerAction(m_dumpLogAction,
        DUMP_LOG, globalcontext);
    //cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+D,Ctrl+L")));
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+F11")));
    mdebug->addAction(cmd);
#endif
#endif

    if (m_searchKeywords.isEmpty()) {
        QTextStream(&m_searchKeywords)
                << ' ' << m_ui.debuggingHelperGroupBox->title()
                << ' ' << m_ui.customLocationGroupBox->title()
                << ' ' << m_ui.dumperLocationLabel->text()
                << ' ' << m_ui.checkBoxUseCodeModel->text()
                << ' ' << m_ui.checkBoxShowThreadNames->text();
        m_searchKeywords.remove(QLatin1Char('&'));
    }
    return w;
}

bool DebuggingHelperOptionPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}


} // namespace Internal
} // namespace Debugger
