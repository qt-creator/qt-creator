/****************************************************************************
**
** Copyright (C) 2016 Orgad Shaneh <orgads@gmail.com>.
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

#include <texteditor/basefilefind.h>

#include <QCoreApplication>

namespace Utils { class FancyLineEdit; }

namespace Git {
namespace Internal {

class GitGrep : public TextEditor::SearchEngine
{
    Q_DECLARE_TR_FUNCTIONS(GitGrep)

public:
    GitGrep();
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
    QWidget *m_widget;
    Utils::FancyLineEdit *m_treeLineEdit;
};

} // namespace Internal
} // namespace Git
