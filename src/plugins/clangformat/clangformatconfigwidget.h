/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#pragma once

#include <cppeditor/cppcodestylesettingspage.h>

#include <QScrollArea>

#include <memory>

namespace ProjectExplorer { class Project; }
namespace TextEditor { class SnippetEditorWidget; }
namespace CppEditor { class CppCodeStyleSettings; }

namespace ClangFormat {

namespace Ui {
class ClangFormatConfigWidget;
class ClangFormatChecksWidget;
}
class ClangFormatFile;

class ClangFormatConfigWidget : public CppEditor::CppCodeStyleWidget
{
    Q_OBJECT

public:
    explicit ClangFormatConfigWidget(ProjectExplorer::Project *project = nullptr,
                                     QWidget *parent = nullptr);
    ~ClangFormatConfigWidget() override;
    void apply() override;
    void setCodeStyleSettings(const CppEditor::CppCodeStyleSettings &settings) override;
    void setTabSettings(const TextEditor::TabSettings &settings) override;
    void synchronize() override;

private:
    void onTableChanged();

    bool eventFilter(QObject *object, QEvent *event) override;

    void showOrHideWidgets();
    void initChecksAndPreview();
    void connectChecks();

    void fillTable();
    void saveChanges(QObject *sender);

    void showCombobox();
    void updatePreview();

    ProjectExplorer::Project *m_project;
    QWidget *m_checksWidget;
    QScrollArea *m_checksScrollArea;
    TextEditor::SnippetEditorWidget *m_preview;
    std::unique_ptr<ClangFormatFile> m_config;
    std::unique_ptr<Ui::ClangFormatChecksWidget> m_checks;
    std::unique_ptr<Ui::ClangFormatConfigWidget> m_ui;
    bool m_disableTableUpdate = false;
};

} // namespace ClangFormat
