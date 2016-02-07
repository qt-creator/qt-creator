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

#ifndef GITGREP_H
#define GITGREP_H

#include <texteditor/basefilefind.h>

#include <utils/fileutils.h>

QT_FORWARD_DECLARE_CLASS(QCheckBox)

namespace Git {
namespace Internal {

class GitGrep : public TextEditor::FileFindExtension
{
    Q_DECLARE_TR_FUNCTIONS(GitGrep)

public:
    GitGrep();
    ~GitGrep() override;
    QString title() const override;
    QWidget *widget() const override;
    bool isEnabled() const override;
    bool isEnabled(const TextEditor::FileFindParameters &parameters) const override;
    QVariant parameters() const override;
    void readSettings(QSettings *settings) override;
    void writeSettings(QSettings *settings) const override;
    QFuture<Utils::FileSearchResultList> executeSearch(
            const TextEditor::FileFindParameters &parameters) override;

private:
    QCheckBox *m_widget;
};

} // namespace Internal
} // namespace Git

#endif // FINDINFILES_H
