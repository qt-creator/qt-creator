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
#include "customparserconfigdialog.h"
#include "projectexplorerconstants.h"
#include "toolchainmanager.h"

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/environment.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QFormLayout>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>

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
static const char errorPatternKeyC[] = "ProjectExplorer.CustomToolChain.ErrorPattern";
static const char errorLineNumberCapKeyC[] = "ProjectExplorer.CustomToolChain.ErrorLineNumberCap";
static const char errorFileNameCapKeyC[] = "ProjectExplorer.CustomToolChain.ErrorFileNameCap";
static const char errorMessageCapKeyC[] = "ProjectExplorer.CustomToolChain.ErrorMessageCap";
static const char errorChannelKeyC[] = "ProjectExplorer.CustomToolChain.ErrorChannel";
static const char errorExampleKeyC[] = "ProjectExplorer.CustomToolChain.ErrorExample";
static const char warningPatternKeyC[] = "ProjectExplorer.CustomToolChain.WarningPattern";
static const char warningLineNumberCapKeyC[] = "ProjectExplorer.CustomToolChain.WarningLineNumberCap";
static const char warningFileNameCapKeyC[] = "ProjectExplorer.CustomToolChain.WarningFileNameCap";
static const char warningMessageCapKeyC[] = "ProjectExplorer.CustomToolChain.WarningMessageCap";
static const char warningChannelKeyC[] = "ProjectExplorer.CustomToolChain.WarningChannel";
static const char warningExampleKeyC[] = "ProjectExplorer.CustomToolChain.WarningExample";

// --------------------------------------------------------------------------
// CustomToolChain
// --------------------------------------------------------------------------

CustomToolChain::CustomToolChain(Detection d) :
    ToolChain(Constants::CUSTOM_TOOLCHAIN_TYPEID, d),
    m_outputParser(Gcc)
{ }

CustomToolChain::CustomToolChain(Core::Id language, Detection d) : CustomToolChain(d)
{
    setLanguage(language);
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

ToolChain::PredefinedMacrosRunner CustomToolChain::createPredefinedMacrosRunner() const
{
    const QStringList theMacros = m_predefinedMacros;

    // This runner must be thread-safe!
    return [theMacros](const QStringList &cxxflags){
        QByteArray result;
        QStringList macros = theMacros;
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
    };
}

QByteArray CustomToolChain::predefinedMacros(const QStringList &cxxflags) const
{
    return createPredefinedMacrosRunner()(cxxflags);
}

ToolChain::CompilerFlags CustomToolChain::compilerFlags(const QStringList &cxxflags) const
{
    foreach (const QString &cxx11Flag, m_cxx11Flags)
        if (cxxflags.contains(cxx11Flag))
            return StandardCxx11;
    return NoFlags;
}

WarningFlags CustomToolChain::warningFlags(const QStringList &cxxflags) const
{
    Q_UNUSED(cxxflags);
    return WarningFlags::Default;
}

const QStringList &CustomToolChain::rawPredefinedMacros() const
{
    return m_predefinedMacros;
}

void CustomToolChain::setPredefinedMacros(const QStringList &list)
{
    if (m_predefinedMacros == list)
        return;
    m_predefinedMacros = list;
    toolChainUpdated();
}

ToolChain::SystemHeaderPathsRunner CustomToolChain::createSystemHeaderPathsRunner() const
{
    const QList<HeaderPath> systemHeaderPaths = m_systemHeaderPaths;

    // This runner must be thread-safe!
    return [systemHeaderPaths](const QStringList &cxxFlags, const QString &) {
        QList<HeaderPath> flagHeaderPaths;
        foreach (const QString &cxxFlag, cxxFlags) {
            if (cxxFlag.startsWith(QLatin1String("-I")))
                flagHeaderPaths << HeaderPath(cxxFlag.mid(2).trimmed(), HeaderPath::GlobalHeaderPath);
        }

        return systemHeaderPaths + flagHeaderPaths;
    };
}

QList<HeaderPath> CustomToolChain::systemHeaderPaths(const QStringList &cxxFlags,
                                                     const FileName &fileName) const
{
    return createSystemHeaderPathsRunner()(cxxFlags, fileName.toString());
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

FileNameList CustomToolChain::suggestedMkspecList() const
{
    return m_mkspecs;
}

IOutputParser *CustomToolChain::outputParser() const
{
    switch (m_outputParser) {
    case Gcc: return new GccParser;
    case Clang: return new ClangParser;
    case LinuxIcc: return new LinuxIccParser;
#if defined(Q_OS_WIN)
    case Msvc: return new MsvcParser;
#endif
    case Custom: return new CustomParser(m_customParserSettings);
    default: return nullptr;
    }
}

QStringList CustomToolChain::headerPathsList() const
{
    return Utils::transform(m_systemHeaderPaths, &HeaderPath::path);
}

void CustomToolChain::setHeaderPaths(const QStringList &list)
{
    QList<HeaderPath> tmp = Utils::transform(list, [](const QString &headerPath) {
        return HeaderPath(headerPath.trimmed(), HeaderPath::GlobalHeaderPath);
    });

    if (m_systemHeaderPaths == tmp)
        return;
    m_systemHeaderPaths = tmp;
    toolChainUpdated();
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

QString CustomToolChain::makeCommand(const Environment &) const
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
    Utils::FileNameList tmp
            = Utils::transform(specs.split(QLatin1Char(',')),
                               [](QString fn) { return FileName::fromString(fn); });

    if (tmp == m_mkspecs)
        return;
    m_mkspecs = tmp;
    toolChainUpdated();
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
    data.insert(QLatin1String(outputParserKeyC), m_outputParser);
    data.insert(QLatin1String(errorPatternKeyC), m_customParserSettings.error.pattern());
    data.insert(QLatin1String(errorFileNameCapKeyC), m_customParserSettings.error.fileNameCap());
    data.insert(QLatin1String(errorLineNumberCapKeyC), m_customParserSettings.error.lineNumberCap());
    data.insert(QLatin1String(errorMessageCapKeyC), m_customParserSettings.error.messageCap());
    data.insert(QLatin1String(errorChannelKeyC), m_customParserSettings.error.channel());
    data.insert(QLatin1String(errorExampleKeyC), m_customParserSettings.error.example());
    data.insert(QLatin1String(warningPatternKeyC), m_customParserSettings.warning.pattern());
    data.insert(QLatin1String(warningFileNameCapKeyC), m_customParserSettings.warning.fileNameCap());
    data.insert(QLatin1String(warningLineNumberCapKeyC), m_customParserSettings.warning.lineNumberCap());
    data.insert(QLatin1String(warningMessageCapKeyC), m_customParserSettings.warning.messageCap());
    data.insert(QLatin1String(warningChannelKeyC), m_customParserSettings.warning.channel());
    data.insert(QLatin1String(warningExampleKeyC), m_customParserSettings.warning.example());

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
    m_outputParser = (OutputParser)data.value(QLatin1String(outputParserKeyC)).toInt();
    m_customParserSettings.error.setPattern(data.value(QLatin1String(errorPatternKeyC)).toString());
    m_customParserSettings.error.setFileNameCap(data.value(QLatin1String(errorFileNameCapKeyC)).toInt());
    m_customParserSettings.error.setLineNumberCap(data.value(QLatin1String(errorLineNumberCapKeyC)).toInt());
    m_customParserSettings.error.setMessageCap(data.value(QLatin1String(errorMessageCapKeyC)).toInt());
    m_customParserSettings.error.setChannel(
                static_cast<CustomParserExpression::CustomParserChannel>(data.value(QLatin1String(errorChannelKeyC)).toInt()));
    m_customParserSettings.error.setExample(data.value(QLatin1String(errorExampleKeyC)).toString());
    m_customParserSettings.warning.setPattern(data.value(QLatin1String(warningPatternKeyC)).toString());
    m_customParserSettings.warning.setFileNameCap(data.value(QLatin1String(warningFileNameCapKeyC)).toInt());
    m_customParserSettings.warning.setLineNumberCap(data.value(QLatin1String(warningLineNumberCapKeyC)).toInt());
    m_customParserSettings.warning.setMessageCap(data.value(QLatin1String(warningMessageCapKeyC)).toInt());
    m_customParserSettings.warning.setChannel(
                static_cast<CustomParserExpression::CustomParserChannel>(data.value(QLatin1String(warningChannelKeyC)).toInt()));
    m_customParserSettings.warning.setExample(data.value(QLatin1String(warningExampleKeyC)).toString());

    QTC_ASSERT(m_outputParser >= Gcc && m_outputParser < OutputParserCount, return false);

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
            && m_systemHeaderPaths == customTc->m_systemHeaderPaths;
}

CustomToolChain::OutputParser CustomToolChain::outputParserType() const
{
    return m_outputParser;
}

void CustomToolChain::setOutputParserType(CustomToolChain::OutputParser parser)
{
    if (m_outputParser == parser)
        return;
    m_outputParser = parser;
    toolChainUpdated();
}

CustomParserSettings CustomToolChain::customParserSettings() const
{
    return m_customParserSettings;
}

void CustomToolChain::setCustomParserSettings(const CustomParserSettings &settings)
{
    if (m_customParserSettings == settings)
        return;
    m_customParserSettings = settings;
    toolChainUpdated();
}

QString CustomToolChain::parserName(CustomToolChain::OutputParser parser)
{
    switch (parser) {
    case Gcc: return tr("GCC");
    case Clang: return tr("Clang");
    case LinuxIcc: return tr("ICC");
#if defined(Q_OS_WIN)
    case Msvc: return tr("MSVC");
#endif
    case Custom: return tr("Custom");
    default: return QString();
    }
}

ToolChainConfigWidget *CustomToolChain::configurationWidget()
{
    return new Internal::CustomToolChainConfigWidget(this);
}

namespace Internal {

// --------------------------------------------------------------------------
// CustomToolChainFactory
// --------------------------------------------------------------------------

CustomToolChainFactory::CustomToolChainFactory()
{
    setDisplayName(tr("Custom"));
}

QSet<Core::Id> CustomToolChainFactory::supportedLanguages() const
{
    return ToolChainManager::allLanguages();
}

bool CustomToolChainFactory::canCreate()
{
    return true;
}

ToolChain *CustomToolChainFactory::create(Core::Id language)
{
    return new CustomToolChain(language, ToolChain::ManualDetection);
}

// Used by the ToolChainManager to restore user-generated tool chains
bool CustomToolChainFactory::canRestore(const QVariantMap &data)
{
    return typeIdFromMap(data) == Constants::CUSTOM_TOOLCHAIN_TYPEID;
}

ToolChain *CustomToolChainFactory::restore(const QVariantMap &data)
{
    auto tc = new CustomToolChain(ToolChain::ManualDetection);
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return nullptr;
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
    m_errorParserComboBox(new QComboBox),
    m_customParserSettingsButton(new QPushButton(tr("Custom Parser Settings...")))
{
    Q_ASSERT(tc);

    for (int i = 0; i < CustomToolChain::OutputParserCount; ++i)
        m_errorParserComboBox->addItem(CustomToolChain::parserName((CustomToolChain::OutputParser)i));

    auto parserLayoutWidget = new QWidget;
    auto parserLayout = new QHBoxLayout(parserLayoutWidget);
    parserLayout->setContentsMargins(0, 0, 0, 0);
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
    parserLayout->addWidget(m_customParserSettingsButton);
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
    connect(m_errorParserComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &CustomToolChainConfigWidget::errorParserChanged);
    connect(m_customParserSettingsButton, &QAbstractButton::clicked,
            this, &CustomToolChainConfigWidget::openCustomParserSettingsDialog);
    errorParserChanged(m_errorParserComboBox->currentIndex());
}

void CustomToolChainConfigWidget::updateSummaries()
{
    if (sender() == m_predefinedMacros)
        m_predefinedDetails->updateSummaryText();
    else
        m_headerDetails->updateSummaryText();
    emit dirty();
}

void CustomToolChainConfigWidget::errorParserChanged(int index)
{
    m_customParserSettingsButton->setEnabled(index == m_errorParserComboBox->count() - 1);
    emit dirty();
}

void CustomToolChainConfigWidget::openCustomParserSettingsDialog()
{
    CustomParserConfigDialog dialog;
    dialog.setSettings(m_customParserSettings);

    if (dialog.exec() == QDialog::Accepted) {
        m_customParserSettings = dialog.settings();
        if (dialog.isDirty())
            emit dirty();
    }
}

void CustomToolChainConfigWidget::applyImpl()
{
    if (toolChain()->isAutoDetected())
        return;

    auto tc = static_cast<CustomToolChain *>(toolChain());
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
    tc->setOutputParserType((CustomToolChain::OutputParser)m_errorParserComboBox->currentIndex());
    tc->setCustomParserSettings(m_customParserSettings);
}

void CustomToolChainConfigWidget::setFromToolchain()
{
    // subwidgets are not yet connected!
    bool blocked = blockSignals(true);
    auto tc = static_cast<CustomToolChain *>(toolChain());
    m_compilerCommand->setFileName(tc->compilerCommand());
    m_makeCommand->setFileName(FileName::fromString(tc->makeCommand(Environment())));
    m_abiWidget->setAbis(QList<Abi>(), tc->targetAbi());
    m_predefinedMacros->setPlainText(tc->rawPredefinedMacros().join(QLatin1Char('\n')));
    m_headerPaths->setPlainText(tc->headerPathsList().join(QLatin1Char('\n')));
    m_cxx11Flags->setText(tc->cxx11Flags().join(QLatin1Char(',')));
    m_mkspecs->setText(tc->mkspecs());
    m_errorParserComboBox->setCurrentIndex(tc->outputParserType());
    m_customParserSettings = tc->customParserSettings();
    blockSignals(blocked);
}

bool CustomToolChainConfigWidget::isDirtyImpl() const
{
    auto tc = static_cast<CustomToolChain *>(toolChain());
    Q_ASSERT(tc);
    return m_compilerCommand->fileName() != tc->compilerCommand()
            || m_makeCommand->path() != tc->makeCommand(Environment())
            || m_abiWidget->currentAbi() != tc->targetAbi()
            || m_predefinedDetails->entries() != tc->rawPredefinedMacros()
            || m_headerDetails->entries() != tc->headerPathsList()
            || m_cxx11Flags->text().split(QLatin1Char(',')) != tc->cxx11Flags()
            || m_mkspecs->text() != tc->mkspecs()
            || m_errorParserComboBox->currentIndex() == tc->outputParserType()
            || m_customParserSettings != tc->customParserSettings();
}

void CustomToolChainConfigWidget::makeReadOnlyImpl()
{
    m_mainLayout->setEnabled(false);
}

} // namespace Internal
} // namespace ProjectExplorer
