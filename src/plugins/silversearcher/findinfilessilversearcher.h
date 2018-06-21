/****************************************************************************
**
** Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
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

#include <coreplugin/find/ifindsupport.h>
#include <texteditor/basefilefind.h>

#include <utils/fileutils.h>

#include <QPointer>

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

    Utils::FileName m_directorySetting;
    QPointer<QWidget> m_widget;
    QString m_path;
    QString m_toolName;
};

} // namespace SilverSearcher
