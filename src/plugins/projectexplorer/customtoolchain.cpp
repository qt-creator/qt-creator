// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customtoolchain.h"

#include "abiwidget.h"
#include "gccparser.h"
#include "clangparser.h"
#include "gcctoolchain.h"
#include "linuxiccparser.h"
#include "msvcparser.h"
#include "customparser.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "projectmacro.h"
#include "toolchain.h"
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

using namespace Utils;

namespace ProjectExplorer::Internal {

const char makeCommandKeyC[] = "ProjectExplorer.CustomToolChain.MakePath";
const char predefinedMacrosKeyC[] = "ProjectExplorer.CustomToolChain.PredefinedMacros";
const char headerPathsKeyC[] = "ProjectExplorer.CustomToolChain.HeaderPaths";
const char cxx11FlagsKeyC[] = "ProjectExplorer.CustomToolChain.Cxx11Flags";
const char mkspecsKeyC[] = "ProjectExplorer.CustomToolChain.Mkspecs";
const char outputParserKeyC[] = "ProjectExplorer.CustomToolChain.OutputParser";

// --------------------------------------------------------------------------
// CustomToolchain
// --------------------------------------------------------------------------

class CustomToolchain : public Toolchain
{
public:
    CustomToolchain()
        : Toolchain(Constants::CUSTOM_TOOLCHAIN_TYPEID)
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

    bool operator ==(const Toolchain &) const override;

    void setMakeCommand(const FilePath &);
    FilePath makeCommand(const Environment &environment) const override;

    void setCxx11Flags(const QStringList &);
    const QStringList &cxx11Flags() const;

    void setMkspecs(const QString &);
    QString mkspecs() const;

    Id outputParserId() const;
    void setOutputParserId(Id parserId);
    static QList<CustomToolchain::Parser> parsers();

    CustomParserSettings customParserSettings() const;

private:
    FilePath m_makeCommand;

    Macros m_predefinedMacros;
    HeaderPaths m_builtInHeaderPaths;
    QStringList m_cxx11Flags;
    QStringList m_mkspecs;

    Id m_outputParserId;
};

CustomParserSettings CustomToolchain::customParserSettings() const
{
    return findOrDefault(ProjectExplorerPlugin::customParsers(),
                         [this](const CustomParserSettings &s) {
        return s.id == outputParserId();
    });
}

bool CustomToolchain::isValid() const
{
    return true;
}

Toolchain::MacroInspectionRunner CustomToolchain::createMacroInspectionRunner() const
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
        return MacroInspectionReport{macros, Toolchain::languageVersion(lang, macros)};
    };
}

LanguageExtensions CustomToolchain::languageExtensions(const QStringList &) const
{
    return LanguageExtension::None;
}

WarningFlags CustomToolchain::warningFlags(const QStringList &cxxflags) const
{
    Q_UNUSED(cxxflags)
    return WarningFlags::Default;
}

const Macros &CustomToolchain::rawPredefinedMacros() const
{
    return m_predefinedMacros;
}

void CustomToolchain::setPredefinedMacros(const Macros &macros)
{
    if (m_predefinedMacros == macros)
        return;
    m_predefinedMacros = macros;
    toolChainUpdated();
}

Toolchain::BuiltInHeaderPathsRunner CustomToolchain::createBuiltInHeaderPathsRunner(
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

void CustomToolchain::addToEnvironment(Environment &env) const
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

QStringList CustomToolchain::suggestedMkspecList() const
{
    return m_mkspecs;
}

QList<OutputLineParser *> CustomToolchain::createOutputParsers() const
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

QStringList CustomToolchain::headerPathsList() const
{
    return Utils::transform<QList>(m_builtInHeaderPaths, &HeaderPath::path);
}

void CustomToolchain::setHeaderPaths(const QStringList &list)
{
    HeaderPaths tmp = Utils::transform<QVector>(list, [](const QString &headerPath) {
        return HeaderPath::makeBuiltIn(headerPath.trimmed());
    });

    if (m_builtInHeaderPaths == tmp)
        return;
    m_builtInHeaderPaths = tmp;
    toolChainUpdated();
}

void CustomToolchain::setMakeCommand(const FilePath &path)
{
    if (path == m_makeCommand)
        return;
    m_makeCommand = path;
    toolChainUpdated();
}

FilePath CustomToolchain::makeCommand(const Environment &) const
{
    return m_makeCommand;
}

void CustomToolchain::setCxx11Flags(const QStringList &flags)
{
    if (flags == m_cxx11Flags)
        return;
    m_cxx11Flags = flags;
    toolChainUpdated();
}

const QStringList &CustomToolchain::cxx11Flags() const
{
    return m_cxx11Flags;
}

void CustomToolchain::setMkspecs(const QString &specs)
{
    const QStringList tmp = specs.split(',');
    if (tmp == m_mkspecs)
        return;
    m_mkspecs = tmp;
    toolChainUpdated();
}

QString CustomToolchain::mkspecs() const
{
    return m_mkspecs.join(',');
}

void CustomToolchain::toMap(Store &data) const
{
    Toolchain::toMap(data);
    data.insert(makeCommandKeyC, m_makeCommand.toString());
    QStringList macros = Utils::transform<QList>(m_predefinedMacros, [](const Macro &m) { return QString::fromUtf8(m.toByteArray()); });
    data.insert(predefinedMacrosKeyC, macros);
    data.insert(headerPathsKeyC, headerPathsList());
    data.insert(cxx11FlagsKeyC, m_cxx11Flags);
    data.insert(mkspecsKeyC, mkspecs());
    data.insert(outputParserKeyC, m_outputParserId.toSetting());
}

void CustomToolchain::fromMap(const Store &data)
{
    Toolchain::fromMap(data);
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

bool CustomToolchain::operator ==(const Toolchain &other) const
{
    if (!Toolchain::operator ==(other))
        return false;

    auto customTc = static_cast<const CustomToolchain *>(&other);
    return m_makeCommand == customTc->m_makeCommand
            && compilerCommand() == customTc->compilerCommand()
            && targetAbi() == customTc->targetAbi()
            && m_predefinedMacros == customTc->m_predefinedMacros
            && m_builtInHeaderPaths == customTc->m_builtInHeaderPaths;
}

Id CustomToolchain::outputParserId() const
{
    return m_outputParserId;
}

void CustomToolchain::setOutputParserId(Id parserId)
{
    if (m_outputParserId == parserId)
        return;
    m_outputParserId = parserId;
    toolChainUpdated();
}

QList<CustomToolchain::Parser> CustomToolchain::parsers()
{
    QList<CustomToolchain::Parser> result;
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
// CustomToolchainConfigWidget
// --------------------------------------------------------------------------

class CustomToolchainConfigWidget final : public ToolchainConfigWidget
{
public:
    explicit CustomToolchainConfigWidget(const ToolchainBundle &bundle);

private:
    void updateSummaries(TextEditDetailsWidget *detailsWidget);
    void errorParserChanged(int index = -1);

    void applyImpl() override;
    void discardImpl() override { setFromToolchain(); }
    bool isDirtyImpl() const override;
    void makeReadOnlyImpl() override;

    void setFromToolchain();

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

CustomToolchainConfigWidget::CustomToolchainConfigWidget(const ToolchainBundle &bundle) :
    ToolchainConfigWidget(bundle),
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
    const QList<CustomToolchain::Parser> parsers = CustomToolchain::parsers();
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
    m_makeCommand->setExpectedKind(PathChooser::ExistingCommand);
    m_makeCommand->setHistoryCompleter("PE.MakeCommand.History");
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

    connect(m_makeCommand, &PathChooser::rawPathChanged, this, &ToolchainConfigWidget::dirty);
    connect(m_abiWidget, &AbiWidget::abiChanged, this, &ToolchainConfigWidget::dirty);
    connect(m_predefinedMacros, &QPlainTextEdit::textChanged,
            this, [this] { updateSummaries(m_predefinedDetails); });
    connect(m_headerPaths, &QPlainTextEdit::textChanged,
            this, [this] { updateSummaries(m_headerDetails); });
    connect(m_cxx11Flags, &QLineEdit::textChanged, this, &ToolchainConfigWidget::dirty);
    connect(m_mkspecs, &QLineEdit::textChanged, this, &ToolchainConfigWidget::dirty);
    connect(m_errorParserComboBox, &QComboBox::currentIndexChanged,
            this, &CustomToolchainConfigWidget::errorParserChanged);
    errorParserChanged();
}

void CustomToolchainConfigWidget::updateSummaries(TextEditDetailsWidget *detailsWidget)
{
    detailsWidget->updateSummaryText();
    emit dirty();
}

void CustomToolchainConfigWidget::errorParserChanged(int )
{
    emit dirty();
}

void CustomToolchainConfigWidget::applyImpl()
{
    if (bundle().isAutoDetected())
        return;

    bundle().setTargetAbi(m_abiWidget->currentAbi());
    const Macros macros = Utils::transform<QVector>(
        m_predefinedDetails->text().split('\n', Qt::SkipEmptyParts),
        [](const QString &m) {
            return Macro::fromKeyValue(m);
        });
    bundle().forEach<CustomToolchain>([&](CustomToolchain &tc) {
        tc.setMakeCommand(m_makeCommand->filePath());
        tc.setPredefinedMacros(macros);
        tc.setHeaderPaths(m_headerDetails->entries());
        tc.setCxx11Flags(m_cxx11Flags->text().split(QLatin1Char(',')));
        tc.setMkspecs(m_mkspecs->text());
        tc.setOutputParserId(Id::fromSetting(m_errorParserComboBox->currentData()));
    });

    // Refresh with actual data from the toolchain. This shows what e.g. the
    // macro parser did with the input.
    setFromToolchain();
}

void CustomToolchainConfigWidget::setFromToolchain()
{
    // subwidgets are not yet connected!
    QSignalBlocker blocker(this);
    m_makeCommand->setFilePath(bundle().makeCommand(Environment()));
    m_abiWidget->setAbis(Abis(), bundle().targetAbi());
    const QStringList macroLines = Utils::transform<QList>(
        bundle().get(&CustomToolchain::rawPredefinedMacros),
        [](const Macro &m) { return QString::fromUtf8(m.toKeyValue(QByteArray())); });
    m_predefinedMacros->setPlainText(macroLines.join('\n'));
    m_headerPaths->setPlainText(bundle().get(&CustomToolchain::headerPathsList).join('\n'));
    m_cxx11Flags->setText(bundle().get(&CustomToolchain::cxx11Flags).join(QLatin1Char(',')));
    m_mkspecs->setText(bundle().get(&CustomToolchain::mkspecs));
    const int index = m_errorParserComboBox->findData(
        bundle().get(&CustomToolchain::outputParserId).toSetting());
    m_errorParserComboBox->setCurrentIndex(index);
}

bool CustomToolchainConfigWidget::isDirtyImpl() const
{
    return m_makeCommand->filePath() != bundle().makeCommand({})
           || m_abiWidget->currentAbi() != bundle().targetAbi()
           || Macro::toMacros(m_predefinedDetails->text().toUtf8())
                  != bundle().get(&CustomToolchain::rawPredefinedMacros)
           || m_headerDetails->entries() != bundle().get(&CustomToolchain::headerPathsList)
           || m_cxx11Flags->text().split(QLatin1Char(','))
                  != bundle().get(&CustomToolchain::cxx11Flags)
           || m_mkspecs->text() != bundle().get(&CustomToolchain::mkspecs)
           || Id::fromSetting(m_errorParserComboBox->currentData())
                  == bundle().get(&CustomToolchain::outputParserId);
}

void CustomToolchainConfigWidget::makeReadOnlyImpl()
{
    m_mainLayout->setEnabled(false);
}

// CustomToolchainFactory

class CustomToolchainFactory final : public ToolchainFactory
{
public:
    CustomToolchainFactory()
    {
        setDisplayName(Tr::tr("Custom"));
        setSupportedToolchainType(Constants::CUSTOM_TOOLCHAIN_TYPEID);
        setSupportedLanguages({Constants::C_LANGUAGE_ID, Constants::CXX_LANGUAGE_ID});
        setToolchainConstructor([] { return new CustomToolchain; });
        setUserCreatable(true);
    }

private:
    std::unique_ptr<ToolchainConfigWidget> createConfigurationWidget(
        const ToolchainBundle &bundle) const override
    {
        return std::make_unique<CustomToolchainConfigWidget>(bundle);
    }

    FilePath correspondingCompilerCommand(const FilePath &srcPath, Id targetLang) const override
    {
        static const std::pair<QString, QString> patternPairs[]
            = {{"gcc", "g++"}, {"clang", "clang++"}, {"icc", "icpc"}};
        for (const auto &[cPattern, cxxPattern] : patternPairs) {
            if (const FilePath &targetPath = GccToolchain::correspondingCompilerCommand(
                    srcPath, targetLang, cPattern, cxxPattern);
                targetPath != srcPath) {
                return targetPath;
            }
        }
        return srcPath;
    }
};

void setupCustomToolchain()
{
    static CustomToolchainFactory theCustomToolchainFactory;
}

} // ProjectExplorer::Internal
