// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/cppcodestylesettingspage.h>

#include <memory>

namespace ProjectExplorer { class Project; }
namespace CppEditor { class CppCodeStyleSettings; }

namespace ClangFormat {

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

    Utils::FilePath globalPath();
    Utils::FilePath projectPath();
    void createStyleFileIfNeeded(bool isGlobal);
    void showOrHideWidgets();
    void initChecksAndPreview();
    void connectChecks();

    void fillTable();
    std::string readFile(const QString &path);
    void saveChanges(QObject *sender);

    void updatePreview();
    void slotCodeStyleChanged(TextEditor::ICodeStylePreferences *currentPreferences);

    class Private;
    Private * const d;
};

} // ClangFormat
