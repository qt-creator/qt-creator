/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef COMPLETIONSETTINGSPAGE_H
#define COMPLETIONSETTINGSPAGE_H

#include "commentssettings.h"

#include <texteditor/completionsettings.h>
#include <texteditor/texteditoroptionspage.h>

#include <QPointer>

namespace CppTools {
namespace Internal {

namespace Ui { class CompletionSettingsPage; }

// TODO: Move this class to the text editor plugin

class CompletionSettingsPage : public TextEditor::TextEditorOptionsPage
{
    Q_OBJECT

public:
    CompletionSettingsPage(QObject *parent);
    ~CompletionSettingsPage();

    QWidget *widget();
    void apply();
    void finish();

private:
    TextEditor::CaseSensitivity caseSensitivity() const;
    TextEditor::CompletionTrigger completionTrigger() const;

    void onCompletionTriggerChanged();

    bool requireCommentsSettingsUpdate() const;

    Ui::CompletionSettingsPage *m_page;
    QPointer<QWidget> m_widget;
    CommentsSettings m_commentsSettings;
};

} // namespace Internal
} // namespace CppTools

#endif // COMPLETIONSETTINGSPAGE_H
