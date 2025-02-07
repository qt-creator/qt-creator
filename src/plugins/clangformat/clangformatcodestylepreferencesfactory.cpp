// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatcodestylepreferencesfactory.h"

#include "clangformatconfigwidget.h"
#include "clangformatfile.h"
#include "clangformatglobalconfigwidget.h"
#include "clangformatindenter.h"
#include "clangformatsettings.h"
#include "clangformatutils.h"

#include <cppeditor/cppcodestylesettings.h>
#include <cppeditor/cppcodestylesettingspage.h>
#include <cppeditor/cppcodestylesnippets.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppeditortr.h>
#include <projectexplorer/project.h>
#include <texteditor/codestyleeditor.h>
#include <texteditor/codestylepool.h>
#include <texteditor/codestyleselectorwidget.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/indenter.h>
#include <texteditor/texteditorsettings.h>
#include <utils/filepath.h>
#include <utils/fileutils.h>

#include <QInputDialog>
#include <QMessageBox>
#include <QString>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QWidget>

using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace ClangFormat {

class ClangFormatSelectorWidget final : public CodeStyleSelectorWidget
{
public:
    ClangFormatSelectorWidget(QWidget *parent = nullptr);

    void onModeChanged(ClangFormatSettings::Mode newMode);
    void onUseCustomSettingsChanged(bool doUse);

private:
    void slotImportClicked() override;
    void slotExportClicked() override;

    void updateReadOnlyState();

    ClangFormatSettings::Mode m_currentMode = ClangFormatSettings::Mode::Indenting;
    bool m_useCustomSettings = false;
};

class ClangFormatCodeStyleEditor final : public CodeStyleEditor
{
public:
    static ClangFormatCodeStyleEditor *create(
        const ICodeStylePreferencesFactory *factory,
        Project *project,
        ICodeStylePreferences *codeStyle,
        QWidget *parent);

private:
    ClangFormatCodeStyleEditor(QWidget *parent);

    void init(
        const ICodeStylePreferencesFactory *factory,
        const ProjectWrapper &project,
        ICodeStylePreferences *codeStyle) override;
    void apply() override;
    void finish() override;

    CodeStyleEditorWidget *createEditorWidget(
        const void *project,
        ICodeStylePreferences *codeStyle,
        QWidget *parent = nullptr) const override;
    CodeStyleSelectorWidget *createCodeStyleSelectorWidget(
        ICodeStylePreferences *codeStyle, QWidget *parent = nullptr) const override;
    QString previewText() const override;
    QString snippetProviderGroupId() const override;

    ClangFormatGlobalConfigWidget *m_globalSettings = nullptr;
};

class ClangFormatCodeStyleEditorWidget : public CodeStyleEditorWidget
{
public:
    ClangFormatCodeStyleEditorWidget(
        const Project *project, ICodeStylePreferences *codeStyle, QWidget *parent);

    void onModeChanged(ClangFormatSettings::Mode newMode);
    void onUseCustomSettingsChanged(bool doUse);

    void apply() override;
    void finish() override;

private:
    CppEditor::CppCodeStylePreferencesWidget *m_legacyIndenterSettings = nullptr;
    ClangFormatConfigWidget *m_clangFormatSettings = nullptr;
};

class ClangFormatCodeStylePreferencesFactory final : public TextEditor::ICodeStylePreferencesFactory
{
public:
    static void setup(QObject *guard);

    TextEditor::CodeStyleEditorWidget *createCodeStyleEditor(
        const TextEditor::ProjectWrapper &project,
        TextEditor::ICodeStylePreferences *codeStyle,
        QWidget *parent = nullptr) const override;

private:
    ClangFormatCodeStylePreferencesFactory() = default;

    Utils::Id languageId() override;
    QString displayName() override;
    TextEditor::ICodeStylePreferences *createCodeStyle() const override;
    TextEditor::Indenter *createIndenter(QTextDocument *doc) const override;
};

ClangFormatSelectorWidget::ClangFormatSelectorWidget(QWidget *parent)
    : CodeStyleSelectorWidget{parent}
{}

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
        CppEditor::Tr::tr("Import Code Format"),
        {},
        CppEditor::Tr::tr("ClangFormat (*clang-format*);;All files (*)"));

    if (filePath.isEmpty())
        return;

    const QString name = QInputDialog::getText(
        this,
        CppEditor::Tr::tr("Import Code Style"),
        CppEditor::Tr::tr("Enter a name for the imported code style:"));
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
        CppEditor::Tr::tr("Import Code Style"),
        CppEditor::Tr::tr("Cannot import code style from \"%1\".").arg(filePath.toUserOutput()));
}

void ClangFormatSelectorWidget::slotExportClicked()
{
    if (m_currentMode == ClangFormatSettings::Mode::Disable) {
        CodeStyleSelectorWidget::slotExportClicked();
        return;
    }

    ICodeStylePreferences *currentPreferences = m_codeStyle->currentPreferences();
    const FilePath filePath = FileUtils::getSaveFilePath(
        CppEditor::Tr::tr("Export Code Format"),
        FileUtils::homePath() / ".clang-format",
        CppEditor::Tr::tr("ClangFormat (*clang-format*);;All files (*)"));

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

ClangFormatCodeStyleEditorWidget::ClangFormatCodeStyleEditorWidget(
    const Project *project, ICodeStylePreferences *codeStyle, QWidget *parent)
    : CodeStyleEditorWidget{parent}
    , m_clangFormatSettings{new ClangFormatConfigWidget(project, codeStyle, this)}
{
    auto layout = new QVBoxLayout{this};
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    auto cppPreferences = dynamic_cast<CppEditor::CppCodeStylePreferences *>(codeStyle);
    if (cppPreferences != nullptr) {
        m_legacyIndenterSettings = new CppEditor::CppCodeStylePreferencesWidget{this};
        m_legacyIndenterSettings->layout()->setContentsMargins(0, 0, 0, 0);
        m_legacyIndenterSettings->setCodeStyle(cppPreferences);
    }

    layout->addWidget(m_legacyIndenterSettings);
    layout->addWidget(m_clangFormatSettings);
}

void ClangFormatCodeStyleEditorWidget::onModeChanged(ClangFormatSettings::Mode newMode)
{
    const bool isLegacyIndenterMode = newMode == ClangFormatSettings::Mode::Disable;
    m_legacyIndenterSettings->setVisible(isLegacyIndenterMode);
    m_clangFormatSettings->setVisible(!isLegacyIndenterMode);
}

void ClangFormatCodeStyleEditorWidget::onUseCustomSettingsChanged(bool doUse)
{
    m_clangFormatSettings->onUseCustomSettingsChanged(doUse);
}

void ClangFormatCodeStyleEditorWidget::apply()
{
    m_legacyIndenterSettings->apply();
    m_clangFormatSettings->apply();
}

void ClangFormatCodeStyleEditorWidget::finish()
{
    m_legacyIndenterSettings->apply();
    m_clangFormatSettings->apply();
}

ClangFormatCodeStyleEditor *ClangFormatCodeStyleEditor::create(
    const ICodeStylePreferencesFactory *factory,
    Project *project,
    ICodeStylePreferences *codeStyle,
    QWidget *parent)
{
    auto editor = new ClangFormatCodeStyleEditor{parent};
    editor->init(factory, wrapProject(project), codeStyle);
    return editor;
}

ClangFormatCodeStyleEditor::ClangFormatCodeStyleEditor(QWidget *parent)
    : CodeStyleEditor{parent}
{}

void ClangFormatCodeStyleEditor::init(
    const ICodeStylePreferencesFactory *factory, const ProjectWrapper &project, ICodeStylePreferences *codeStyle)
{
    m_globalSettings
        = new ClangFormatGlobalConfigWidget{unwrapProject(project), codeStyle, this};
    m_layout->addWidget(m_globalSettings);
    CodeStyleEditor::init(factory, project, codeStyle);

    const ClangFormatSettings::Mode currentMode = m_globalSettings->mode();

    auto selector = static_cast<ClangFormatSelectorWidget *>(m_selector);
    connect(
        m_globalSettings,
        &ClangFormatGlobalConfigWidget::modeChanged,
        selector,
        &ClangFormatSelectorWidget::onModeChanged);
    selector->onModeChanged(currentMode);

    auto editorWidget = static_cast<ClangFormatCodeStyleEditorWidget *>(m_editor);
    connect(
        m_globalSettings,
        &ClangFormatGlobalConfigWidget::modeChanged,
        editorWidget,
        &ClangFormatCodeStyleEditorWidget::onModeChanged);
    editorWidget->onModeChanged(currentMode);

    connect(
        m_globalSettings,
        &ClangFormatGlobalConfigWidget::useCustomSettingsChanged,
        editorWidget,
        &ClangFormatCodeStyleEditorWidget::onUseCustomSettingsChanged);
    editorWidget->onUseCustomSettingsChanged(m_globalSettings->useCustomSettings());
    connect(
        m_globalSettings,
        &ClangFormatGlobalConfigWidget::useCustomSettingsChanged,
        selector,
        &ClangFormatSelectorWidget::onUseCustomSettingsChanged);
    selector->onUseCustomSettingsChanged(m_globalSettings->useCustomSettings());
}

void ClangFormatCodeStyleEditor::apply()
{
    CodeStyleEditor::apply();

    m_globalSettings->apply();
}

void ClangFormatCodeStyleEditor::finish()
{
    CodeStyleEditor::finish();

    m_globalSettings->finish();
}

CodeStyleEditorWidget *ClangFormatCodeStyleEditor::createEditorWidget(
    const void *project, ICodeStylePreferences *codeStyle, QWidget *parent) const
{
    return new ClangFormatCodeStyleEditorWidget{
            reinterpret_cast<const Project *>(project), codeStyle, parent};
}

CodeStyleSelectorWidget *ClangFormatCodeStyleEditor::createCodeStyleSelectorWidget(
    ICodeStylePreferences *codeStyle, QWidget *parent) const
{
    auto selector = new ClangFormatSelectorWidget{parent};
    selector->setCodeStyle(codeStyle);
    return selector;
}

QString ClangFormatCodeStyleEditor::previewText() const
{
    return QString::fromLatin1(CppEditor::Constants::DEFAULT_CODE_STYLE_SNIPPETS[0]);
}

QString ClangFormatCodeStyleEditor::snippetProviderGroupId() const
{
    return CppEditor::Constants::CPP_SNIPPETS_GROUP_ID;
}

void ClangFormatCodeStylePreferencesFactory::setup(QObject *guard)
{
    static ClangFormatCodeStylePreferencesFactory theClangFormatStyleFactory;

    // Replace the default one.
    const Id factoryId = theClangFormatStyleFactory.languageId();
    TextEditorSettings::unregisterCodeStyleFactory(factoryId);
    TextEditorSettings::registerCodeStyleFactory(&theClangFormatStyleFactory);
    QObject::connect(guard, &QObject::destroyed, [=] {
        TextEditorSettings::unregisterCodeStyleFactory(factoryId);
    });
}

CodeStyleEditorWidget *ClangFormatCodeStylePreferencesFactory::createCodeStyleEditor(
    const ProjectWrapper &project, ICodeStylePreferences *codeStyle, QWidget *parent) const
{
    return ClangFormatCodeStyleEditor::create(this, unwrapProject(project), codeStyle, parent);
}

Id ClangFormatCodeStylePreferencesFactory::languageId()
{
    return CppEditor::Constants::CPP_SETTINGS_ID;
}

QString ClangFormatCodeStylePreferencesFactory::displayName()
{
    return CppEditor::Tr::tr(CppEditor::Constants::CPP_SETTINGS_NAME);
}

ICodeStylePreferences *ClangFormatCodeStylePreferencesFactory::createCodeStyle() const
{
    return new CppEditor::CppCodeStylePreferences;
}

Indenter *ClangFormatCodeStylePreferencesFactory::createIndenter(QTextDocument *doc) const
{
    return new ClangFormatForwardingIndenter(doc);
}

void setupCodeStyleFactory(QObject *guard)
{
    ClangFormatCodeStylePreferencesFactory::setup(guard);
}

} // namespace ClangFormat
