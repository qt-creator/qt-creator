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

#include "customtoolchain.h"
#include "abiwidget.h"
#include "gccparser.h"
#include "clangparser.h"
#include "linuxiccparser.h"
#include "msvcparser.h"
#include "customparser.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectmacro.h"
#include "toolchainmanager.h"

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/environment.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QUuid>

using namespace Utils;

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------------

static const char compilerCommandKeyC[] = "ProjectExplorer.CustomToolChain.CompilerPath";
static const char makeCommandKeyC[] = "ProjectExplorer.CustomToolChain.MakePath";
static const char targetAbiKeyC[] = "ProjectExplorer.CustomToolChain.TargetAbi";
static const char predefinedMacrosKeyC[] = "ProjectExplorer.CustomToolChain.PredefinedMacros";
static const char headerPathsKeyC[] = "ProjectExplorer.CustomToolChain.HeaderPaths";
static const char cxx11FlagsKeyC[] = "ProjectExplorer.CustomToolChain.Cxx11Flags";
static const char mkspecsKeyC[] = "ProjectExplorer.CustomToolChain.Mkspecs";
static const char outputParserKeyC[] = "ProjectExplorer.CustomToolChain.OutputParser";

// --------------------------------------------------------------------------
// CustomToolChain
// --------------------------------------------------------------------------

CustomToolChain::CustomToolChain() :
    ToolChain(Constants::CUSTOM_TOOLCHAIN_TYPEID),
    m_outputParserId(GccParser::id())
{
    setTypeDisplayName(tr("Custom"));
}

Internal::CustomParserSettings CustomToolChain::customParserSettings() const
{
    return findOrDefault(ProjectExplorerPlugin::customParsers(),
                         [this](const Internal::CustomParserSettings &s) {
        return s.id == outputParserId();
    });
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

ToolChain::MacroInspectionRunner CustomToolChain::createMacroInspectionRunner() const
{
    const Macros theMacros = m_predefinedMacros;
    const Utils::Id lang = language();

    // This runner must be thread-safe!
    return [theMacros, lang](const QStringList &cxxflags){
        Macros macros = theMacros;
        for (const QString &cxxFlag : cxxflags) {
            if (cxxFlag.startsWith(QLatin1String("-D")))
                macros.append(Macro::fromKeyValue(cxxFlag.mid(2).trimmed()));
             else if (cxxFlag.startsWith(QLatin1String("-U")) && !cxxFlag.contains('='))
                macros.append({cxxFlag.mid(2).trimmed().toUtf8(), MacroType::Undefine});

        }
        return MacroInspectionReport{macros, ToolChain::languageVersion(lang, macros)};
    };
}

Macros CustomToolChain::predefinedMacros(const QStringList &cxxflags) const
{
    return createMacroInspectionRunner()(cxxflags).macros;
}

Utils::LanguageExtensions CustomToolChain::languageExtensions(const QStringList &) const
{
    return LanguageExtension::None;
}

WarningFlags CustomToolChain::warningFlags(const QStringList &cxxflags) const
{
    Q_UNUSED(cxxflags)
    return WarningFlags::Default;
}

const Macros &CustomToolChain::rawPredefinedMacros() const
{
    return m_predefinedMacros;
}

void CustomToolChain::setPredefinedMacros(const Macros &macros)
{
    if (m_predefinedMacros == macros)
        return;
    m_predefinedMacros = macros;
    toolChainUpdated();
}

ToolChain::BuiltInHeaderPathsRunner CustomToolChain::createBuiltInHeaderPathsRunner(
        const Environment &) const
{
    const HeaderPaths builtInHeaderPaths = m_builtInHeaderPaths;

    // This runner must be thread-safe!
    return [builtInHeaderPaths](const QStringList &cxxFlags, const QString &, const QString &) {
        HeaderPaths flagHeaderPaths;
        for (const QString &cxxFlag : cxxFlags) {
            if (cxxFlag.startsWith(QLatin1String("-I"))) {
                flagHeaderPaths.push_back({cxxFlag.mid(2).trimmed(), HeaderPathType::BuiltIn});
            }
        }

        return builtInHeaderPaths + flagHeaderPaths;
    };
}

HeaderPaths CustomToolChain::builtInHeaderPaths(const QStringList &cxxFlags,
                                                const FilePath &fileName,
                                                const Environment &env) const
{
    return createBuiltInHeaderPathsRunner(env)(cxxFlags, fileName.toString(), "");
}

void CustomToolChain::addToEnvironment(Environment &env) const
{
    if (!m_compilerCommand.isEmpty()) {
        FilePath path = m_compilerCommand.parentDir();
        env.prependOrSetPath(path.toString());
        FilePath makePath = m_makeCommand.parentDir();
        if (makePath != path)
            env.prependOrSetPath(makePath.toString());
    }
}

QStringList CustomToolChain::suggestedMkspecList() const
{
    return m_mkspecs;
}

QList<Utils::OutputLineParser *> CustomToolChain::createOutputParsers() const
{
    if (m_outputParserId == GccParser::id())
        return GccParser::gccParserSuite();
    if (m_outputParserId == ClangParser::id())
        return ClangParser::clangParserSuite();
    if (m_outputParserId == LinuxIccParser::id())
        return LinuxIccParser::iccParserSuite();
    if (m_outputParserId == MsvcParser::id())
        return {new MsvcParser};
    return {new Internal::CustomParser(customParserSettings())};
    return {};
}

QStringList CustomToolChain::headerPathsList() const
{
    return Utils::transform<QList>(m_builtInHeaderPaths, &HeaderPath::path);
}

void CustomToolChain::setHeaderPaths(const QStringList &list)
{
    HeaderPaths tmp = Utils::transform<QVector>(list, [](const QString &headerPath) {
        return HeaderPath(headerPath.trimmed(), HeaderPathType::BuiltIn);
    });

    if (m_builtInHeaderPaths == tmp)
        return;
    m_builtInHeaderPaths = tmp;
    toolChainUpdated();
}

void CustomToolChain::setCompilerCommand(const FilePath &path)
{
    if (path == m_compilerCommand)
        return;
    m_compilerCommand = path;
    toolChainUpdated();
}

FilePath CustomToolChain::compilerCommand() const
{
    return m_compilerCommand;
}

void CustomToolChain::setMakeCommand(const FilePath &path)
{
    if (path == m_makeCommand)
        return;
    m_makeCommand = path;
    toolChainUpdated();
}

FilePath CustomToolChain::makeCommand(const Environment &) const
{
    return m_makeCommand;
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
    const QStringList tmp = specs.split(',');
    if (tmp == m_mkspecs)
        return;
    m_mkspecs = tmp;
    toolChainUpdated();
}

QString CustomToolChain::mkspecs() const
{
    return m_mkspecs.join(',');
}

QVariantMap CustomToolChain::toMap() const
{
    QVariantMap data = ToolChain::toMap();
    data.insert(QLatin1String(compilerCommandKeyC), m_compilerCommand.toString());
    data.insert(QLatin1String(makeCommandKeyC), m_makeCommand.toString());
    data.insert(QLatin1String(targetAbiKeyC), m_targetAbi.toString());
    QStringList macros = Utils::transform<QList>(m_predefinedMacros, [](const Macro &m) { return QString::fromUtf8(m.toByteArray()); });
    data.insert(QLatin1String(predefinedMacrosKeyC), macros);
    data.insert(QLatin1String(headerPathsKeyC), headerPathsList());
    data.insert(QLatin1String(cxx11FlagsKeyC), m_cxx11Flags);
    data.insert(QLatin1String(mkspecsKeyC), mkspecs());
    data.insert(QLatin1String(outputParserKeyC), m_outputParserId.toSetting());

    return data;
}

bool CustomToolChain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;

    m_compilerCommand = FilePath::fromString(data.value(QLatin1String(compilerCommandKeyC)).toString());
    m_makeCommand = FilePath::fromString(data.value(QLatin1String(makeCommandKeyC)).toString());
    m_targetAbi = Abi::fromString(data.value(QLatin1String(targetAbiKeyC)).toString());
    const QStringList macros = data.value(QLatin1String(predefinedMacrosKeyC)).toStringList();
    m_predefinedMacros = Macro::toMacros(macros.join('\n').toUtf8());
    setHeaderPaths(data.value(QLatin1String(headerPathsKeyC)).toStringList());
    m_cxx11Flags = data.value(QLatin1String(cxx11FlagsKeyC)).toStringList();
    setMkspecs(data.value(QLatin1String(mkspecsKeyC)).toString());
    setOutputParserId(Utils::Id::fromSetting(data.value(QLatin1String(outputParserKeyC))));

    // Restore Pre-4.13 settings.
    if (outputParserId() == Internal::CustomParser::id()) {
        Internal::CustomParserSettings customParserSettings;
        customParserSettings.error.setPattern(
                    data.value("ProjectExplorer.CustomToolChain.ErrorPattern").toString());
        customParserSettings.error.setFileNameCap(
                    data.value("ProjectExplorer.CustomToolChain.ErrorLineNumberCap").toInt());
        customParserSettings.error.setLineNumberCap(
                    data.value("ProjectExplorer.CustomToolChain.ErrorFileNameCap").toInt());
        customParserSettings.error.setMessageCap(
                    data.value("ProjectExplorer.CustomToolChain.ErrorMessageCap").toInt());
        customParserSettings.error.setChannel(
                    static_cast<Internal::CustomParserExpression::CustomParserChannel>(
                        data.value("ProjectExplorer.CustomToolChain.ErrorChannel").toInt()));
        customParserSettings.error.setExample(
                    data.value("ProjectExplorer.CustomToolChain.ErrorExample").toString());
        customParserSettings.warning.setPattern(
                    data.value("ProjectExplorer.CustomToolChain.WarningPattern").toString());
        customParserSettings.warning.setFileNameCap(
                    data.value("ProjectExplorer.CustomToolChain.WarningLineNumberCap").toInt());
        customParserSettings.warning.setLineNumberCap(
                    data.value("ProjectExplorer.CustomToolChain.WarningFileNameCap").toInt());
        customParserSettings.warning.setMessageCap(
                    data.value("ProjectExplorer.CustomToolChain.WarningMessageCap").toInt());
        customParserSettings.warning.setChannel(
                    static_cast<Internal::CustomParserExpression::CustomParserChannel>(
                        data.value("ProjectExplorer.CustomToolChain.WarningChannel").toInt()));
        customParserSettings.warning.setExample(
                    data.value("ProjectExplorer.CustomToolChain.WarningExample").toString());
        if (!customParserSettings.error.pattern().isEmpty()
                || !customParserSettings.error.pattern().isEmpty()) {
            // Found custom parser in old settings, move to new place.
            customParserSettings.id = Utils::Id::fromString(QUuid::createUuid().toString());
            setOutputParserId(customParserSettings.id);
            customParserSettings.displayName = tr("Parser for toolchain %1").arg(displayName());
            QList<Internal::CustomParserSettings> settings
                    = ProjectExplorerPlugin::customParsers();
            settings << customParserSettings;
            ProjectExplorerPlugin::setCustomParsers(settings);
        }
    }

    return true;
}

bool CustomToolChain::operator ==(const ToolChain &other) const
{
    if (!ToolChain::operator ==(other))
        return false;

    auto customTc = static_cast<const CustomToolChain *>(&other);
    return m_compilerCommand == customTc->m_compilerCommand
            && m_makeCommand == customTc->m_makeCommand
            && m_targetAbi == customTc->m_targetAbi
            && m_predefinedMacros == customTc->m_predefinedMacros
            && m_builtInHeaderPaths == customTc->m_builtInHeaderPaths;
}

Utils::Id CustomToolChain::outputParserId() const
{
    return m_outputParserId;
}

void CustomToolChain::setOutputParserId(Utils::Id parserId)
{
    if (m_outputParserId == parserId)
        return;
    m_outputParserId = parserId;
    toolChainUpdated();
}

QList<CustomToolChain::Parser> CustomToolChain::parsers()
{
    QList<CustomToolChain::Parser> result;
    result.append({GccParser::id(),      tr("GCC")});
    result.append({ClangParser::id(),    tr("Clang")});
    result.append({LinuxIccParser::id(), tr("ICC")});
    result.append({MsvcParser::id(),     tr("MSVC")});
    return result;
}

std::unique_ptr<ToolChainConfigWidget> CustomToolChain::createConfigurationWidget()
{
    return std::make_unique<Internal::CustomToolChainConfigWidget>(this);
}

namespace Internal {

// --------------------------------------------------------------------------
// CustomToolChainFactory
// --------------------------------------------------------------------------

CustomToolChainFactory::CustomToolChainFactory()
{
    setDisplayName(CustomToolChain::tr("Custom"));
    setSupportedToolChainType(Constants::CUSTOM_TOOLCHAIN_TYPEID);
    setSupportsAllLanguages(true);
    setToolchainConstructor([] { return new CustomToolChain; });
    setUserCreatable(true);
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

    QStringList entries() const
    {
        return textEditWidget()->toPlainText().split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    }

    QString text() const
    {
        return textEditWidget()->toPlainText();
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
        setSummaryText(count ? tr("%n entries", "", count) : tr("Empty"));
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
    m_mkspecs(new QLineEdit),
    m_errorParserComboBox(new QComboBox)
{
    Q_ASSERT(tc);

    const QList<CustomToolChain::Parser> parsers = CustomToolChain::parsers();
    for (const auto &parser : parsers)
        m_errorParserComboBox->addItem(parser.displayName, parser.parserId.toString());
    for (const Internal::CustomParserSettings &s : ProjectExplorerPlugin::customParsers())
        m_errorParserComboBox->addItem(s.displayName, s.id.toString());

    auto parserLayoutWidget = new QWidget;
    auto parserLayout = new QHBoxLayout(parserLayoutWidget);
    parserLayout->setContentsMargins(0, 0, 0, 0);
    m_predefinedMacros->setPlaceholderText(tr("MACRO[=VALUE]"));
    m_predefinedMacros->setTabChangesFocus(true);
    m_predefinedMacros->setToolTip(tr("Each line defines a macro. Format is MACRO[=VALUE]."));
    m_headerPaths->setTabChangesFocus(true);
    m_headerPaths->setToolTip(tr("Each line adds a global header lookup path."));
    m_cxx11Flags->setToolTip(tr("Comma-separated list of flags that turn on C++11 support."));
    m_mkspecs->setToolTip(tr("Comma-separated list of mkspecs."));
    m_compilerCommand->setExpectedKind(PathChooser::ExistingCommand);
    m_compilerCommand->setHistoryCompleter(QLatin1String("PE.ToolChainCommand.History"));
    m_makeCommand->setExpectedKind(PathChooser::ExistingCommand);
    m_makeCommand->setHistoryCompleter(QLatin1String("PE.MakeCommand.History"));
    m_mainLayout->addRow(tr("&Compiler path:"), m_compilerCommand);
    m_mainLayout->addRow(tr("&Make path:"), m_makeCommand);
    m_mainLayout->addRow(tr("&ABI:"), m_abiWidget);
    m_mainLayout->addRow(tr("&Predefined macros:"), m_predefinedDetails);
    m_mainLayout->addRow(tr("&Header paths:"), m_headerDetails);
    m_mainLayout->addRow(tr("C++11 &flags:"), m_cxx11Flags);
    m_mainLayout->addRow(tr("&Qt mkspecs:"), m_mkspecs);
    parserLayout->addWidget(m_errorParserComboBox);
    m_mainLayout->addRow(tr("&Error parser:"), parserLayoutWidget);
    addErrorLabel();

    setFromToolchain();
    m_predefinedDetails->updateSummaryText();
    m_headerDetails->updateSummaryText();

    connect(m_compilerCommand, &PathChooser::rawPathChanged, this, &ToolChainConfigWidget::dirty);
    connect(m_makeCommand, &PathChooser::rawPathChanged, this, &ToolChainConfigWidget::dirty);
    connect(m_abiWidget, &AbiWidget::abiChanged, this, &ToolChainConfigWidget::dirty);
    connect(m_predefinedMacros, &QPlainTextEdit::textChanged,
            this, &CustomToolChainConfigWidget::updateSummaries);
    connect(m_headerPaths, &QPlainTextEdit::textChanged,
            this, &CustomToolChainConfigWidget::updateSummaries);
    connect(m_cxx11Flags, &QLineEdit::textChanged, this, &ToolChainConfigWidget::dirty);
    connect(m_mkspecs, &QLineEdit::textChanged, this, &ToolChainConfigWidget::dirty);
    connect(m_errorParserComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CustomToolChainConfigWidget::errorParserChanged);
    errorParserChanged();
}

void CustomToolChainConfigWidget::updateSummaries()
{
    if (sender() == m_predefinedMacros)
        m_predefinedDetails->updateSummaryText();
    else
        m_headerDetails->updateSummaryText();
    emit dirty();
}

void CustomToolChainConfigWidget::errorParserChanged(int )
{
    emit dirty();
}

void CustomToolChainConfigWidget::applyImpl()
{
    if (toolChain()->isAutoDetected())
        return;

    auto tc = static_cast<CustomToolChain *>(toolChain());
    Q_ASSERT(tc);
    QString displayName = tc->displayName();
    tc->setCompilerCommand(m_compilerCommand->filePath());
    tc->setMakeCommand(m_makeCommand->filePath());
    tc->setTargetAbi(m_abiWidget->currentAbi());
    Macros macros = Utils::transform<QVector>(
                m_predefinedDetails->text().split('\n', Qt::SkipEmptyParts),
                [](const QString &m) {
        return Macro::fromKeyValue(m);
    });
    tc->setPredefinedMacros(macros);
    tc->setHeaderPaths(m_headerDetails->entries());
    tc->setCxx11Flags(m_cxx11Flags->text().split(QLatin1Char(',')));
    tc->setMkspecs(m_mkspecs->text());
    tc->setDisplayName(displayName); // reset display name
    tc->setOutputParserId(Utils::Id::fromSetting(m_errorParserComboBox->currentData()));

    setFromToolchain(); // Refresh with actual data from the toolchain. This shows what e.g. the
                        // macro parser did with the input.
}

void CustomToolChainConfigWidget::setFromToolchain()
{
    // subwidgets are not yet connected!
    QSignalBlocker blocker(this);
    auto tc = static_cast<CustomToolChain *>(toolChain());
    m_compilerCommand->setFilePath(tc->compilerCommand());
    m_makeCommand->setFilePath(tc->makeCommand(Environment()));
    m_abiWidget->setAbis(Abis(), tc->targetAbi());
    const QStringList macroLines = Utils::transform<QList>(tc->rawPredefinedMacros(), [](const Macro &m) {
        return QString::fromUtf8(m.toKeyValue(QByteArray()));
    });
    m_predefinedMacros->setPlainText(macroLines.join('\n'));
    m_headerPaths->setPlainText(tc->headerPathsList().join('\n'));
    m_cxx11Flags->setText(tc->cxx11Flags().join(QLatin1Char(',')));
    m_mkspecs->setText(tc->mkspecs());
    int index = m_errorParserComboBox->findData(tc->outputParserId().toSetting());
    m_errorParserComboBox->setCurrentIndex(index);
}

bool CustomToolChainConfigWidget::isDirtyImpl() const
{
    auto tc = static_cast<CustomToolChain *>(toolChain());
    Q_ASSERT(tc);
    return m_compilerCommand->filePath() != tc->compilerCommand()
            || m_makeCommand->filePath().toString() != tc->makeCommand(Environment()).toString()
            || m_abiWidget->currentAbi() != tc->targetAbi()
            || Macro::toMacros(m_predefinedDetails->text().toUtf8()) != tc->rawPredefinedMacros()
            || m_headerDetails->entries() != tc->headerPathsList()
            || m_cxx11Flags->text().split(QLatin1Char(',')) != tc->cxx11Flags()
            || m_mkspecs->text() != tc->mkspecs()
            || Utils::Id::fromSetting(m_errorParserComboBox->currentData()) == tc->outputParserId();
}

void CustomToolChainConfigWidget::makeReadOnlyImpl()
{
    m_mainLayout->setEnabled(false);
}

} // namespace Internal
} // namespace ProjectExplorer
