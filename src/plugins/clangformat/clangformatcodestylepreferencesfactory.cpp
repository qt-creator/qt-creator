// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatcodestylepreferencesfactory.h"

#include "clangformatconfigwidget.h"
#include "clangformatfile.h"
#include "clangformatglobalconfigwidget.h"
#include "clangformatindenter.h"
#include "clangformatsettings.h"
#include "clangformattr.h"
#include "clangformatutils.h"

#include <cppeditor/cppcodestylesettings.h>
#include <cppeditor/cppcodestylesettingspage.h>
#include <cppeditor/cppcodestylesnippets.h>
#include <cppeditor/cppeditorconstants.h>
#include <projectexplorer/projectmanager.h>
#include <texteditor/codestyleeditor.h>
#include <texteditor/codestylepool.h>
#include <texteditor/codestyleselectorwidget.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/indenter.h>
#include <utils/filepath.h>
#include <utils/fileutils.h>
#include <utils/shutdownguard.h>

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
    ClangFormatSelectorWidget(const FilePath &projectFile, QWidget *parent = nullptr);

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
    ClangFormatCodeStyleEditor(
        const ICodeStylePreferencesFactory *factory,
        const FilePath &projectFile,
        ICodeStylePreferences *codeStyle,
        QWidget *parent);

    void apply() override;
    void finish() override;

    ClangFormatGlobalConfigWidget *m_globalSettings = nullptr;
};

class ClangFormatCodeStyleWidget : public CodeStyleWidget
{
public:
    ClangFormatCodeStyleWidget(
        const FilePath &projectFile, ICodeStylePreferences *codeStyle, QWidget *parent);

    void onModeChanged(ClangFormatSettings::Mode newMode);
    void onUseCustomSettingsChanged(bool doUse);

    void apply() override;
    void finish() override;

private:
    CppEditor::CppCodeStylePreferencesWidget *m_legacyIndenterSettings = nullptr;
    ClangFormatConfigWidget *m_clangFormatSettings = nullptr;
};

ClangFormatSelectorWidget::ClangFormatSelectorWidget(const FilePath &projectFile, QWidget *parent)
    : CodeStyleSelectorWidget{projectFile, parent}
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

ClangFormatCodeStyleWidget::ClangFormatCodeStyleWidget(
    const FilePath &projectFile, ICodeStylePreferences *codeStyle, QWidget *parent)
    : CodeStyleWidget{parent}
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

ClangFormatCodeStyleEditor::ClangFormatCodeStyleEditor(
        const ICodeStylePreferencesFactory *factory,
        const FilePath &projectFile,
        ICodeStylePreferences *codeStyle,
        QWidget *parent)
    : CodeStyleEditor{parent}
{
    m_globalSettings = new ClangFormatGlobalConfigWidget(
        ProjectManager::projectWithProjectFile(projectFile, !projectFile.isEmpty()), codeStyle, this);
    m_layout->addWidget(m_globalSettings);

    auto selector = new ClangFormatSelectorWidget{projectFile, this};
    selector->setCodeStyle(codeStyle);
    addSelector(selector);
    addInfoLabel();

    ClangFormatCodeStyleWidget *editorWidget = nullptr;
    if (projectFile.isEmpty()) {
        editorWidget = new ClangFormatCodeStyleWidget{projectFile, codeStyle, this};
        addEditorWidget(editorWidget);
    } else {
        setupPreview(factory, projectFile, codeStyle,
                     QString::fromLatin1(CppEditor::Constants::DEFAULT_CODE_STYLE_SNIPPETS[0]),
                     CppEditor::Constants::CPP_SNIPPETS_GROUP_ID);
    }

    const ClangFormatSettings::Mode currentMode = m_globalSettings->mode();
    connect(m_globalSettings, &ClangFormatGlobalConfigWidget::modeChanged,
            selector, &ClangFormatSelectorWidget::onModeChanged);
    selector->onModeChanged(currentMode);
    connect(m_globalSettings, &ClangFormatGlobalConfigWidget::useCustomSettingsChanged,
            selector, &ClangFormatSelectorWidget::onUseCustomSettingsChanged);
    selector->onUseCustomSettingsChanged(m_globalSettings->useCustomSettings());

    if (editorWidget) {
        connect(m_globalSettings, &ClangFormatGlobalConfigWidget::modeChanged,
                editorWidget, &ClangFormatCodeStyleWidget::onModeChanged);
        editorWidget->onModeChanged(currentMode);
        connect(m_globalSettings, &ClangFormatGlobalConfigWidget::useCustomSettingsChanged,
                editorWidget, &ClangFormatCodeStyleWidget::onUseCustomSettingsChanged);
        editorWidget->onUseCustomSettingsChanged(m_globalSettings->useCustomSettings());
    }
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

// ClangFormatCodeStylePreferencesFactory

class ClangFormatCodeStylePreferencesFactory final : public ICodeStylePreferencesFactory
{
public:
    ClangFormatCodeStylePreferencesFactory()
        : ICodeStylePreferencesFactory(CppEditor::Constants::CPP_SETTINGS_ID)
    {}

    CodeStyleEditor *createCodeStyleEditor(
            const FilePath &projectFile,
            ICodeStylePreferences *codeStyle,
            QWidget *parent = nullptr) const override
    {
        return new ClangFormatCodeStyleEditor{this, projectFile, codeStyle, parent};
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
