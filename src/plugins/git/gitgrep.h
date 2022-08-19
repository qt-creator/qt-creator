// Copyright (C) 2016 Orgad Shaneh <orgads@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/basefilefind.h>

#include <QCoreApplication>

QT_FORWARD_DECLARE_CLASS(QCheckBox);

namespace Utils { class FancyLineEdit; }

namespace Git {
namespace Internal {

class GitClient;

class GitGrep : public TextEditor::SearchEngine
{
    Q_DECLARE_TR_FUNCTIONS(GitGrep)

public:
    explicit GitGrep(GitClient *client);
    ~GitGrep() override;
    QString title() const override;
    QString toolTip() const override;
    QWidget *widget() const override;
    QVariant parameters() const override;
    void readSettings(QSettings *settings) override;
    void writeSettings(QSettings *settings) const override;
    QFuture<Utils::FileSearchResultList> executeSearch(
            const TextEditor::FileFindParameters &parameters,
            TextEditor::BaseFileFind *baseFileFind) override;
    Core::IEditor *openEditor(const Core::SearchResultItem &item,
                              const TextEditor::FileFindParameters &parameters) override;

private:
    GitClient *m_client;
    QWidget *m_widget;
    Utils::FancyLineEdit *m_treeLineEdit;
    QCheckBox *m_recurseSubmodules = nullptr;
};

} // namespace Internal
} // namespace Git
