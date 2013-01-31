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

#include "abiwidget.h"
#include "customtoolchain.h"
#include "gccparser.h"
#include "projectexplorerconstants.h"
#include "toolchainmanager.h"

#include <utils/detailswidget.h>
#include <utils/environment.h>
#include <utils/pathchooser.h>

#include <QFormLayout>
#include <QPlainTextEdit>
#include <QLineEdit>

using namespace Utils;

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------------

static const char compilerCommandKeyC[] = "ProjectExplorer.CustomToolChain.CompilerPath";
static const char makeCommandKeyC[] = "ProjectExplorer.CustomToolChain.MakePath";
static const char targetAbiKeyC[] = "ProjectExplorer.CustomToolChain.TargetAbi";
static const char supportedAbisKeyC[] = "ProjectExplorer.CustomToolChain.SupportedAbis";
static const char predefinedMacrosKeyC[] = "ProjectExplorer.CustomToolChain.PredefinedMacros";
static const char headerPathsKeyC[] = "ProjectExplorer.CustomToolChain.HeaderPaths";
static const char cxx11FlagsKeyC[] = "ProjectExplorer.CustomToolChain.Cxx11Flags";
static const char mkspecsKeyC[] = "ProjectExplorer.CustomToolChain.Mkspecs";

// --------------------------------------------------------------------------
// CustomToolChain
// --------------------------------------------------------------------------

CustomToolChain::CustomToolChain(bool autodetect) :
    ToolChain(QLatin1String(Constants::CUSTOM_TOOLCHAIN_ID), autodetect)
{ }

CustomToolChain::CustomToolChain(const QString &id, bool autodetect) :
    ToolChain(id, autodetect)
{ }

CustomToolChain::CustomToolChain(const CustomToolChain &tc) :
    ToolChain(tc),
    m_compilerCommand(tc.m_compilerCommand),
    m_makeCommand(tc.m_makeCommand),
    m_targetAbi(tc.m_targetAbi),
    m_predefinedMacros(tc.m_predefinedMacros),
    m_systemHeaderPaths(tc.m_systemHeaderPaths)
{ }

QString CustomToolChain::type() const
{
    return QLatin1String("custom");
}

QString CustomToolChain::typeDisplayName() const
{
    return Internal::CustomToolChainFactory::tr("Custom");
}

Abi CustomToolChain::targetAbi() const
{
    return m_targetAbi;
}

void CustomToolChain::setTargetAbi(const Abi &abi)
{
    if (abi == m_targetAbi)
        return;

    m_targetAbi = abi;
    toolChainUpdated();
}

bool CustomToolChain::isValid() const
{
    return true;
}

QByteArray CustomToolChain::predefinedMacros(const QStringList &cxxflags) const
{
    QByteArray result;
    QStringList macros = m_predefinedMacros;
    foreach (const QString &cxxFlag, cxxflags) {
        if (cxxFlag.startsWith(QLatin1String("-D"))) {
            macros << cxxFlag.mid(2).trimmed();
        } else if (cxxFlag.startsWith(QLatin1String("-U"))) {
            const QString &removedName = cxxFlag.mid(2).trimmed();
            for (int i = macros.size() - 1; i >= 0; --i) {
                const QString &m = macros.at(i);
                if (m.left(m.indexOf(QLatin1Char('='))) == removedName)
                    macros.removeAt(i);
            }
        }
    }
    foreach (const QString &str, macros) {
        QByteArray ba = str.toUtf8();
        int equals = ba.indexOf('=');
        if (equals == -1) {
            result += "#define " + ba.trimmed() + '\n';
        } else {
            result += "#define " + ba.left(equals).trimmed() + ' '
                    + ba.mid(equals + 1).trimmed() + '\n';
        }
    }
    return result;
}

ToolChain::CompilerFlags CustomToolChain::compilerFlags(const QStringList &cxxflags) const
{
    foreach (const QString &cxx11Flag, m_cxx11Flags)
        if (cxxflags.contains(cxx11Flag))
            return STD_CXX11;
    return NO_FLAGS;
}

const QStringList &CustomToolChain::rawPredefinedMacros() const
{
    return m_predefinedMacros;
}

void CustomToolChain::setPredefinedMacros(const QStringList &list)
{
    m_predefinedMacros = list;
}

QList<HeaderPath> CustomToolChain::systemHeaderPaths(const QStringList &cxxFlags, const Utils::FileName &) const
{
    QList<HeaderPath> flagHeaderPaths;
    foreach (const QString &cxxFlag, cxxFlags) {
        if (cxxFlag.startsWith(QLatin1String("-I")))
            flagHeaderPaths << HeaderPath(cxxFlag.mid(2).trimmed(), HeaderPath::GlobalHeaderPath);
    }

    return m_systemHeaderPaths + flagHeaderPaths;
}

void CustomToolChain::addToEnvironment(Environment &env) const
{
    if (!m_compilerCommand.isEmpty()) {
        FileName path = m_compilerCommand.parentDir();
        env.prependOrSetPath(path.toString());
        FileName makePath = m_makeCommand.parentDir();
        if (makePath != path)
            env.prependOrSetPath(makePath.toString());
    }
}

QList<FileName> CustomToolChain::suggestedMkspecList() const
{
    return m_mkspecs;
}

// TODO: Customize
IOutputParser *CustomToolChain::outputParser() const
{
    return new GccParser;
}

QStringList CustomToolChain::headerPathsList() const
{
    QStringList list;
    foreach (const HeaderPath &headerPath, m_systemHeaderPaths)
        list << headerPath.path();
    return list;
}

void CustomToolChain::setHeaderPaths(const QStringList &list)
{
    m_systemHeaderPaths.clear();
    foreach (const QString &headerPath, list)
        m_systemHeaderPaths << HeaderPath(headerPath.trimmed(), HeaderPath::GlobalHeaderPath);
}

void CustomToolChain::setCompilerCommand(const FileName &path)
{
    if (path == m_compilerCommand)
        return;
    m_compilerCommand = path;
    toolChainUpdated();
}

FileName CustomToolChain::compilerCommand() const
{
    return m_compilerCommand;
}

void CustomToolChain::setMakeCommand(const FileName &path)
{
    if (path == m_makeCommand)
        return;
    m_makeCommand = path;
    toolChainUpdated();
}

QString CustomToolChain::makeCommand(const Utils::Environment &) const
{
    return m_makeCommand.toString();
}

void CustomToolChain::setCxx11Flags(const QStringList &flags)
{
    if (flags == m_cxx11Flags)
        return;
    m_cxx11Flags = flags;
    toolChainUpdated();
}

const QStringList &CustomToolChain::cxx11Flags() const
{
    return m_cxx11Flags;
}

void CustomToolChain::setMkspecs(const QString &specs)
{
    m_mkspecs.clear();
    foreach (const QString &spec, specs.split(QLatin1Char(',')))
        m_mkspecs << FileName::fromString(spec);
}

QString CustomToolChain::mkspecs() const
{
    QString list;
    foreach (const FileName &spec, m_mkspecs)
        list.append(spec.toString() + QLatin1Char(','));
    list.chop(1);
    return list;
}

ToolChain *CustomToolChain::clone() const
{
    return new CustomToolChain(*this);
}

QVariantMap CustomToolChain::toMap() const
{
    QVariantMap data = ToolChain::toMap();
    data.insert(QLatin1String(compilerCommandKeyC), m_compilerCommand.toString());
    data.insert(QLatin1String(makeCommandKeyC), m_makeCommand.toString());
    data.insert(QLatin1String(targetAbiKeyC), m_targetAbi.toString());
    data.insert(QLatin1String(predefinedMacrosKeyC), m_predefinedMacros);
    data.insert(QLatin1String(headerPathsKeyC), headerPathsList());
    data.insert(QLatin1String(cxx11FlagsKeyC), m_cxx11Flags);
    data.insert(QLatin1String(mkspecsKeyC), mkspecs());

    return data;
}

bool CustomToolChain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;

    m_compilerCommand = FileName::fromString(data.value(QLatin1String(compilerCommandKeyC)).toString());
    m_makeCommand = FileName::fromString(data.value(QLatin1String(makeCommandKeyC)).toString());
    m_targetAbi = Abi(data.value(QLatin1String(targetAbiKeyC)).toString());
    m_predefinedMacros = data.value(QLatin1String(predefinedMacrosKeyC)).toStringList();
    setHeaderPaths(data.value(QLatin1String(headerPathsKeyC)).toStringList());
    m_cxx11Flags = data.value(QLatin1String(cxx11FlagsKeyC)).toStringList();
    setMkspecs(data.value(QLatin1String(mkspecsKeyC)).toString());

    return true;
}

bool CustomToolChain::operator ==(const ToolChain &other) const
{
    if (!ToolChain::operator ==(other))
        return false;

    const CustomToolChain *customTc = static_cast<const CustomToolChain *>(&other);
    return m_compilerCommand == customTc->m_compilerCommand
            && m_makeCommand == customTc->m_makeCommand
            && m_targetAbi == customTc->m_targetAbi
            && m_predefinedMacros == customTc->m_predefinedMacros
            && m_systemHeaderPaths == customTc->m_systemHeaderPaths;
}

ToolChainConfigWidget *CustomToolChain::configurationWidget()
{
    return new Internal::CustomToolChainConfigWidget(this);
}

namespace Internal {

// --------------------------------------------------------------------------
// CustomToolChainFactory
// --------------------------------------------------------------------------

QString CustomToolChainFactory::displayName() const
{
    return tr("Custom");
}

QString CustomToolChainFactory::id() const
{
    return QLatin1String(Constants::CUSTOM_TOOLCHAIN_ID);
}

bool CustomToolChainFactory::canCreate()
{
    return true;
}

ToolChain *CustomToolChainFactory::create()
{
    return createToolChain(false);
}

// Used by the ToolChainManager to restore user-generated tool chains
bool CustomToolChainFactory::canRestore(const QVariantMap &data)
{
    const QString id = idFromMap(data);
    return id.startsWith(QLatin1String(Constants::CUSTOM_TOOLCHAIN_ID) + QLatin1Char(':'));
}

ToolChain *CustomToolChainFactory::restore(const QVariantMap &data)
{
    CustomToolChain *tc = new CustomToolChain(false);
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return 0;
}

CustomToolChain *CustomToolChainFactory::createToolChain(bool autoDetect)
{
    return new CustomToolChain(autoDetect);
}

// --------------------------------------------------------------------------
// Helper for ConfigWidget
// --------------------------------------------------------------------------

class TextEditDetailsWidget : public DetailsWidget
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::Internal::TextEditDetailsWidget)
public:
    TextEditDetailsWidget(QPlainTextEdit *textEdit)
    {
        setWidget(textEdit);
    }

    inline QPlainTextEdit *textEditWidget() const
    {
        return static_cast<QPlainTextEdit *>(widget());
    }

    inline QStringList entries() const
    {
        return textEditWidget()->toPlainText().split(QLatin1Char('\n'), QString::SkipEmptyParts);
    }

    // not accurate, counts empty lines (except last)
    int entryCount() const
    {
        int count = textEditWidget()->blockCount();
        QString text = textEditWidget()->toPlainText();
        if (text.isEmpty() || text.endsWith(QLatin1Char('\n')))
            --count;
        return count;
    }

    void updateSummaryText()
    {
        int count = entryCount();
        setSummaryText(count ? tr("%n entry(ies)", "", count) : tr("Empty"));
    }
};

// --------------------------------------------------------------------------
// CustomToolChainConfigWidget
// --------------------------------------------------------------------------

CustomToolChainConfigWidget::CustomToolChainConfigWidget(CustomToolChain *tc) :
    ToolChainConfigWidget(tc),
    m_compilerCommand(new PathChooser),
    m_makeCommand(new PathChooser),
    m_abiWidget(new AbiWidget),
    m_predefinedMacros(new QPlainTextEdit),
    m_headerPaths(new QPlainTextEdit),
    m_predefinedDetails(new TextEditDetailsWidget(m_predefinedMacros)),
    m_headerDetails(new TextEditDetailsWidget(m_headerPaths)),
    m_cxx11Flags(new QLineEdit),
    m_mkspecs(new QLineEdit)
{
    Q_ASSERT(tc);

    m_predefinedMacros->setTabChangesFocus(true);
    m_predefinedMacros->setToolTip(tr("Each line defines a macro. Format is MACRO[=VALUE]"));
    m_headerPaths->setTabChangesFocus(true);
    m_headerPaths->setToolTip(tr("Each line adds a global header lookup path"));
    m_cxx11Flags->setToolTip(tr("Comma-separated list of flags that turn on C++11 support"));
    m_mkspecs->setToolTip(tr("Comma-separated list of mkspecs"));
    m_compilerCommand->setExpectedKind(PathChooser::ExistingCommand);
    m_makeCommand->setExpectedKind(PathChooser::ExistingCommand);
    m_mainLayout->addRow(tr("&Compiler path:"), m_compilerCommand);
    m_mainLayout->addRow(tr("&Make path:"), m_makeCommand);
    m_mainLayout->addRow(tr("&ABI:"), m_abiWidget);
    m_mainLayout->addRow(tr("&Predefined macros:"), m_predefinedDetails);
    m_mainLayout->addRow(tr("&Header paths:"), m_headerDetails);
    m_mainLayout->addRow(tr("C++11 &flags:"), m_cxx11Flags);
    m_mainLayout->addRow(tr("&Qt mkspecs:"), m_mkspecs);
    addErrorLabel();

    setFromToolchain();
    m_predefinedDetails->updateSummaryText();
    m_headerDetails->updateSummaryText();

    connect(m_abiWidget, SIGNAL(abiChanged()), this, SIGNAL(dirty()));
    connect(m_predefinedMacros, SIGNAL(textChanged()), this, SLOT(updateSummaries()));
    connect(m_headerPaths, SIGNAL(textChanged()), this, SLOT(updateSummaries()));
}

void CustomToolChainConfigWidget::updateSummaries()
{
    if (sender() == m_predefinedMacros)
        m_predefinedDetails->updateSummaryText();
    else
        m_headerDetails->updateSummaryText();
}

void CustomToolChainConfigWidget::applyImpl()
{
    if (toolChain()->isAutoDetected())
        return;

    CustomToolChain *tc = static_cast<CustomToolChain *>(toolChain());
    Q_ASSERT(tc);
    QString displayName = tc->displayName();
    tc->setCompilerCommand(m_compilerCommand->fileName());
    tc->setMakeCommand(m_makeCommand->fileName());
    tc->setTargetAbi(m_abiWidget->currentAbi());
    tc->setPredefinedMacros(m_predefinedDetails->entries());
    tc->setHeaderPaths(m_headerDetails->entries());
    tc->setCxx11Flags(m_cxx11Flags->text().split(QLatin1Char(',')));
    tc->setMkspecs(m_mkspecs->text());
    tc->setDisplayName(displayName); // reset display name
}

void CustomToolChainConfigWidget::setFromToolchain()
{
    // subwidgets are not yet connected!
    bool blocked = blockSignals(true);
    CustomToolChain *tc = static_cast<CustomToolChain *>(toolChain());
    m_compilerCommand->setFileName(tc->compilerCommand());
    m_makeCommand->setFileName(FileName::fromString(tc->makeCommand(Utils::Environment())));
    m_abiWidget->setAbis(QList<Abi>(), tc->targetAbi());
    m_predefinedMacros->setPlainText(tc->rawPredefinedMacros().join(QLatin1String("\n")));
    m_headerPaths->setPlainText(tc->headerPathsList().join(QLatin1String("\n")));
    m_cxx11Flags->setText(tc->cxx11Flags().join(QLatin1String(",")));
    m_mkspecs->setText(tc->mkspecs());
    blockSignals(blocked);
}

bool CustomToolChainConfigWidget::isDirtyImpl() const
{
    CustomToolChain *tc = static_cast<CustomToolChain *>(toolChain());
    Q_ASSERT(tc);
    return m_compilerCommand->fileName() != tc->compilerCommand()
            || m_makeCommand->path() != tc->makeCommand(Utils::Environment())
            || m_abiWidget->currentAbi() != tc->targetAbi()
            || m_predefinedDetails->entries() != tc->rawPredefinedMacros()
            || m_headerDetails->entries() != tc->headerPathsList()
            || m_cxx11Flags->text().split(QLatin1Char(',')) != tc->cxx11Flags()
            || m_mkspecs->text() != tc->mkspecs();
}

void CustomToolChainConfigWidget::makeReadOnlyImpl()
{
    m_mainLayout->setEnabled(false);
}

} // namespace Internal
} // namespace ProjectExplorer
