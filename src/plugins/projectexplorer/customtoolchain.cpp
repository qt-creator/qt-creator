// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customtoolchain.h"

#include "abiwidget.h"
#include "gccparser.h"
#include "clangparser.h"
#include "linuxiccparser.h"
#include "msvcparser.h"
#include "customparser.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "projectmacro.h"
#include "toolchainconfigwidget.h"

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

namespace ProjectExplorer::Internal {

const char makeCommandKeyC[] = "ProjectExplorer.CustomToolChain.MakePath";
const char predefinedMacrosKeyC[] = "ProjectExplorer.CustomToolChain.PredefinedMacros";
const char headerPathsKeyC[] = "ProjectExplorer.CustomToolChain.HeaderPaths";
const char cxx11FlagsKeyC[] = "ProjectExplorer.CustomToolChain.Cxx11Flags";
const char mkspecsKeyC[] = "ProjectExplorer.CustomToolChain.Mkspecs";
const char outputParserKeyC[] = "ProjectExplorer.CustomToolChain.OutputParser";

// --------------------------------------------------------------------------
// CustomToolChain
// --------------------------------------------------------------------------

class CustomToolChain : public ToolChain
{
public:
    CustomToolChain()
        : ToolChain(Constants::CUSTOM_TOOLCHAIN_TYPEID)
        , m_outputParserId(GccParser::id())
    {
        setTypeDisplayName(Tr::tr("Custom"));
        setTargetAbiKey("ProjectExplorer.CustomToolChain.TargetAbi");
        setCompilerCommandKey("ProjectExplorer.CustomToolChain.CompilerPath");
    }

    class Parser {
    public:
        Id parserId;   ///< A unique id identifying a parser
        QString displayName; ///< A translateable name to show in the user interface
    };

    bool isValid() const override;

    MacroInspectionRunner createMacroInspectionRunner() const override;
    LanguageExtensions languageExtensions(const QStringList &cxxflags) const override;
    WarningFlags warningFlags(const QStringList &cxxflags) const override;
    const Macros &rawPredefinedMacros() const;
    void setPredefinedMacros(const Macros &macros);

    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(const Environment &) const override;
    void addToEnvironment(Environment &env) const override;
    QStringList suggestedMkspecList() const override;
    QList<OutputLineParser *> createOutputParsers() const override;
    QStringList headerPathsList() const;
    void setHeaderPaths(const QStringList &list);

    void toMap(Store &data) const override;
    void fromMap(const Store &data) override;

    std::unique_ptr<ToolChainConfigWidget> createConfigurationWidget() override;

    bool operator ==(const ToolChain &) const override;

    void setMakeCommand(const FilePath &);
    FilePath makeCommand(const Environment &environment) const override;

    void setCxx11Flags(const QStringList &);
    const QStringList &cxx11Flags() const;

    void setMkspecs(const QString &);
    QString mkspecs() const;

    Id outputParserId() const;
    void setOutputParserId(Id parserId);
    static QList<CustomToolChain::Parser> parsers();

    CustomParserSettings customParserSettings() const;

private:
    FilePath m_makeCommand;

    Macros m_predefinedMacros;
    HeaderPaths m_builtInHeaderPaths;
    QStringList m_cxx11Flags;
    QStringList m_mkspecs;

    Id m_outputParserId;
};

CustomParserSettings CustomToolChain::customParserSettings() const
{
    return findOrDefault(ProjectExplorerPlugin::customParsers(),
                         [this](const CustomParserSettings &s) {
        return s.id == outputParserId();
    });
}

bool CustomToolChain::isValid() const
{
    return true;
}

ToolChain::MacroInspectionRunner CustomToolChain::createMacroInspectionRunner() const
{
    const Macros theMacros = m_predefinedMacros;
    const Id lang = language();

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

LanguageExtensions CustomToolChain::languageExtensions(const QStringList &) const
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
    return [builtInHeaderPaths](const QStringList &cxxFlags, const FilePath &sysRoot, const QString &) {
        Q_UNUSED(sysRoot)
        HeaderPaths flagHeaderPaths;
        for (const QString &cxxFlag : cxxFlags) {
            if (cxxFlag.startsWith(QLatin1String("-I")))
                flagHeaderPaths.push_back(HeaderPath::makeBuiltIn(cxxFlag.mid(2).trimmed()));
        }

        return builtInHeaderPaths + flagHeaderPaths;
    };
}

void CustomToolChain::addToEnvironment(Environment &env) const
{
    const FilePath compiler = compilerCommand();
    if (compiler.isEmpty())
        return;
    const FilePath path = compiler.parentDir();
    env.prependOrSetPath(path);
    const FilePath makePath = m_makeCommand.parentDir();
    if (makePath != path)
        env.prependOrSetPath(makePath);
}

QStringList CustomToolChain::suggestedMkspecList() const
{
    return m_mkspecs;
}

QList<OutputLineParser *> CustomToolChain::createOutputParsers() const
{
    if (m_outputParserId == GccParser::id())
        return GccParser::gccParserSuite();
    if (m_outputParserId == ClangParser::id())
        return ClangParser::clangParserSuite();
    if (m_outputParserId == LinuxIccParser::id())
        return LinuxIccParser::iccParserSuite();
    if (m_outputParserId == MsvcParser::id())
        return {new MsvcParser};
    return {new CustomParser(customParserSettings())};
}

QStringList CustomToolChain::headerPathsList() const
{
    return Utils::transform<QList>(m_builtInHeaderPaths, &HeaderPath::path);
}

void CustomToolChain::setHeaderPaths(const QStringList &list)
{
    HeaderPaths tmp = Utils::transform<QVector>(list, [](const QString &headerPath) {
        return HeaderPath::makeBuiltIn(headerPath.trimmed());
    });

    if (m_builtInHeaderPaths == tmp)
        return;
    m_builtInHeaderPaths = tmp;
    toolChainUpdated();
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

void CustomToolChain::toMap(Store &data) const
{
    ToolChain::toMap(data);
    data.insert(makeCommandKeyC, m_makeCommand.toString());
    QStringList macros = Utils::transform<QList>(m_predefinedMacros, [](const Macro &m) { return QString::fromUtf8(m.toByteArray()); });
    data.insert(predefinedMacrosKeyC, macros);
    data.insert(headerPathsKeyC, headerPathsList());
    data.insert(cxx11FlagsKeyC, m_cxx11Flags);
    data.insert(mkspecsKeyC, mkspecs());
    data.insert(outputParserKeyC, m_outputParserId.toSetting());
}

void CustomToolChain::fromMap(const Store &data)
{
    ToolChain::fromMap(data);
    if (hasError())
        return;

    m_makeCommand = FilePath::fromString(data.value(makeCommandKeyC).toString());
    const QStringList macros = data.value(predefinedMacrosKeyC).toStringList();
    m_predefinedMacros = Macro::toMacros(macros.join('\n').toUtf8());
    setHeaderPaths(data.value(headerPathsKeyC).toStringList());
    m_cxx11Flags = data.value(cxx11FlagsKeyC).toStringList();
    setMkspecs(data.value(mkspecsKeyC).toString());
    setOutputParserId(Id::fromSetting(data.value(outputParserKeyC)));
}

bool CustomToolChain::operator ==(const ToolChain &other) const
{
    if (!ToolChain::operator ==(other))
        return false;

    auto customTc = static_cast<const CustomToolChain *>(&other);
    return m_makeCommand == customTc->m_makeCommand
            && compilerCommand() == customTc->compilerCommand()
            && targetAbi() == customTc->targetAbi()
            && m_predefinedMacros == customTc->m_predefinedMacros
            && m_builtInHeaderPaths == customTc->m_builtInHeaderPaths;
}

Id CustomToolChain::outputParserId() const
{
    return m_outputParserId;
}

void CustomToolChain::setOutputParserId(Id parserId)
{
    if (m_outputParserId == parserId)
        return;
    m_outputParserId = parserId;
    toolChainUpdated();
}

QList<CustomToolChain::Parser> CustomToolChain::parsers()
{
    QList<CustomToolChain::Parser> result;
    result.append({GccParser::id(),      Tr::tr("GCC")});
    result.append({ClangParser::id(),    Tr::tr("Clang")});
    result.append({LinuxIccParser::id(), Tr::tr("ICC")});
    result.append({MsvcParser::id(),     Tr::tr("MSVC")});
    return result;
}

// --------------------------------------------------------------------------
// Helper for ConfigWidget
// --------------------------------------------------------------------------

class TextEditDetailsWidget : public DetailsWidget
{
public:
    TextEditDetailsWidget(QPlainTextEdit *textEdit)
    {
        setWidget(textEdit);
    }

    QPlainTextEdit *textEditWidget() const
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
        setSummaryText(count ? Tr::tr("%n entries", "", count) : Tr::tr("Empty"));
    }
};

// --------------------------------------------------------------------------
// CustomToolChainConfigWidget
// --------------------------------------------------------------------------

class CustomToolChainConfigWidget final : public ToolChainConfigWidget
{
public:
    explicit CustomToolChainConfigWidget(CustomToolChain *);

private:
    void updateSummaries(TextEditDetailsWidget *detailsWidget);
    void errorParserChanged(int index = -1);

    void applyImpl() override;
    void discardImpl() override { setFromToolchain(); }
    bool isDirtyImpl() const override;
    void makeReadOnlyImpl() override;

    void setFromToolchain();

    PathChooser *m_compilerCommand;
    PathChooser *m_makeCommand;
    AbiWidget *m_abiWidget;
    QPlainTextEdit *m_predefinedMacros;
    QPlainTextEdit *m_headerPaths;
    TextEditDetailsWidget *m_predefinedDetails;
    TextEditDetailsWidget *m_headerDetails;
    QLineEdit *m_cxx11Flags;
    QLineEdit *m_mkspecs;
    QComboBox *m_errorParserComboBox;
};

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
    for (const CustomParserSettings &s : ProjectExplorerPlugin::customParsers())
        m_errorParserComboBox->addItem(s.displayName, s.id.toString());

    auto parserLayoutWidget = new QWidget;
    auto parserLayout = new QHBoxLayout(parserLayoutWidget);
    parserLayout->setContentsMargins(0, 0, 0, 0);
    m_predefinedMacros->setPlaceholderText(Tr::tr("MACRO[=VALUE]"));
    m_predefinedMacros->setTabChangesFocus(true);
    m_predefinedMacros->setToolTip(Tr::tr("Each line defines a macro. Format is MACRO[=VALUE]."));
    m_headerPaths->setTabChangesFocus(true);
    m_headerPaths->setToolTip(Tr::tr("Each line adds a global header lookup path."));
    m_cxx11Flags->setToolTip(Tr::tr("Comma-separated list of flags that turn on C++11 support."));
    m_mkspecs->setToolTip(Tr::tr("Comma-separated list of mkspecs."));
    m_compilerCommand->setExpectedKind(PathChooser::ExistingCommand);
    m_compilerCommand->setHistoryCompleter("PE.ToolChainCommand.History");
    m_makeCommand->setExpectedKind(PathChooser::ExistingCommand);
    m_makeCommand->setHistoryCompleter("PE.MakeCommand.History");
    m_mainLayout->addRow(Tr::tr("&Compiler path:"), m_compilerCommand);
    m_mainLayout->addRow(Tr::tr("&Make path:"), m_makeCommand);
    m_mainLayout->addRow(Tr::tr("&ABI:"), m_abiWidget);
    m_mainLayout->addRow(Tr::tr("&Predefined macros:"), m_predefinedDetails);
    m_mainLayout->addRow(Tr::tr("&Header paths:"), m_headerDetails);
    m_mainLayout->addRow(Tr::tr("C++11 &flags:"), m_cxx11Flags);
    m_mainLayout->addRow(Tr::tr("&Qt mkspecs:"), m_mkspecs);
    parserLayout->addWidget(m_errorParserComboBox);
    m_mainLayout->addRow(Tr::tr("&Error parser:"), parserLayoutWidget);
    addErrorLabel();

    setFromToolchain();
    m_predefinedDetails->updateSummaryText();
    m_headerDetails->updateSummaryText();

    connect(m_compilerCommand, &PathChooser::rawPathChanged, this, &ToolChainConfigWidget::dirty);
    connect(m_makeCommand, &PathChooser::rawPathChanged, this, &ToolChainConfigWidget::dirty);
    connect(m_abiWidget, &AbiWidget::abiChanged, this, &ToolChainConfigWidget::dirty);
    connect(m_predefinedMacros, &QPlainTextEdit::textChanged,
            this, [this] { updateSummaries(m_predefinedDetails); });
    connect(m_headerPaths, &QPlainTextEdit::textChanged,
            this, [this] { updateSummaries(m_headerDetails); });
    connect(m_cxx11Flags, &QLineEdit::textChanged, this, &ToolChainConfigWidget::dirty);
    connect(m_mkspecs, &QLineEdit::textChanged, this, &ToolChainConfigWidget::dirty);
    connect(m_errorParserComboBox, &QComboBox::currentIndexChanged,
            this, &CustomToolChainConfigWidget::errorParserChanged);
    errorParserChanged();
}

void CustomToolChainConfigWidget::updateSummaries(TextEditDetailsWidget *detailsWidget)
{
    detailsWidget->updateSummaryText();
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
    tc->setOutputParserId(Id::fromSetting(m_errorParserComboBox->currentData()));

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
            || Id::fromSetting(m_errorParserComboBox->currentData()) == tc->outputParserId();
}

void CustomToolChainConfigWidget::makeReadOnlyImpl()
{
    m_mainLayout->setEnabled(false);
}

std::unique_ptr<ToolChainConfigWidget> CustomToolChain::createConfigurationWidget()
{
    return std::make_unique<CustomToolChainConfigWidget>(this);
}

// --------------------------------------------------------------------------
// CustomToolChainFactory
// --------------------------------------------------------------------------

CustomToolChainFactory::CustomToolChainFactory()
{
    setDisplayName(Tr::tr("Custom"));
    setSupportedToolChainType(Constants::CUSTOM_TOOLCHAIN_TYPEID);
    setSupportsAllLanguages(true);
    setToolchainConstructor([] { return new CustomToolChain; });
    setUserCreatable(true);
}

} // ProjectExplorer::Internal
