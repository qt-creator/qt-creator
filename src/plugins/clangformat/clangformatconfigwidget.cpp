// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatconfigwidget.h"

#include "clangformatconstants.h"
#include "clangformatfile.h"
#include "clangformatindenter.h"
#include "clangformattr.h"
#include "clangformatutils.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <cppeditor/cppcodestylesettings.h>
#include <cppeditor/cppcodestylesettingspage.h>
#include <cppeditor/cppcodestylesnippets.h>
#include <cppeditor/cpphighlighter.h>
#include <cppeditor/cpptoolssettings.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/editorconfiguration.h>
#include <projectexplorer/project.h>

#include <texteditor/displaysettings.h>
#include <texteditor/icodestylepreferences.h>
#include <texteditor/snippets/snippeteditor.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>

#include <utils/guard.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QVBoxLayout>

#include <memory>

using namespace ProjectExplorer;
using namespace Utils;

namespace ClangFormat {

template<typename... Args>
static void invokeMethodForLanguageClientManager(const char *method, Args &&...args)
{
    QObject *languageClientManager = ExtensionSystem::PluginManager::getObjectByName(
        "LanguageClientManager");
    if (!languageClientManager)
        return;
    QMetaObject::invokeMethod(languageClientManager, method, args...);
}

bool ClangFormatConfigWidget::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Wheel && qobject_cast<QComboBox *>(object)) {
        event->ignore();
        return true;
    }
    return QWidget::eventFilter(object, event);
}

ClangFormatConfigWidget::ClangFormatConfigWidget(
    const Project *project, TextEditor::ICodeStylePreferences *codeStyle, QWidget *parent)
    : CodeStyleEditorWidget(parent)
{
    m_project = project;
    m_config = std::make_unique<ClangFormatFile>(codeStyle->currentPreferences());

    createStyleFileIfNeeded(!m_project);

    initPreview(codeStyle);
    initEditor();

    using namespace Layouting;

    Column {
        m_clangVersion,
        Row { m_editorScrollArea, m_preview },
        Row {m_clangFileIsCorrectText, st}
    }.attachTo(this);

    connect(codeStyle, &TextEditor::ICodeStylePreferences::currentPreferencesChanged,
            this, &ClangFormatConfigWidget::slotCodeStyleChanged);

    connect(codeStyle, &TextEditor::ICodeStylePreferences::aboutToBeRemoved,
            this, &ClangFormatFile::removeClangFormatFileForStylePreferences);

    connect(codeStyle, &TextEditor::ICodeStylePreferences::aboutToBeCopied,
            this, &ClangFormatFile::copyClangFormatFileBasedOnStylePreferences);

    slotCodeStyleChanged(codeStyle->currentPreferences());

    reopenClangFormatDocument();
    updatePreview();
}

ClangFormatConfigWidget::~ClangFormatConfigWidget()
{
    auto doc = qobject_cast<TextEditor::TextDocument *>(m_editor->document());
    invokeMethodForLanguageClientManager("documentClosed", Q_ARG(Core::IDocument *, doc));
}

void ClangFormatConfigWidget::slotCodeStyleChanged(TextEditor::ICodeStylePreferences *codeStyle)
{
    if (!codeStyle)
        return;
    m_config.reset(new ClangFormatFile(codeStyle));
    m_config->setIsReadOnly(codeStyle->isReadOnly());

    reopenClangFormatDocument();
    updateReadOnlyState();
    updatePreview();
}

void ClangFormatConfigWidget::updateReadOnlyState()
{
    const bool isReadOnly = m_config->isReadOnly() || !m_useCustomSettings;
    setDisabled(isReadOnly);
}

void ClangFormatConfigWidget::initEditor()
{
    m_editorScrollArea = new QScrollArea();
    Core::EditorFactories factories = Core::IEditorFactory::preferredEditorTypes(
        m_config->filePath());
    Core::IEditorFactory *factory = factories.takeFirst();
    m_editor.reset(factory->createEditor());

    m_editor->document()->open(m_config->filePath(), m_config->filePath());
    m_editor->widget()->adjustSize();

    invokeMethodForLanguageClientManager("documentOpened",
                                         Q_ARG(Core::IDocument *, m_editor->document()));
    invokeMethodForLanguageClientManager("editorOpened",
                                         Q_ARG(Core::IEditor *, m_editor.get()));

    m_editorWidget = m_editor->widget();

    m_editorScrollArea->setWidget(m_editor->widget());
    m_editorScrollArea->setWidgetResizable(true);

    m_clangFileIsCorrectText = new InfoLabel("", Utils::InfoLabel::Ok);
    m_clangFileIsCorrectText->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_clangFileIsCorrectText->hide();

    m_clangVersion = new QLabel(Tr::tr("Current ClangFormat version: %1.").arg(LLVM_VERSION_STRING),
                                this);

    connect(m_editor->document(), &TextEditor::TextDocument::contentsChanged, this, [this] {
        clang::format::FormatStyle currentSettingsStyle{};
        const Utils::Result<> success
            = parseConfigurationContent(m_editor->document()->contents().toStdString(),
                                        currentSettingsStyle);

        if (success) {
            m_clangFileIsCorrectText->hide();
            m_indenter->setOverriddenStyle(currentSettingsStyle);
            updatePreview();
            return;
        }
        m_clangFileIsCorrectText->show();
        m_clangFileIsCorrectText->setText(Tr::tr("Warning:") + " " + success.error());
        m_clangFileIsCorrectText->setType(Utils::InfoLabel::Warning);
    });

    QShortcut *completionSC = new QShortcut(QKeySequence("Ctrl+Space"), this);
    connect(completionSC, &QShortcut::activated, this, [this] {
        if (auto editor = qobject_cast<TextEditor::BaseTextEditor *>(m_editor.get()))
            editor->editorWidget()->invokeAssist(TextEditor::Completion);
    });

    QShortcut *saveSC = new QShortcut(QKeySequence("Ctrl+S"), this);
    connect(saveSC, &QShortcut::activated, this, [this] { apply(); });
}

void ClangFormatConfigWidget::initPreview(TextEditor::ICodeStylePreferences *codeStyle)
{
    FilePath fileName = m_project ? m_project->projectFilePath().pathAppended("snippet.cpp")
                                  : Core::ICore::userResourcePath("snippet.cpp");

    m_preview = new TextEditor::SnippetEditorWidget(this);
    TextEditor::DisplaySettings displaySettings = m_preview->displaySettings();
    displaySettings.m_visualizeWhitespace = true;
    m_preview->setDisplaySettings(displaySettings);
    m_preview->setPlainText(QLatin1String(CppEditor::Constants::DEFAULT_CODE_STYLE_SNIPPETS[0]));
    m_indenter = new ClangFormatIndenter(m_preview->document());
    m_indenter->setOverriddenPreferences(codeStyle);
    m_preview->textDocument()->setIndenter(m_indenter);
    m_preview->textDocument()->setFontSettings(TextEditor::TextEditorSettings::fontSettings());
    m_preview->textDocument()->resetSyntaxHighlighter(
        [] { return new CppEditor::CppHighlighter(); });
    m_indenter->setFileName(fileName);
    m_preview->show();
}

static clang::format::FormatStyle constructStyle(const QByteArray &baseStyle = QByteArray())
{
    if (!baseStyle.isEmpty()) {
        // Try to get the style for this base style.
        llvm::Expected<clang::format::FormatStyle> style
            = clang::format::getStyle(baseStyle.toStdString(), "dummy.cpp", baseStyle.toStdString());
        if (style)
            return *style;

        handleAllErrors(style.takeError(), [](const llvm::ErrorInfoBase &) {
            // do nothing
        });
        // Fallthrough to the default style.
    }
    return qtcStyle();
}

FilePath ClangFormatConfigWidget::globalPath()
{
    return Core::ICore::userResourcePath();
}

FilePath ClangFormatConfigWidget::projectPath()
{
    if (m_project)
        return globalPath().pathAppended("clang-format/" + projectUniqueId(m_project));

    return {};
}

void ClangFormatConfigWidget::createStyleFileIfNeeded(bool isGlobal)
{
    const FilePath path = isGlobal ? globalPath() : projectPath();
    const FilePath configFile = path / Constants::SETTINGS_FILE_NAME;

    if (configFile.exists())
        return;

    path.ensureWritableDir();
    if (!isGlobal) {
        FilePath possibleProjectConfig = m_project->rootProjectDirectory()
                                         / Constants::SETTINGS_FILE_NAME;
        if (possibleProjectConfig.exists()) {
            // Just copy the .clang-format if current project has one.
            possibleProjectConfig.copyFile(configFile);
            return;
        }
    }

    const std::string config = clang::format::configurationAsText(constructStyle());
    configFile.writeFileContents(QByteArray::fromStdString(config));
}

void ClangFormatConfigWidget::updatePreview()
{
    QTextCursor cursor(m_preview->document());
    cursor.setPosition(0);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    m_preview->textDocument()->autoFormatOrIndent(cursor);
}

void ClangFormatConfigWidget::reopenClangFormatDocument()
{
    GuardLocker locker(m_ignoreChanges);

    if (m_editor->document()->open(m_config->filePath(), m_config->filePath())) {
        invokeMethodForLanguageClientManager("documentOpened",
                                             Q_ARG(Core::IDocument *, m_editor->document()));
    }
}

void ClangFormatConfigWidget::apply()
{
    if (!m_editorWidget->isEnabled())
        return;

    m_editor->document()->save(m_config->filePath());
}

void ClangFormatConfigWidget::onUseCustomSettingsChanged(bool doUse)
{
    m_useCustomSettings = doUse;
    updateReadOnlyState();
}

} // namespace ClangFormat
