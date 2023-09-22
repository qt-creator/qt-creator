// Copyright (C) 2016 Orgad Shaneh <orgads@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/basefilefind.h>

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace Utils { class FancyLineEdit; }

namespace Git::Internal {

class GitGrepParameters;

class GitGrep : public TextEditor::SearchEngine
{
public:
    GitGrep();
    ~GitGrep() override;

    QString title() const override;
    QString toolTip() const override;
    QWidget *widget() const override;
    void readSettings(Utils::QtcSettings *settings) override;
    void writeSettings(Utils::QtcSettings *settings) const override;
    TextEditor::SearchExecutor searchExecutor() const override;
    TextEditor::EditorOpener editorOpener() const override;

private:
    GitGrepParameters gitParameters() const;
    QWidget *m_widget;
    Utils::FancyLineEdit *m_treeLineEdit;
    QCheckBox *m_recurseSubmodules = nullptr;
};

} // Git::Internal
