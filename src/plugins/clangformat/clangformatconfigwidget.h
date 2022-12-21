// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/cppcodestylesettingspage.h>

#include <clang/Format/Format.h>

#include <utils/guard.h>

#include <QScrollArea>

#include <memory>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLabel;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }
namespace TextEditor { class SnippetEditorWidget; }
namespace CppEditor { class CppCodeStyleSettings; }

namespace ClangFormat {

namespace Ui { class ClangFormatChecksWidget; }

class ClangFormatFile;
class ClangFormatChecks;

class ClangFormatConfigWidget : public CppEditor::CppCodeStyleWidget
{
    Q_OBJECT

public:
    explicit ClangFormatConfigWidget(TextEditor::ICodeStylePreferences *codeStyle,
                                     ProjectExplorer::Project *project = nullptr,
                                     QWidget *parent = nullptr);
    ~ClangFormatConfigWidget() override;
    void apply() override;
    void finish() override;
    void setCodeStyleSettings(const CppEditor::CppCodeStyleSettings &settings) override;
    void setTabSettings(const TextEditor::TabSettings &settings) override;
    void synchronize() override;

private:
    bool eventFilter(QObject *object, QEvent *event) override;

    void showOrHideWidgets();
    void initChecksAndPreview();
    void connectChecks();

    void fillTable();
    void saveChanges(QObject *sender);

    void updatePreview();
    void slotCodeStyleChanged(TextEditor::ICodeStylePreferences *currentPreferences);

    ProjectExplorer::Project *m_project;
    QWidget *m_checksWidget;
    QScrollArea *m_checksScrollArea;
    TextEditor::SnippetEditorWidget *m_preview;
    std::unique_ptr<ClangFormatFile> m_config;
    clang::format::FormatStyle m_style;

    Utils::Guard m_ignoreChanges;

    QLabel *m_fallbackConfig;
};

} // ClangFormat
