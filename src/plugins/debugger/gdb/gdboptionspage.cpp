/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchaintype.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>
#include <QtCore/QFileInfo>

namespace Debugger {
namespace Internal {

const char gdbBinariesSettingsGroupC[] = "GdbBinaries";
const char debugModeGdbBinaryKeyC[] = "GdbBinary";

GdbOptionsPage::GdbBinaryToolChainMap GdbOptionsPage::gdbBinaryToolChainMap;
bool GdbOptionsPage::gdbBinariesChanged = true;

void GdbOptionsPage::readGdbBinarySettings() /* static */
{
    using namespace ProjectExplorer;
    QSettings *settings = Core::ICore::instance()->settings();
    // Convert gdb binaries from flat settings list (see writeSettings)
    // into map ("binary1=gdb,1,2", "binary2=symbian_gdb,3,4").
    gdbBinaryToolChainMap.clear();
    const QChar separator = QLatin1Char(',');
    const QString keyRoot = QLatin1String(gdbBinariesSettingsGroupC) + QLatin1Char('/') +
                            QLatin1String(debugModeGdbBinaryKeyC);
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
            gdbBinariesChanged = true;
            const QString msg = QString::fromLatin1("Warning: The gdb binary '%1' does not exist, skipping.\n").arg(binary);
            qWarning("%s", qPrintable(msg));
            continue;
        }
        // Create entries for all toolchains.
        tokens.pop_front();
        foreach (const QString &t, tokens) {
            // Paranoia: Check if the there is already a binary configured for the toolchain.
            const int toolChain = t.toInt();
            const QString predefinedGdb = gdbBinaryToolChainMap.key(toolChain);
            if (predefinedGdb.isEmpty()) {
                gdbBinaryToolChainMap.insert(binary, toolChain);
            } else {
                const QString toolChainName =
                    ProjectExplorer::ToolChain::toolChainName(ToolChainType(toolChain));
                const QString msg =
                        QString::fromLatin1("An inconsistency has been encountered in the Ini-file '%1':\n"
                                            "Skipping gdb binary '%2' for toolchain '%3' as '%4' is already configured for it.").
                        arg(settings->fileName(), binary, toolChainName, predefinedGdb);
                qWarning("%s", qPrintable(msg));
            }
        }
    }
    // Linux defaults
#ifdef Q_OS_UNIX
    if (gdbBinaryToolChainMap.isEmpty()) {
        const QString gdb = QLatin1String("gdb");
        gdbBinaryToolChainMap.insert(gdb, ToolChain_GCC);
        gdbBinaryToolChainMap.insert(gdb, ToolChain_LINUX_ICC);
        gdbBinaryToolChainMap.insert(gdb, ToolChain_OTHER);
        gdbBinaryToolChainMap.insert(gdb, ToolChain_UNKNOWN);
    }
#endif
}

void GdbOptionsPage::writeGdbBinarySettings() /* static */
{
    QSettings *settings = Core::ICore::instance()->settings();
    // Convert gdb binaries map into a flat settings list of
    // ("binary1=gdb,1,2", "binary2=symbian_gdb,3,4"). It needs to be ASCII for installers
    QString lastBinary;
    QStringList settingsList;
    const QChar separator = QLatin1Char(',');
    const GdbBinaryToolChainMap::const_iterator cend = gdbBinaryToolChainMap.constEnd();
    for (GdbBinaryToolChainMap::const_iterator it = gdbBinaryToolChainMap.constBegin(); it != cend; ++it) {
        if (it.key() != lastBinary) {
            lastBinary = it.key(); // Start new entry with first toolchain
            settingsList.push_back(lastBinary);
        }
        settingsList.back().append(separator); // Append toolchain to last binary
        settingsList.back().append(QString::number(it.value()));
    }
    // Terminate settings list by an empty element such that consecutive keys resulting
    // from ini-file merging are suppressed while reading.
    settingsList.push_back(QString());
    // Write out list
    settings->beginGroup(QLatin1String(gdbBinariesSettingsGroupC));
    settings->remove(QString()); // remove all keys in group.
    const int count = settingsList.size();
    const QString keyRoot = QLatin1String(debugModeGdbBinaryKeyC);
    for (int i = 0; i < count; i++)
        settings->setValue(keyRoot + QString::number(i + 1), settingsList.at(i));
    settings->endGroup();
}

GdbOptionsPage::GdbOptionsPage()
    : m_ui(0)
{
}

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
    QWidget *w = new QWidget(parent);
    m_ui = new Ui::GdbOptionsPage;
    m_ui->setupUi(w);
    m_ui->gdbChooserWidget->setGdbBinaries(gdbBinaryToolChainMap);
    m_ui->scriptFileChooser->setExpectedKind(Utils::PathChooser::File);
    m_ui->scriptFileChooser->setPromptDialogTitle(tr("Choose Location of Startup Script File"));

    m_group.clear();
    m_group.insert(debuggerCore()->action(GdbScriptFile),
        m_ui->scriptFileChooser);
    m_group.insert(debuggerCore()->action(GdbEnvironment),
        m_ui->environmentEdit);
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

#if 1
    m_ui->groupBoxPluginDebugging->hide();
#else // The related code (handleAqcuiredInferior()) is disabled as well.
    m_group.insert(debuggerCore()->action(AllPluginBreakpoints),
        m_ui->radioButtonAllPluginBreakpoints);
    m_group.insert(debuggerCore()->action(SelectedPluginBreakpoints),
        m_ui->radioButtonSelectedPluginBreakpoints);
    m_group.insert(debuggerCore()->action(NoPluginBreakpoints),
        m_ui->radioButtonNoPluginBreakpoints);
    m_group.insert(debuggerCore()->action(SelectedPluginBreakpointsPattern),
        m_ui->lineEditSelectedPluginBreakpointsPattern);
#endif

    m_ui->lineEditSelectedPluginBreakpointsPattern->
        setEnabled(debuggerCore()->action(SelectedPluginBreakpoints)->value().toBool());
    connect(m_ui->radioButtonSelectedPluginBreakpoints, SIGNAL(toggled(bool)),
        m_ui->lineEditSelectedPluginBreakpointsPattern, SLOT(setEnabled(bool)));

    // FIXME
    m_ui->environmentEdit->hide();
    m_ui->labelEnvironment->hide();

    if (m_searchKeywords.isEmpty()) {
        QLatin1Char sep(' ');
        QTextStream(&m_searchKeywords)
                << sep << m_ui->groupBoxLocations->title()
                << sep << m_ui->labelEnvironment->text()
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
        gdbBinariesChanged = true;
        gdbBinaryToolChainMap = m_ui->gdbChooserWidget->gdbBinaries();
        m_ui->gdbChooserWidget->clearDirty();
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

} // namespace Internal
} // namespace Debugger
