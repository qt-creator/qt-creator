// Copyright (C) 2016 Orgad Shaneh <orgads@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/basefilefind.h>

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace Utils {
class FancyLineEdit;
class SearchResultItem;
}

namespace Git::Internal {

class GitClient;

class GitGrep : public TextEditor::SearchEngine
{
public:
    explicit GitGrep(GitClient *client);
    ~GitGrep() override;

    QString title() const override;
    QString toolTip() const override;
    QWidget *widget() const override;
    QVariant parameters() const override;
    void readSettings(QSettings *settings) override;
    void writeSettings(QSettings *settings) const override;
    QFuture<Utils::SearchResultItems> executeSearch(
            const TextEditor::FileFindParameters &parameters,
            TextEditor::BaseFileFind *baseFileFind) override;
    Core::IEditor *openEditor(const Utils::SearchResultItem &item,
                              const TextEditor::FileFindParameters &parameters) override;

private:
    GitClient *m_client;
    QWidget *m_widget;
    Utils::FancyLineEdit *m_treeLineEdit;
    QCheckBox *m_recurseSubmodules = nullptr;
};

} // Git::Internal
