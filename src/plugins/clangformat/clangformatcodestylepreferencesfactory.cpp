// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatcodestylepreferencesfactory.h"

#include "clangformatconstants.h"
#include "clangformatfile.h"
#include "clangformatglobalconfigwidget.h"
#include "clangformatindenter.h"
#include "clangformatsettings.h"
#include "clangformattr.h"
#include "clangformatutils.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <cppeditor/cppcodestylesettings.h>
#include <cppeditor/cppcodestylesettingspage.h>
#include <cppeditor/cppcodestylesnippets.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cpphighlighter.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/editorconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <texteditor/codestyleeditor.h>
#include <texteditor/codestylepool.h>
#include <texteditor/codestyleselectorwidget.h>
#include <texteditor/displaysettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/icodestylepreferences.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/indenter.h>
#include <texteditor/snippets/snippeteditor.h>
#include <texteditor/snippets/snippetprovider.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/filepath.h>
#include <utils/fileutils.h>
#include <utils/guard.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/shutdownguard.h>

#include <QComboBox>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QString>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QWidget>

#include <memory>

using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace ClangFormat {

// ClangFormatSelectorWidget

class ClangFormatSelectorWidget final : public CodeStyleSelectorWidget
{
public:
    ClangFormatSelectorWidget(const FilePath &projectFile, QWidget *parent)
        : CodeStyleSelectorWidget{projectFile, parent}
    {}

    void onModeChanged(ClangFormatSettings::Mode newMode);
    void onUseCustomSettingsChanged(bool doUse);

private:
    void slotImportClicked() override;
    void slotExportClicked() override;

    void updateReadOnlyState();

    ClangFormatSettings::Mode m_currentMode = ClangFormatSettings::Mode::Indenting;
    bool m_useCustomSettings = false;
};

void ClangFormatSelectorWidget::onModeChanged(ClangFormatSettings::Mode newMode)
{
    m_currentMode = newMode;
    updateReadOnlyState();
}

void ClangFormatSelectorWidget::onUseCustomSettingsChanged(bool doUse)
{
    m_useCustomSettings = doUse;
    updateReadOnlyState();
}

void ClangFormatSelectorWidget::slotImportClicked()
{
    if (m_currentMode == ClangFormatSettings::Mode::Disable) {
        CodeStyleSelectorWidget::slotImportClicked();
        return;
    }

    const FilePath filePath = FileUtils::getOpenFilePath(
        Tr::tr("Import Code Format"), {}, Tr::tr("ClangFormat (*clang-format*);;All files (*)"));

    if (filePath.isEmpty())
        return;

    const QString name = QInputDialog::getText(
        this, Tr::tr("Import Code Style"), Tr::tr("Enter a name for the imported code style:"));
    if (name.isEmpty())
        return;

    CodeStylePool *codeStylePool = m_codeStyle->delegatingPool();
    ICodeStylePreferences *importedStyle = codeStylePool->createCodeStyle(name);
    const ClangFormatFile file{importedStyle, filePath};

    if (importedStyle != nullptr) {
        m_codeStyle->setCurrentDelegate(importedStyle);
        return;
    }

    QMessageBox::warning(
        this,
        Tr::tr("Import Code Style"),
        Tr::tr("Cannot import code style from \"%1\".").arg(filePath.toUserOutput()));
}

void ClangFormatSelectorWidget::slotExportClicked()
{
    if (m_currentMode == ClangFormatSettings::Mode::Disable) {
        CodeStyleSelectorWidget::slotExportClicked();
        return;
    }

    ICodeStylePreferences *currentPreferences = m_codeStyle->currentPreferences();
    const FilePath filePath = FileUtils::getSaveFilePath(
        Tr::tr("Export Code Format"),
        FileUtils::homePath() / ".clang-format",
        Tr::tr("ClangFormat (*clang-format*);;All files (*)"));

    if (filePath.isEmpty())
        return;

    const FilePath clangFormatFile = filePathToCurrentSettings(currentPreferences);
    clangFormatFile.copyFile(filePath);
}

void ClangFormatSelectorWidget::updateReadOnlyState()
{
    const bool isReadOnly = m_currentMode != ClangFormatSettings::Mode::Disable
                            && !m_useCustomSettings;
    setDisabled(isReadOnly);
}

// ClangFormatConfigWidget

template<typename... Args>
static void invokeMethodForLanguageClientManager(const char *method, Args &&...args)
{
    QObject *languageClientManager = ExtensionSystem::PluginManager::getObjectByName(
        "LanguageClientManager");
    if (!languageClientManager)
        return;
    QMetaObject::invokeMethod(languageClientManager, method, args...);
}

class ClangFormatConfigWidget final : public QWidget
{
public:
    ClangFormatConfigWidget(
        const Project *project,
        ICodeStylePreferences *codeStyle,
        QWidget *parent);
    ~ClangFormatConfigWidget() override;

    void apply();

    void onUseCustomSettingsChanged(bool doUse);

private:
    bool eventFilter(QObject *object, QEvent *event) override;

    FilePath globalPath();
    FilePath projectPath();
    void createStyleFileIfNeeded(bool isGlobal);
    void initPreview(ICodeStylePreferences *codeStyle);
    void initEditor();
    void reopenClangFormatDocument();
    void updatePreview();
    void slotCodeStyleChanged(ICodeStylePreferences *currentPreferences);
    void updateReadOnlyState();
    TextEditorWidget *editorWidget() const;

    const Project *m_project = nullptr;
    QScrollArea *m_editorScrollArea = nullptr;
    SnippetEditorWidget * const m_preview;
    std::unique_ptr<Core::IEditor> m_editor;
    std::unique_ptr<ClangFormatFile> m_config;
    Guard m_ignoreChanges;
    QLabel *m_clangVersion;
    InfoLabel *m_clangFileIsCorrectText;
    ClangFormatIndenter *m_indenter;

    bool m_useCustomSettings = false;
};

bool ClangFormatConfigWidget::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Wheel && qobject_cast<QComboBox *>(object)) {
        event->ignore();
        return true;
    }
    return QWidget::eventFilter(object, event);
}

ClangFormatConfigWidget::ClangFormatConfigWidget(
    const Project *project, ICodeStylePreferences *codeStyle, QWidget *parent)
    : QWidget(parent)
    , m_preview(new SnippetEditorWidget(this))
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

    connect(codeStyle, &ICodeStylePreferences::currentPreferencesChanged,
            this, &ClangFormatConfigWidget::slotCodeStyleChanged);

    connect(codeStyle, &ICodeStylePreferences::aboutToBeRemoved,
            this, &ClangFormatFile::removeClangFormatFileForStylePreferences);

    connect(codeStyle, &ICodeStylePreferences::aboutToBeCopied,
            this, &ClangFormatFile::copyClangFormatFileBasedOnStylePreferences);

    slotCodeStyleChanged(codeStyle->currentPreferences());

    reopenClangFormatDocument();
    updatePreview();
}

ClangFormatConfigWidget::~ClangFormatConfigWidget()
{
    auto doc = qobject_cast<TextDocument *>(m_editor->document());
    invokeMethodForLanguageClientManager("documentClosed", Q_ARG(Core::IDocument *, doc));
}

void ClangFormatConfigWidget::slotCodeStyleChanged(ICodeStylePreferences *codeStyle)
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
    m_preview->setReadOnly(isReadOnly);
    TextEditorWidget * const widget = editorWidget();
    QTC_ASSERT(widget, return);
    widget->setReadOnly(isReadOnly);
}

TextEditorWidget *ClangFormatConfigWidget::editorWidget() const
{
    return TextEditorWidget::fromEditor(m_editor.get());
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

    m_editorScrollArea->setWidget(m_editor->widget());
    m_editorScrollArea->setWidgetResizable(true);

    if (TextEditorWidget *editor = editorWidget())
        editor->setMinimapVisible(false);

    m_clangFileIsCorrectText = new InfoLabel("", InfoLabel::Ok);
    m_clangFileIsCorrectText->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_clangFileIsCorrectText->hide();

    m_clangVersion = new QLabel(Tr::tr("Current ClangFormat version: %1.").arg(LLVM_VERSION_STRING),
                                this);

    connect(m_editor->document(), &TextDocument::contentsChanged, this, [this] {
        clang::format::FormatStyle currentSettingsStyle{};
        const Result<> success
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
        m_clangFileIsCorrectText->setType(InfoLabel::Warning);
    });

    QShortcut *completionSC = new QShortcut(QKeySequence("Ctrl+Space"), this);
    connect(completionSC, &QShortcut::activated, this, [this] {
        if (auto editor = qobject_cast<BaseTextEditor *>(m_editor.get()))
            editor->editorWidget()->invokeAssist(Completion);
    });

    QShortcut *saveSC = new QShortcut(QKeySequence("Ctrl+S"), this);
    connect(saveSC, &QShortcut::activated, this, [this] { apply(); });
}

void ClangFormatConfigWidget::initPreview(ICodeStylePreferences *codeStyle)
{
    FilePath fileName = m_project ? m_project->projectFilePath().pathAppended("snippet.cpp")
                                  : Core::ICore::userResourcePath("snippet.cpp");

    DisplaySettingsData displaySettings = m_preview->displaySettings();
    displaySettings.m_visualizeWhitespace = true;
    m_preview->setDisplaySettings(displaySettings);
    m_preview->setPlainText(QLatin1String(CppEditor::Constants::DEFAULT_CODE_STYLE_SNIPPETS[0]));
    m_indenter = new ClangFormatIndenter(m_preview->document());
    m_indenter->setOverriddenPreferences(codeStyle);
    m_preview->textDocument()->setIndenter(m_indenter);
    m_preview->textDocument()->setFontSettings(globalFontSettings().data());
    m_preview->textDocument()->resetSyntaxHighlighter(
        [] { return new CppEditor::CppHighlighter(); });
    m_indenter->setFileName(fileName);
    m_preview->show();
}

static clang::format::FormatStyle constructStyle(const QByteArray &baseStyle = QByteArray())
{
    if (!baseStyle.isEmpty()) {
        llvm::Expected<clang::format::FormatStyle> style
            = clang::format::getStyle(baseStyle.toStdString(), "dummy.cpp", baseStyle.toStdString());
        if (style)
            return *style;

        handleAllErrors(style.takeError(), [](const llvm::ErrorInfoBase &) {
            // do nothing
        });
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
    TextEditorWidget * const widget = editorWidget();
    if (QTC_GUARD(widget) && widget->isReadOnly())
        return;
    m_editor->document()->save(m_config->filePath());
}

void ClangFormatConfigWidget::onUseCustomSettingsChanged(bool doUse)
{
    m_useCustomSettings = doUse;
    updateReadOnlyState();
}

// ClangFormatCodeStyleWidget

class ClangFormatCodeStyleWidget : public QWidget
{
public:
    ClangFormatCodeStyleWidget(
        const FilePath &projectFile, ICodeStylePreferences *codeStyle, QWidget *parent);

    void onModeChanged(ClangFormatSettings::Mode newMode);
    void onUseCustomSettingsChanged(bool doUse);

    void apply();
    void finish();

private:
    CppEditor::CppCodeStylePreferencesWidget *m_legacyIndenterSettings = nullptr;
    ClangFormatConfigWidget *m_clangFormatSettings = nullptr;
};

ClangFormatCodeStyleWidget::ClangFormatCodeStyleWidget(
    const FilePath &projectFile, ICodeStylePreferences *codeStyle, QWidget *parent)
    : QWidget{parent}
    , m_clangFormatSettings{new ClangFormatConfigWidget(
          ProjectManager::projectWithProjectFile(projectFile, !projectFile.isEmpty()), codeStyle, this)}
{
    auto layout = new QVBoxLayout{this};
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    if (QTC_GUARD(codeStyle)) {
        const auto cppPreferences = static_cast<CppEditor::CppCodeStylePreferences *>(codeStyle);
        m_legacyIndenterSettings = new CppEditor::CppCodeStylePreferencesWidget{this};
        m_legacyIndenterSettings->layout()->setContentsMargins(0, 0, 0, 0);
        m_legacyIndenterSettings->setCodeStyle(cppPreferences);
        layout->addWidget(m_legacyIndenterSettings);
    }

    layout->addWidget(m_clangFormatSettings);
}

void ClangFormatCodeStyleWidget::onModeChanged(ClangFormatSettings::Mode newMode)
{
    const bool isLegacyIndenterMode = newMode == ClangFormatSettings::Mode::Disable;
    if (m_legacyIndenterSettings)
        m_legacyIndenterSettings->setVisible(isLegacyIndenterMode);
    m_clangFormatSettings->setVisible(!isLegacyIndenterMode);
}

void ClangFormatCodeStyleWidget::onUseCustomSettingsChanged(bool doUse)
{
    m_clangFormatSettings->onUseCustomSettingsChanged(doUse);
}

void ClangFormatCodeStyleWidget::apply()
{
    if (m_legacyIndenterSettings)
        m_legacyIndenterSettings->apply();
    m_clangFormatSettings->apply();
}

void ClangFormatCodeStyleWidget::finish()
{
    if (m_legacyIndenterSettings)
        m_legacyIndenterSettings->apply();
    m_clangFormatSettings->apply();
}

// ClangFormatSettingsEditor

class ClangFormatSettingsEditor final : public CodeStyleEditor
{
public:
    ClangFormatSettingsEditor(ICodeStylePreferences *codeStyle, QWidget *parent);
    void apply() override;
    void finish() override;

private:
    ClangFormatGlobalConfigWidget *m_globalSettings = nullptr;
    ClangFormatCodeStyleWidget *m_editorWidget = nullptr;
};

ClangFormatSettingsEditor::ClangFormatSettingsEditor(
        ICodeStylePreferences *codeStyle, QWidget *parent)
    : CodeStyleEditor{parent}
{
    m_globalSettings = new ClangFormatGlobalConfigWidget(nullptr, codeStyle, this);
    addHeaderWidget(m_globalSettings);

    auto selector = new ClangFormatSelectorWidget{{}, this};
    selector->setCodeStyle(codeStyle);
    addSelector(selector);
    addInfoLabel();

    m_editorWidget = new ClangFormatCodeStyleWidget{{}, codeStyle, this};
    addEditorWidget(m_editorWidget);

    const ClangFormatSettings::Mode currentMode = m_globalSettings->mode();
    connect(m_globalSettings, &ClangFormatGlobalConfigWidget::modeChanged,
            selector, &ClangFormatSelectorWidget::onModeChanged);
    selector->onModeChanged(currentMode);
    connect(m_globalSettings, &ClangFormatGlobalConfigWidget::useCustomSettingsChanged,
            selector, &ClangFormatSelectorWidget::onUseCustomSettingsChanged);
    selector->onUseCustomSettingsChanged(m_globalSettings->useCustomSettings());

    connect(m_globalSettings, &ClangFormatGlobalConfigWidget::modeChanged,
            m_editorWidget, &ClangFormatCodeStyleWidget::onModeChanged);
    m_editorWidget->onModeChanged(currentMode);
    connect(m_globalSettings, &ClangFormatGlobalConfigWidget::useCustomSettingsChanged,
            m_editorWidget, &ClangFormatCodeStyleWidget::onUseCustomSettingsChanged);
    m_editorWidget->onUseCustomSettingsChanged(m_globalSettings->useCustomSettings());
}

void ClangFormatSettingsEditor::apply()
{
    m_editorWidget->apply();
    m_globalSettings->apply();
}

void ClangFormatSettingsEditor::finish()
{
    m_editorWidget->finish();
    m_globalSettings->finish();
}

// ClangFormatProjectEditor

class ClangFormatProjectEditor final : public CodeStyleEditor
{
public:
    ClangFormatProjectEditor(
        const FilePath &projectFile, ICodeStylePreferences *codeStyle, QWidget *parent);
    void apply() override;
    void finish() override;

private:
    ClangFormatGlobalConfigWidget *m_globalSettings = nullptr;
};

ClangFormatProjectEditor::ClangFormatProjectEditor(
        const FilePath &projectFile, ICodeStylePreferences *codeStyle, QWidget *parent)
    : CodeStyleEditor{parent}
{
    m_globalSettings = new ClangFormatGlobalConfigWidget(
        ProjectManager::projectWithProjectFile(projectFile, true), codeStyle, this);
    addHeaderWidget(m_globalSettings);

    auto selector = new ClangFormatSelectorWidget{projectFile, this};
    selector->setCodeStyle(codeStyle);
    addSelector(selector);
    addInfoLabel();

    auto preview = new SnippetEditorWidget;
    DisplaySettingsData displaySettings = preview->displaySettings();
    displaySettings.m_visualizeWhitespace = true;
    preview->setDisplaySettings(displaySettings);
    SnippetProvider::decorateEditor(preview, CppEditor::Constants::CPP_SNIPPETS_GROUP_ID);
    preview->setPlainText(
        QString::fromLatin1(CppEditor::Constants::DEFAULT_CODE_STYLE_SNIPPETS[0]));
    setupPreview(preview, new ClangFormatForwardingIndenter(preview->document()), projectFile, codeStyle);

    const ClangFormatSettings::Mode currentMode = m_globalSettings->mode();
    connect(m_globalSettings, &ClangFormatGlobalConfigWidget::modeChanged,
            selector, &ClangFormatSelectorWidget::onModeChanged);
    selector->onModeChanged(currentMode);
    connect(m_globalSettings, &ClangFormatGlobalConfigWidget::useCustomSettingsChanged,
            selector, &ClangFormatSelectorWidget::onUseCustomSettingsChanged);
    selector->onUseCustomSettingsChanged(m_globalSettings->useCustomSettings());
}

void ClangFormatProjectEditor::apply()
{
    m_globalSettings->apply();
}

void ClangFormatProjectEditor::finish()
{
    m_globalSettings->finish();
}

// ClangFormatCodeStylePreferencesFactory

class ClangFormatCodeStylePreferencesFactory final : public ICodeStylePreferencesFactory
{
public:
    ClangFormatCodeStylePreferencesFactory()
        : ICodeStylePreferencesFactory(CppEditor::Constants::CPP_SETTINGS_ID)
    {}

    CodeStyleEditor *createSettingsEditor(
            ICodeStylePreferences *codeStyle, QWidget *parent) const override
    {
        return new ClangFormatSettingsEditor{codeStyle, parent};
    }

    CodeStyleEditor *createProjectEditor(
            const FilePath &projectFile,
            ICodeStylePreferences *codeStyle,
            QWidget *parent) const override
    {
        return new ClangFormatProjectEditor{projectFile, codeStyle, parent};
    }

    QString displayName() override { return Tr::tr("C++"); }

    ICodeStylePreferences *createCodeStyle() const override
    {
        return new CppEditor::CppCodeStylePreferences;
    }

    Indenter *createIndenter(QTextDocument *doc) const override
    {
        return new ClangFormatForwardingIndenter(doc);
    }
};

void setupCodeStyleFactory()
{
    // Replace the default one by overwriting it with this here which has the same ID.
    static GuardedObject<ClangFormatCodeStylePreferencesFactory> theClangFormatStyleFactory;
}

} // namespace ClangFormat
