// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatsettings.h"

#include "../beautifierconstants.h"
#include "../beautifierplugin.h"
#include "../beautifiertr.h"
#include "../configurationpanel.h"

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QDateTime>
#include <QXmlStreamWriter>
#include <QButtonGroup>
#include <QComboBox>
#include <QGroupBox>
#include <QRadioButton>

using namespace Utils;

namespace Beautifier::Internal {

const char SETTINGS_NAME[]               = "clangformat";

ClangFormatSettings::ClangFormatSettings()
    : AbstractSettings(SETTINGS_NAME, ".clang-format")
{
    command.setDefaultValue("clang-format");
    command.setPromptDialogTitle(BeautifierPlugin::msgCommandPromptDialogTitle("Clang Format"));
    command.setLabelText(Tr::tr("Clang Format command:"));

    registerAspect(&usePredefinedStyle);
    usePredefinedStyle.setSettingsKey("usePredefinedStyle");
    usePredefinedStyle.setLabelText(Tr::tr("Use predefined style:"));
    usePredefinedStyle.setDefaultValue(true);

    registerAspect(&predefinedStyle);
    predefinedStyle.setSettingsKey("predefinedStyle");
    predefinedStyle.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    predefinedStyle.addOption("LLVM");
    predefinedStyle.addOption("Google");
    predefinedStyle.addOption("Chromium");
    predefinedStyle.addOption("Mozilla");
    predefinedStyle.addOption("WebKit");
    predefinedStyle.addOption("File");
    predefinedStyle.setDefaultValue("LLVM");

    registerAspect(&fallbackStyle);
    fallbackStyle.setSettingsKey("fallbackStyle");
    fallbackStyle.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    fallbackStyle.addOption("Default");
    fallbackStyle.addOption("None");
    fallbackStyle.addOption("LLVM");
    fallbackStyle.addOption("Google");
    fallbackStyle.addOption("Chromium");
    fallbackStyle.addOption("Mozilla");
    fallbackStyle.addOption("WebKit");
    fallbackStyle.setDefaultValue("Default");

    registerAspect(&predefinedStyle);
    predefinedStyle.setSettingsKey("predefinedStyle");
    predefinedStyle.setDefaultValue("LLVM");

    registerAspect(&customStyle);
    customStyle.setSettingsKey("customStyle");

    documentationFilePath.setFilePath(Core::ICore::userResourcePath(Constants::SETTINGS_DIRNAME)
        .pathAppended(Constants::DOCUMENTATION_DIRNAME)
        .pathAppended(SETTINGS_NAME).stringAppended(".xml"));

    read();
}

void ClangFormatSettings::createDocumentationFile() const
{
    QFile file(documentationFilePath().toFSPathString());
    const QFileInfo fi(file);
    if (!fi.exists())
        fi.dir().mkpath(fi.absolutePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return;

    QXmlStreamWriter stream(&file);
    stream.setAutoFormatting(true);
    stream.writeStartDocument("1.0", true);
    stream.writeComment("Created " + QDateTime::currentDateTime().toString(Qt::ISODate));
    stream.writeStartElement(Constants::DOCUMENTATION_XMLROOT);

    const QStringList lines = {
        "BasedOnStyle {string: LLVM, Google, Chromium, Mozilla, WebKit}",
        "AccessModifierOffset {int}",
        "AlignEscapedNewlinesLeft {bool}",
        "AlignTrailingComments {bool}",
        "AllowAllParametersOfDeclarationOnNextLine {bool}",
        "AllowShortFunctionsOnASingleLine {bool}",
        "AllowShortIfStatementsOnASingleLine {bool}",
        "AllowShortLoopsOnASingleLine {bool}",
        "AlwaysBreakBeforeMultilineStrings {bool}",
        "AlwaysBreakTemplateDeclarations {bool}",
        "BinPackParameters {bool}",
        "BreakBeforeBinaryOperators {bool}",
        "BreakBeforeBraces {BraceBreakingStyle: BS_Attach, BS_Linux, BS_Stroustrup, BS_Allman, BS_GNU}",
        "BreakBeforeTernaryOperators {bool}",
        "BreakConstructorInitializersBeforeComma {bool}",
        "ColumnLimit {unsigned}",
        "CommentPragmas {string}",
        "ConstructorInitializerAllOnOneLineOrOnePerLine {bool}",
        "ConstructorInitializerIndentWidth {unsigned}",
        "ContinuationIndentWidth {unsigned}",
        "Cpp11BracedListStyle {bool}",
        "IndentCaseLabels {bool}",
        "IndentFunctionDeclarationAfterType {bool}",
        "IndentWidth {unsigned}",
        "Language {LanguageKind: LK_None, LK_Cpp, LK_JavaScript, LK_Proto}",
        "MaxEmptyLinesToKeep {unsigned}",
        "NamespaceIndentation {NamespaceIndentationKind: NI_None, NI_Inner, NI_All}",
        "ObjCSpaceAfterProperty {bool}",
        "ObjCSpaceBeforeProtocolList {bool}",
        "PenaltyBreakBeforeFirstCallParameter {unsigned}",
        "PenaltyBreakComment {unsigned}",
        "PenaltyBreakFirstLessLess {unsigned}",
        "PenaltyBreakString {unsigned}",
        "PenaltyExcessCharacter {unsigned}",
        "PenaltyReturnTypeOnItsOwnLine {unsigned}",
        "PointerBindsToType {bool}",
        "SpaceBeforeAssignmentOperators {bool}",
        "SpaceBeforeParens {SpaceBeforeParensOptions: SBPO_Never, SBPO_ControlStatements, SBPO_Always}",
        "SpaceInEmptyParentheses {bool}",
        "SpacesBeforeTrailingComments {unsigned}",
        "SpacesInAngles {bool}",
        "SpacesInCStyleCastParentheses {bool}",
        "SpacesInContainerLiterals {bool}",
        "SpacesInParentheses {bool}",
        "Standard {LanguageStandard: LS_Cpp03, LS_Cpp11, LS_Auto}",
        "TabWidth {unsigned}",
        "UseTab {UseTabStyle: UT_Never, UT_ForIndentation, UT_Always}"
    };

    for (const QString& line : lines) {
        const int firstSpace = line.indexOf(' ');
        const QString keyword = line.left(firstSpace);
        const QString options = line.right(line.size() - firstSpace).trimmed();
        const QString text = "<p><span class=\"option\">" + keyword
                + "</span> <span class=\"param\">" + options
                + "</span></p><p>" + Tr::tr("No description available.") + "</p>";
        stream.writeStartElement(Constants::DOCUMENTATION_XMLENTRY);
        stream.writeTextElement(Constants::DOCUMENTATION_XMLKEY, keyword);
        stream.writeTextElement(Constants::DOCUMENTATION_XMLDOC, text);
        stream.writeEndElement();
    }

    stream.writeEndElement();
    stream.writeEndDocument();
}

QStringList ClangFormatSettings::completerWords()
{
    return {
        "LLVM",
        "Google",
        "Chromium",
        "Mozilla",
        "WebKit",
        "BS_Attach",
        "BS_Linux",
        "BS_Stroustrup",
        "BS_Allman",
        "NI_None",
        "NI_Inner",
        "NI_All",
        "LS_Cpp03",
        "LS_Cpp11",
        "LS_Auto",
        "UT_Never",
        "UT_ForIndentation",
        "UT_Always"
    };
}

QString ClangFormatSettings::styleFileName(const QString &key) const
{
    return m_styleDir.absolutePath() + '/' + key + '/' + m_ending;
}

void ClangFormatSettings::readStyles()
{
    const QStringList dirs = m_styleDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for (const QString &dir : dirs) {
        QFile file(m_styleDir.absoluteFilePath(dir + '/' + m_ending));
        if (file.open(QIODevice::ReadOnly))
            m_styles.insert(dir, QString::fromLocal8Bit(file.readAll()));
    }
}

class ClangFormatOptionsPageWidget : public Core::IOptionsPageWidget
{
public:
    explicit ClangFormatOptionsPageWidget(ClangFormatSettings *settings)
    {
        ClangFormatSettings &s = *settings;
        QGroupBox *options = nullptr;

        auto predefinedStyleButton = new QRadioButton;
        s.usePredefinedStyle.adoptButton(predefinedStyleButton);

        auto customizedStyleButton = new QRadioButton(Tr::tr("Use customized style:"));

        auto styleButtonGroup = new QButtonGroup;
        styleButtonGroup->addButton(predefinedStyleButton);
        styleButtonGroup->addButton(customizedStyleButton);

        auto configurations = new ConfigurationPanel(this);
        configurations->setSettings(&s);
        configurations->setCurrentConfiguration(s.customStyle());

        using namespace Layouting;

        auto fallbackBlob = Row { noMargin, Tr::tr("Fallback style:"), s.fallbackStyle }.emerge();

        auto predefinedBlob = Column { noMargin, s.predefinedStyle, fallbackBlob }.emerge();

        Column {
            Group {
                title(Tr::tr("Configuration")),
                Form {
                    s.command, br,
                    s.supportedMimeTypes
                }
            },
            Group {
                title(Tr::tr("Options")),
                bindTo(&options),
                Form {
                    s.usePredefinedStyle, predefinedBlob, br,
                    customizedStyleButton, configurations,
                },
            },
            st
        }.attachTo(this);

        if (s.usePredefinedStyle.value())
            predefinedStyleButton->click();
        else
            customizedStyleButton->click();

        const auto updateEnabled = [&s, styleButtonGroup, predefinedBlob, fallbackBlob,
                                    configurations, predefinedStyleButton] {
            const bool predefSelected = styleButtonGroup->checkedButton() == predefinedStyleButton;
            predefinedBlob->setEnabled(predefSelected);
            fallbackBlob->setEnabled(predefSelected && s.predefinedStyle.volatileValue().toInt() == 5); // File
            configurations->setEnabled(!predefSelected);
        };
        updateEnabled();
        connect(styleButtonGroup, &QButtonGroup::buttonClicked, this, updateEnabled);
        connect(&s.predefinedStyle, &SelectionAspect::volatileValueChanged, this, updateEnabled);

        setOnApply([settings, configurations] {
            settings->customStyle.setValue(configurations->currentConfiguration());
            settings->save();
        });

        s.read();

        connect(s.command.pathChooser(), &PathChooser::validChanged, options, &QWidget::setEnabled);
        options->setEnabled(s.command.pathChooser()->isValid());
    }
};

ClangFormatOptionsPage::ClangFormatOptionsPage(ClangFormatSettings *settings)
{
    setId("ClangFormat");
    setDisplayName(Tr::tr("Clang Format"));
    setCategory(Constants::OPTION_CATEGORY);
    setWidgetCreator([settings] { return new ClangFormatOptionsPageWidget(settings); });
}

} // Beautifier::Internal
