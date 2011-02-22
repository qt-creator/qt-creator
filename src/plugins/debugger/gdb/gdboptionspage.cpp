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

#include "gdboptionspage.h"
#include "debuggeractions.h"
#include "debuggercore.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/abi.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QFileInfo>

using namespace ProjectExplorer;

namespace Debugger {
namespace Internal {

static const char *GDB_MAPPING_ARRAY = "GdbMapping";
static const char *GDB_ABI_KEY = "Abi";
static const char *GDB_BINARY_KEY = "Binary";

GdbOptionsPage::GdbBinaryToolChainMap GdbOptionsPage::abiToGdbMap;
bool GdbOptionsPage::gdbMappingChanged = true;

void GdbOptionsPage::readGdbSettings() /* static */
{
    // FIXME: Convert old settings!
    QSettings *settings = Core::ICore::instance()->settings();

    abiToGdbMap.clear();

    int size = settings->beginReadArray(GDB_MAPPING_ARRAY);
    for (int i = 0; i < size; ++i) {
        settings->setArrayIndex(i);
        Abi abi(settings->value(GDB_ABI_KEY).toString());
        if (!abi.isValid())
            continue;
        QString binary = settings->value(GDB_BINARY_KEY).toString();
        if (binary.isEmpty())
            continue;
        abiToGdbMap.insert(abi.toString(), binary);
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
            if (abi.isEmpty() || abiToGdbMap.contains(abi))
                continue;

            abiToGdbMap.insert(abi, binary);
        }
    }

    gdbMappingChanged = false;
}

void GdbOptionsPage::writeGdbSettings() /* static */
{
    // FIXME: This should actually get called in response to ICore::saveSettingsRequested()
    if (!gdbMappingChanged)
        return;

    QSettings *settings = Core::ICore::instance()->settings();

    settings->beginWriteArray(GDB_MAPPING_ARRAY);

    int index = 0;
    for (QMap<QString, QString>::const_iterator i = abiToGdbMap.constBegin();
         i != abiToGdbMap.constEnd(); ++i) {
        if (i.value().isEmpty())
            continue;

        settings->setArrayIndex(index);
        ++index;

        settings->setValue(GDB_ABI_KEY, i.key());
        settings->setValue(GDB_BINARY_KEY, i.value());
    }
    settings->endArray();

    gdbMappingChanged = false;
}

GdbOptionsPage::GdbOptionsPage()
    : m_ui(0)
{ }

QString GdbOptionsPage::settingsId()
{
    return QLatin1String("M.Gdb");
}

QString GdbOptionsPage::displayName() const
{
    return tr("Gdb");
}

QString GdbOptionsPage::category() const
{
    return QLatin1String(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
}

QString GdbOptionsPage::displayCategory() const
{
    return QCoreApplication::translate("Debugger", Debugger::Constants::DEBUGGER_SETTINGS_TR_CATEGORY);
}

QIcon GdbOptionsPage::categoryIcon() const
{
    return QIcon(QLatin1String(Debugger::Constants::DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

QWidget *GdbOptionsPage::createPage(QWidget *parent)
{
    // Fix up abi mapping now that the ToolChainManager is available:
    connect(ToolChainManager::instance(), SIGNAL(toolChainAdded(ProjectExplorer::ToolChain*)),
            this, SLOT(handleToolChainAdditions(ProjectExplorer::ToolChain*)));
    connect(ToolChainManager::instance(), SIGNAL(toolChainRemoved(ProjectExplorer::ToolChain*)),
            this, SLOT(handleToolChainRemovals(ProjectExplorer::ToolChain*)));

    // Update mapping now that toolchains are available
    QList<ToolChain *> tcs =
            ToolChainManager::instance()->toolChains();

    QStringList abiList;
    foreach (ToolChain *tc, tcs) {
        const QString abi = tc->targetAbi().toString();
        if (!abiList.contains(abi))
            abiList.append(abi);
        if (!abiToGdbMap.contains(abi))
            handleToolChainAdditions(tc);
    }

    QStringList toRemove;
    for (QMap<QString, QString>::const_iterator i = abiToGdbMap.constBegin();
         i != abiToGdbMap.constEnd(); ++i) {
        if (!abiList.contains(i.key()))
            toRemove.append(i.key());
    }

    foreach (const QString &key, toRemove)
        abiToGdbMap.remove(key);

    // Actual page setup:
    QWidget *w = new QWidget(parent);
    m_ui = new Ui::GdbOptionsPage;
    m_ui->setupUi(w);
    m_ui->gdbChooserWidget->setGdbMapping(abiToGdbMap);

    m_ui->scriptFileChooser->setExpectedKind(Utils::PathChooser::File);
    m_ui->scriptFileChooser->setPromptDialogTitle(tr("Choose Location of Startup Script File"));

    m_group.clear();
    m_group.insert(debuggerCore()->action(GdbScriptFile),
        m_ui->scriptFileChooser);
    m_group.insert(debuggerCore()->action(LoadGdbInit),
        m_ui->checkBoxLoadGdbInit);
    m_group.insert(debuggerCore()->action(TargetAsync),
        m_ui->checkBoxTargetAsync);
    m_group.insert(debuggerCore()->action(AdjustBreakpointLocations),
        m_ui->checkBoxAdjustBreakpointLocations);
    m_group.insert(debuggerCore()->action(GdbWatchdogTimeout),
        m_ui->spinBoxGdbWatchdogTimeout);

    m_group.insert(debuggerCore()->action(UseMessageBoxForSignals),
        m_ui->checkBoxUseMessageBoxForSignals);
    m_group.insert(debuggerCore()->action(SkipKnownFrames),
        m_ui->checkBoxSkipKnownFrames);
    m_group.insert(debuggerCore()->action(EnableReverseDebugging),
        m_ui->checkBoxEnableReverseDebugging);
    m_group.insert(debuggerCore()->action(GdbWatchdogTimeout), 0);

    m_ui->groupBoxPluginDebugging->hide();

    m_ui->lineEditSelectedPluginBreakpointsPattern->
        setEnabled(debuggerCore()->action(SelectedPluginBreakpoints)->value().toBool());
    connect(m_ui->radioButtonSelectedPluginBreakpoints, SIGNAL(toggled(bool)),
        m_ui->lineEditSelectedPluginBreakpointsPattern, SLOT(setEnabled(bool)));

    if (m_searchKeywords.isEmpty()) {
        QLatin1Char sep(' ');
        QTextStream(&m_searchKeywords)
                << sep << m_ui->groupBoxLocations->title()
                << sep << m_ui->checkBoxLoadGdbInit->text()
                << sep << m_ui->checkBoxTargetAsync->text()
                << sep << m_ui->labelGdbStartupScript->text()
                << sep << m_ui->labelGdbWatchdogTimeout->text()
                << sep << m_ui->checkBoxEnableReverseDebugging->text()
                << sep << m_ui->checkBoxSkipKnownFrames->text()
                << sep << m_ui->checkBoxUseMessageBoxForSignals->text()
                << sep << m_ui->checkBoxAdjustBreakpointLocations->text()
                << sep << m_ui->groupBoxPluginDebugging->title()
                << sep << m_ui->radioButtonAllPluginBreakpoints->text()
                << sep << m_ui->radioButtonSelectedPluginBreakpoints->text()
                << sep << m_ui->labelSelectedPluginBreakpoints->text()
                << sep << m_ui->radioButtonNoPluginBreakpoints->text()
                   ;
        m_searchKeywords.remove(QLatin1Char('&'));
    }
    return w;
}

void GdbOptionsPage::apply()
{
    if (!m_ui) // page never shown
        return;

    m_group.apply(Core::ICore::instance()->settings());
    if (m_ui->gdbChooserWidget->isDirty()) {
        abiToGdbMap = m_ui->gdbChooserWidget->gdbMapping();
        m_ui->gdbChooserWidget->setGdbMapping(abiToGdbMap);
        gdbMappingChanged = true;
    }
}

void GdbOptionsPage::finish()
{
    if (!m_ui) // page never shown
        return;
    delete m_ui;
    m_ui = 0;
    m_group.finish();
}

bool GdbOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

void GdbOptionsPage::handleToolChainAdditions(ToolChain *tc)
{
    Abi tcAbi = tc->targetAbi();

    if (tcAbi.binaryFormat() != Abi::Format_ELF
            && tcAbi.binaryFormat() != Abi::Format_Mach_O
            && !( tcAbi.os() == Abi::Windows
                  && tcAbi.osFlavor() == Abi::Windows_msys ))
        return;
    if (abiToGdbMap.contains(tcAbi.toString()))
        return;

    QString binary;
#ifdef Q_OS_UNIX
    Abi hostAbi = Abi::hostAbi();
    if (hostAbi == tcAbi)
        binary = QLatin1String("gdb");
#endif
    abiToGdbMap.insert(tc->targetAbi().toString(), binary);
}

void GdbOptionsPage::handleToolChainRemovals(ToolChain *tc)
{
    QList<ToolChain *> tcs = ToolChainManager::instance()->toolChains();
    foreach (ToolChain *current, tcs) {
        if (current->targetAbi() == tc->targetAbi())
            return;
    }

    abiToGdbMap.remove(tc->targetAbi().toString());
}

} // namespace Internal
} // namespace Debugger
