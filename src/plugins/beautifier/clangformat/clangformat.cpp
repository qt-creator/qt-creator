// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Tested with version 3.3, 3.4 and 3.4.1

#include "clangformat.h"

#include "../beautifierconstants.h"
#include "../beautifiertool.h"
#include "../beautifiertr.h"
#include "../configurationpanel.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <texteditor/formattexteditor.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QAction>
#include <QButtonGroup>
#include <QComboBox>
#include <QDateTime>
#include <QGroupBox>
#include <QMenu>
#include <QRadioButton>
#include <QTextBlock>
#include <QTextCodec>
#include <QXmlStreamWriter>

using namespace TextEditor;
using namespace Utils;

namespace Beautifier::Internal {

const char SETTINGS_NAME[]               = "clangformat";

class ClangFormatSettings : public AbstractSettings
{
public:
    ClangFormatSettings()
        : AbstractSettings(SETTINGS_NAME, ".clang-format")
    {
        command.setDefaultValue("clang-format");
        command.setPromptDialogTitle(BeautifierTool::msgCommandPromptDialogTitle("Clang Format"));
        command.setLabelText(Tr::tr("Clang Format command:"));

        usePredefinedStyle.setSettingsKey("usePredefinedStyle");
        usePredefinedStyle.setDefaultValue(true);
        usePredefinedStyle.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
        usePredefinedStyle.setLabelText(Tr::tr("Use predefined style:"));

        predefinedStyle.setSettingsKey("predefinedStyle");
        predefinedStyle.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
        predefinedStyle.addOption("LLVM");
        predefinedStyle.addOption("Google");
        predefinedStyle.addOption("Chromium");
        predefinedStyle.addOption("Mozilla");
        predefinedStyle.addOption("WebKit");
        predefinedStyle.addOption("File");
        predefinedStyle.setDefaultValue("LLVM");

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

        customStyle.setSettingsKey("customStyle");

        documentationFilePath = Core::ICore::userResourcePath(Constants::SETTINGS_DIRNAME)
                                    .pathAppended(Constants::DOCUMENTATION_DIRNAME)
                                    .pathAppended(SETTINGS_NAME).stringAppended(".xml");

        read();
    }

    void createDocumentationFile() const override;

    QStringList completerWords() override;

    BoolAspect usePredefinedStyle{this};
    SelectionAspect predefinedStyle{this};
    SelectionAspect fallbackStyle{this};
    StringAspect customStyle{this};

    Utils::FilePath styleFileName(const QString &key) const override;

private:
    void readStyles() override;
};

void ClangFormatSettings::createDocumentationFile() const
{
    QFile file(documentationFilePath.toFSPathString());
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

FilePath ClangFormatSettings::styleFileName(const QString &key) const
{
    return m_styleDir / key / m_ending;
}

void ClangFormatSettings::readStyles()
{
    const FilePaths dirs = m_styleDir.dirEntries(QDir::AllDirs | QDir::NoDotAndDotDot);
    for (const FilePath &dir : dirs) {
        if (auto contents = dir.pathAppended(m_ending).fileContents())
            m_styles.insert(dir.fileName(), QString::fromLocal8Bit(*contents));
    }
}

static ClangFormatSettings &settings()
{
    static ClangFormatSettings theSettings;
    return theSettings;
}

class ClangFormatSettingsPageWidget : public Core::IOptionsPageWidget
{
public:
    ClangFormatSettingsPageWidget()
    {
        ClangFormatSettings &s = settings();
        QGroupBox *options = nullptr;

        auto predefinedStyleButton = new QRadioButton;
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
        // clang-format off
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
                    s.usePredefinedStyle.adoptButton(predefinedStyleButton), predefinedBlob, br,
                    customizedStyleButton, configurations,
                },
            },
            st
        }.attachTo(this);
        // clang-format on

        if (s.usePredefinedStyle.value())
            predefinedStyleButton->click();
        else
            customizedStyleButton->click();

        const auto updateEnabled = [&s, styleButtonGroup, predefinedBlob, fallbackBlob,
                                    configurations, predefinedStyleButton] {
            const bool predefSelected = styleButtonGroup->checkedButton() == predefinedStyleButton;
            predefinedBlob->setEnabled(predefSelected);
            fallbackBlob->setEnabled(predefSelected && s.predefinedStyle.volatileValue() == 5); // File
            configurations->setEnabled(!predefSelected);
        };
        updateEnabled();
        connect(styleButtonGroup, &QButtonGroup::buttonClicked, this, updateEnabled);
        connect(&s.predefinedStyle, &SelectionAspect::volatileValueChanged, this, updateEnabled);

        setOnApply([configurations, customizedStyleButton] {
            settings().usePredefinedStyle.setValue(!customizedStyleButton->isChecked());
            settings().customStyle.setValue(configurations->currentConfiguration());
            settings().apply();
            settings().save();
        });

        s.read();

        connect(s.command.pathChooser(), &PathChooser::validChanged, options, &QWidget::setEnabled);
        options->setEnabled(s.command.pathChooser()->isValid());
    }
};

// ClangFormat

ClangFormat::ClangFormat()
{
    Core::ActionContainer *menu = Core::ActionManager::createMenu("ClangFormat.Menu");
    menu->menu()->setTitle(Tr::tr("&ClangFormat"));

    m_formatFile = new QAction(msgFormatCurrentFile(), this);
    Core::Command *cmd
            = Core::ActionManager::registerAction(m_formatFile, "ClangFormat.FormatFile");
    menu->addAction(cmd);
    connect(m_formatFile, &QAction::triggered, this, &ClangFormat::formatFile);

    m_formatLines = new QAction(msgFormatLines(), this);
    cmd = Core::ActionManager::registerAction(m_formatLines, "ClangFormat.FormatLines");
    menu->addAction(cmd);
    connect(m_formatLines, &QAction::triggered, this, &ClangFormat::formatLines);

    m_formatRange = new QAction(msgFormatAtCursor(), this);
    cmd = Core::ActionManager::registerAction(m_formatRange, "ClangFormat.FormatAtCursor");
    menu->addAction(cmd);
    connect(m_formatRange, &QAction::triggered, this, &ClangFormat::formatAtCursor);

    m_disableFormattingSelectedText = new QAction(msgDisableFormattingSelectedText(), this);
    cmd = Core::ActionManager::registerAction(
        m_disableFormattingSelectedText, "ClangFormat.DisableFormattingSelectedText");
    menu->addAction(cmd);
    connect(m_disableFormattingSelectedText, &QAction::triggered,
            this, &ClangFormat::disableFormattingSelectedText);

    Core::ActionManager::actionContainer(Constants::MENU_ID)->addMenu(menu);

    connect(&settings().supportedMimeTypes, &BaseAspect::changed,
            this, [this] { updateActions(Core::EditorManager::currentEditor()); });
}

QString ClangFormat::id() const
{
    return "ClangFormat";
}

void ClangFormat::updateActions(Core::IEditor *editor)
{
    const bool enabled = editor && settings().isApplicable(editor->document());
    m_formatFile->setEnabled(enabled);
    m_formatRange->setEnabled(enabled);
}

void ClangFormat::formatFile()
{
    formatCurrentFile(textCommand());
}

void ClangFormat::formatAtPosition(const int pos, const int length)
{
    const TextEditorWidget *widget = TextEditorWidget::currentTextEditorWidget();
    if (!widget)
        return;

    const QTextCodec *codec = widget->textDocument()->codec();
    if (!codec) {
        formatCurrentFile(textCommand(pos, length));
        return;
    }

    const QString &text = widget->textAt(0, pos + length);
    const QStringView buffer(text);
    const int encodedOffset = codec->fromUnicode(buffer.left(pos)).size();
    const int encodedLength = codec->fromUnicode(buffer.mid(pos, length)).size();
    formatCurrentFile(textCommand(encodedOffset, encodedLength));
}

void ClangFormat::formatAtCursor()
{
    const TextEditorWidget *widget = TextEditorWidget::currentTextEditorWidget();
    if (!widget)
        return;

    const QTextCursor tc = widget->textCursor();

    if (tc.hasSelection()) {
        const int selectionStart = tc.selectionStart();
        formatAtPosition(selectionStart, tc.selectionEnd() - selectionStart);
    } else {
        // Pretend that the current line was selected.
        // Note that clang-format will extend the range to the next bigger
        // syntactic construct if needed.
        const QTextBlock block = tc.block();
        formatAtPosition(block.position(), block.length());
    }
}

void ClangFormat::formatLines()
{
    const TextEditorWidget *widget = TextEditorWidget::currentTextEditorWidget();
    if (!widget)
        return;

    const QTextCursor tc = widget->textCursor();
    // Current line by default
    int lineStart = tc.blockNumber() + 1;
    int lineEnd = lineStart;

    // Note that clang-format will extend the range to the next bigger
    // syntactic construct if needed.
    if (tc.hasSelection()) {
        const QTextBlock start = tc.document()->findBlock(tc.selectionStart());
        const QTextBlock end = tc.document()->findBlock(tc.selectionEnd());
        lineStart = start.blockNumber() + 1;
        lineEnd = end.blockNumber() + 1;
    }

    auto cmd = textCommand();
    cmd.addOption(QString("-lines=%1:%2").arg(QString::number(lineStart)).arg(QString::number(lineEnd)));
    formatCurrentFile(cmd);
}

void ClangFormat::disableFormattingSelectedText()
{
    TextEditorWidget *widget = TextEditorWidget::currentTextEditorWidget();
    if (!widget)
        return;

    const QTextCursor tc = widget->textCursor();
    if (!tc.hasSelection())
        return;

    // Insert start marker
    const QTextBlock selectionStartBlock = tc.document()->findBlock(tc.selectionStart());
    QTextCursor insertCursor(tc.document());
    insertCursor.beginEditBlock();
    insertCursor.setPosition(selectionStartBlock.position());
    insertCursor.insertText("// clang-format off\n");
    const int positionToRestore = tc.position();

    // Insert end marker
    QTextBlock selectionEndBlock = tc.document()->findBlock(tc.selectionEnd());
    insertCursor.setPosition(selectionEndBlock.position() + selectionEndBlock.length() - 1);
    insertCursor.insertText("\n// clang-format on");
    insertCursor.endEditBlock();

    // Reset the cursor position in order to clear the selection.
    QTextCursor restoreCursor(tc.document());
    restoreCursor.setPosition(positionToRestore);
    widget->setTextCursor(restoreCursor);

    // The indentation of these markers might be undesired, so reformat.
    // This is not optimal because two undo steps will be needed to remove the markers.
    const int reformatTextLength = insertCursor.position() - selectionStartBlock.position();
    formatAtPosition(selectionStartBlock.position(), reformatTextLength);
}

Command ClangFormat::textCommand() const
{
    Command cmd;
    cmd.setExecutable(settings().command());
    cmd.setProcessing(Command::PipeProcessing);

    if (settings().usePredefinedStyle()) {
        const QString predefinedStyle = settings().predefinedStyle.stringValue();
        cmd.addOption("-style=" + predefinedStyle);
        if (predefinedStyle == "File") {
            const QString fallbackStyle = settings().fallbackStyle.stringValue();
            if (fallbackStyle != "Default")
                cmd.addOption("-fallback-style=" + fallbackStyle);
        }

        cmd.addOption("-assume-filename=%file");
    } else {
        cmd.addOption("-style=file");
        const FilePath path = settings().styleFileName(settings().customStyle())
            .absolutePath().pathAppended("%filename");
        cmd.addOption("-assume-filename=" + path.nativePath());
    }

    return cmd;
}

bool ClangFormat::isApplicable(const Core::IDocument *document) const
{
    return settings().isApplicable(document);
}

Command ClangFormat::textCommand(int offset, int length) const
{
    Command cmd = textCommand();
    cmd.addOption("-offset=" + QString::number(offset));
    cmd.addOption("-length=" + QString::number(length));
    return cmd;
}


// ClangFormatSettingsPage

class ClangFormatSettingsPage final : public Core::IOptionsPage
{
public:
    ClangFormatSettingsPage()
    {
        setId("ClangFormat");
        setDisplayName(Tr::tr("Clang Format"));
        setCategory(Constants::OPTION_CATEGORY);
        setWidgetCreator([] { return new ClangFormatSettingsPageWidget; });
    }
};

const ClangFormatSettingsPage settingsPage;

} // Beautifier::Internal
