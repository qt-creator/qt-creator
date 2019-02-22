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

#include <texteditor/icodestylepreferencesfactory.h>

#include <memory>

namespace ProjectExplorer {
class Project;
}
namespace TextEditor {
class SnippetEditorWidget;
}

namespace ClangFormat {

namespace Ui {
class ClangFormatConfigWidget;
}

class ClangFormatConfigWidget : public TextEditor::CodeStyleEditorWidget
{
    Q_OBJECT

public:
    explicit ClangFormatConfigWidget(ProjectExplorer::Project *project = nullptr,
                                     QWidget *parent = nullptr);
    ~ClangFormatConfigWidget() override;
    void apply() override;

private:
    void initialize();
    void fillTable();

    void hideGlobalCheckboxes();
    void showGlobalCheckboxes();

    void updatePreview();

    ProjectExplorer::Project *m_project;
    TextEditor::SnippetEditorWidget *m_preview;
    std::unique_ptr<Ui::ClangFormatConfigWidget> m_ui;
};

} // namespace ClangFormat
