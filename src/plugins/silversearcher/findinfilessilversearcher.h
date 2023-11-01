// Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/basefilefind.h>

#include <utils/filepath.h>

#include <QPointer>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace Core { class IFindSupport; }

namespace SilverSearcher {

class FindInFilesSilverSearcher : public TextEditor::SearchEngine
{
    Q_OBJECT

public:
    explicit FindInFilesSilverSearcher(QObject *parent);

    // TextEditor::FileFindExtension
    QString title() const override;
    QString toolTip() const override;
    QWidget *widget() const override;
    void readSettings(Utils::QtcSettings *settings) override;
    void writeSettings(Utils::QtcSettings *settings) const override;
    TextEditor::SearchExecutor searchExecutor() const override;

private:
    QPointer<Core::IFindSupport> m_currentFindSupport;

    Utils::FilePath m_directorySetting;
    QPointer<QWidget> m_widget;
    QPointer<QLineEdit> m_searchOptionsLineEdit;
    QString m_path;
    QString m_toolName;
};

} // namespace SilverSearcher
