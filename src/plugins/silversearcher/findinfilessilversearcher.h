// Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/find/ifindsupport.h>
#include <texteditor/basefilefind.h>

#include <utils/fileutils.h>

#include <QPointer>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace SilverSearcher {

class FindInFilesSilverSearcher : public TextEditor::SearchEngine
{
    Q_OBJECT

public:
    explicit FindInFilesSilverSearcher(QObject *parent);
    ~FindInFilesSilverSearcher() override;

    // TextEditor::FileFindExtension
    QString title() const override;
    QString toolTip() const override;
    QWidget *widget() const override;
    QVariant parameters() const override;
    void readSettings(QSettings *settings) override;
    void writeSettings(QSettings *settings) const override;
    QFuture<Utils::FileSearchResultList> executeSearch(
            const TextEditor::FileFindParameters &parameters, TextEditor::BaseFileFind *) override;
    Core::IEditor *openEditor(const Core::SearchResultItem &item,
                              const TextEditor::FileFindParameters &parameters) override;

private:
    QPointer<Core::IFindSupport> m_currentFindSupport;

    Utils::FilePath m_directorySetting;
    QPointer<QWidget> m_widget;
    QPointer<QLineEdit> m_searchOptionsLineEdit;
    QString m_path;
    QString m_toolName;
};

} // namespace SilverSearcher
