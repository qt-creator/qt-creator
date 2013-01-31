/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef COMPLETIONSETTINGSPAGE_H
#define COMPLETIONSETTINGSPAGE_H

#include "commentssettings.h"

#include <texteditor/completionsettings.h>
#include <texteditor/texteditoroptionspage.h>

namespace CppTools {
namespace Internal {

namespace Ui {
class CompletionSettingsPage;
}

// TODO: Move this class to the text editor plugin

class CompletionSettingsPage : public TextEditor::TextEditorOptionsPage
{
    Q_OBJECT

public:
    CompletionSettingsPage(QObject *parent);
    ~CompletionSettingsPage();

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();
    bool matches(const QString &) const;

    const CommentsSettings &commentsSettings() const;

signals:
    void commentsSettingsChanged(const CppTools::CommentsSettings &settings);

private:
    TextEditor::CaseSensitivity caseSensitivity() const;
    TextEditor::CompletionTrigger completionTrigger() const;

    bool requireCommentsSettingsUpdate() const;

    Ui::CompletionSettingsPage *m_page;
    QString m_searchKeywords;
    CommentsSettings m_commentsSettings;
};

} // namespace Internal
} // namespace CppTools

#endif // COMPLETIONSETTINGSPAGE_H
