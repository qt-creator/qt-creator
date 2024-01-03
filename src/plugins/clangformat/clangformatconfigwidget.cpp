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

#include <cppeditor/cppcodestylepreferences.h>
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

static QObject *languageClientPlugin()
{
    return ExtensionSystem::PluginManager::getObjectByName("LanguageClient");
}

class ClangFormatConfigWidget final : public TextEditor::CodeStyleEditorWidget
{
public:
    ClangFormatConfigWidget(TextEditor::ICodeStylePreferences *codeStyle,
                            Project *project,
                            QWidget *parent);

    ~ClangFormatConfigWidget()
    {
        auto doc = qobject_cast<TextEditor::TextDocument *>(m_editor->document());
        QMetaObject::invokeMethod(languageClientPlugin(),
                                  "closeClientForDocument",
                                  Q_ARG(TextEditor::TextDocument *, doc));
        QMetaObject::invokeMethod(languageClientPlugin(),
                                  "closeDocument",
                                  Q_ARG(Core::IDocument *, doc));
    }

    void apply() final;

private:
    bool eventFilter(QObject *object, QEvent *event) final;

    FilePath globalPath();
    FilePath projectPath();
    void createStyleFileIfNeeded(bool isGlobal);

    void initPreview(TextEditor::ICodeStylePreferences *codeStyle);
    void initEditor(TextEditor::ICodeStylePreferences *codeStyle);

    void reopenClangFormatDocument();

    void updatePreview();
    void slotCodeStyleChanged(TextEditor::ICodeStylePreferences *currentPreferences);

    ProjectExplorer::Project *m_project = nullptr;
    QWidget *m_editorWidget = nullptr;
    QScrollArea *m_editorScrollArea = nullptr;
    TextEditor::SnippetEditorWidget *m_preview = nullptr;
    Core::IEditor *m_editor = nullptr;

    std::unique_ptr<ClangFormatFile> m_config;

    Guard m_ignoreChanges;
    QLabel *m_clangVersion;
    QLabel *m_clangFileIsCorrectText;
    QLabel *m_clangFileIsCorrectIcon;
    ClangFormatIndenter *m_indenter;
};

bool ClangFormatConfigWidget::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Wheel && qobject_cast<QComboBox *>(object)) {
        event->ignore();
        return true;
    }
    return QWidget::eventFilter(object, event);
}

ClangFormatConfigWidget::ClangFormatConfigWidget(TextEditor::ICodeStylePreferences *codeStyle,
                                                 Project *project,
                                                 QWidget *parent)
    : CodeStyleEditorWidget(parent)
{
    m_project = project;
    m_config = std::make_unique<ClangFormatFile>(codeStyle->currentPreferences());

    createStyleFileIfNeeded(!m_project);

    initPreview(codeStyle);
    initEditor(codeStyle);

    using namespace Layouting;

    Column {
        m_clangVersion,
        Row { m_editorScrollArea, m_preview },
        Row {m_clangFileIsCorrectIcon, m_clangFileIsCorrectText, st}
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

void ClangFormatConfigWidget::slotCodeStyleChanged(
    TextEditor::ICodeStylePreferences *codeStyle)
{
    if (!codeStyle)
        return;
    m_config.reset(new ClangFormatFile(codeStyle));
    m_config->setIsReadOnly(codeStyle->isReadOnly());

    m_editorWidget->setEnabled(!codeStyle->isReadOnly() && !codeStyle->isTemporarilyReadOnly()
                                && codeStyle->isAdditionalTabVisible());

    reopenClangFormatDocument();
    updatePreview();
}

void ClangFormatConfigWidget::initEditor(TextEditor::ICodeStylePreferences *codeStyle)
{
    m_editorScrollArea = new QScrollArea();
    Core::EditorFactories factories = Core::IEditorFactory::preferredEditorTypes(
        m_config->filePath());
    Core::IEditorFactory *factory = factories.takeFirst();
    m_editor = factory->createEditor();

    QString errorString;
    m_editor->document()->open(&errorString, m_config->filePath(), m_config->filePath());
    m_editor->widget()->adjustSize();

    QMetaObject::invokeMethod(languageClientPlugin(),
                              "openDocument",
                              Q_ARG(Core::IDocument *, m_editor->document()));
    QMetaObject::invokeMethod(languageClientPlugin(),
                              "openEditor",
                              Q_ARG(Core::IEditor *, m_editor));

    m_editorWidget = m_editor->widget();
    m_editorWidget->setEnabled(!codeStyle->isReadOnly() && !codeStyle->isTemporarilyReadOnly()
                               && codeStyle->isAdditionalTabVisible());

    m_editorScrollArea->setWidget(m_editor->widget());
    m_editorScrollArea->setWidgetResizable(true);

    m_clangFileIsCorrectText = new QLabel(Tr::tr("Clang-Format is configured correctly."));
    QPalette paletteCorrect = m_clangFileIsCorrectText->palette();
    paletteCorrect.setColor(QPalette::WindowText, Qt::darkGreen);
    m_clangFileIsCorrectText->setPalette(paletteCorrect);

    m_clangFileIsCorrectIcon = new QLabel(this);
    m_clangFileIsCorrectIcon->setPixmap(Icons::OK.icon().pixmap(16, 16));

    m_clangVersion = new QLabel(Tr::tr("Current ClangFormat version: %1.").arg(LLVM_VERSION_STRING),
                                this);

    connect(m_editor->document(), &TextEditor::TextDocument::contentsChanged, this, [this] {
        clang::format::FormatStyle currentSettingsStyle;
        const bool success
            = parseConfigurationContent(m_editor->document()->contents().toStdString(),
                                        currentSettingsStyle);

        QString text = "";
        Qt::GlobalColor currentColor;
        QPixmap pixmap;
        if (success) {
            text = Tr::tr("Clang-Format is configured correctly.");
            currentColor = Qt::darkGreen;
            pixmap = Icons::OK.icon().pixmap(16, 16);
        } else {
            text = Tr::tr("Clang-Format is not configured correctly.");
            currentColor = Qt::red;
            pixmap = Icons::WARNING.icon().pixmap(16, 16);
        }

        m_clangFileIsCorrectText->setText(text);
        QPalette paletteCorrect = m_clangFileIsCorrectText->palette();
        paletteCorrect.setColor(QPalette::WindowText, currentColor);
        m_clangFileIsCorrectText->setPalette(paletteCorrect);
        m_clangFileIsCorrectIcon->setPixmap(pixmap);

        if (!success)
            return;
        m_indenter->setOverriddenStyle(currentSettingsStyle);
        updatePreview();
    });

    QShortcut *completionSC = new QShortcut(QKeySequence("Ctrl+Space"), this);
    connect(completionSC, &QShortcut::activated, this, [this] {
        if (auto *editor = qobject_cast<TextEditor::BaseTextEditor *>(m_editor))
            editor->editorWidget()->invokeAssist(TextEditor::Completion);
    });

    QShortcut *saveSC = new QShortcut(QKeySequence("Ctrl+S"), this);
    connect(saveSC, &QShortcut::activated, this, [this] { apply(); });

    m_editorScrollArea->show();
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
    m_preview->textDocument()->indenter()->setFileName(fileName);
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

    QString errorString;
    if (m_editor->document()->open(&errorString, m_config->filePath(), m_config->filePath())
        == Core::IDocument::OpenResult::Success) {
        QMetaObject::invokeMethod(languageClientPlugin(),
                                  "openDocument",
                                  Q_ARG(Core::IDocument *, m_editor->document()));
    }
}

void ClangFormatConfigWidget::apply()
{
    if (!m_editorWidget->isEnabled())
        return;

    clang::format::FormatStyle currentSettingsStyle;
    const bool success = parseConfigurationContent(m_editor->document()->contents().toStdString(),
                                                   currentSettingsStyle);

    auto saveSettings = [this] {
        QString errorString;
        m_editor->document()->save(&errorString, m_config->filePath());
    };

    if (success) {
        saveSettings();
        return;
    }

    QMessageBox mBox;
    mBox.setText(Tr::tr("The current settings are not valid. Are you sure you want to apply them?"));
    mBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    mBox.setDefaultButton(QMessageBox::No);
    int ret = mBox.exec();
    switch (ret) {
    case QMessageBox::Yes:
        saveSettings();
        break;
    case QMessageBox::No:
    default:
        break;
    }
}

TextEditor::CodeStyleEditorWidget *createClangFormatConfigWidget(
    TextEditor::ICodeStylePreferences *codeStyle,
    Project *project,
    QWidget *parent)
{
    return new ClangFormatConfigWidget(codeStyle, project, parent);
}

} // namespace ClangFormat
