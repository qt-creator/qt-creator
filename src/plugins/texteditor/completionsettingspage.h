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

#pragma once

#include "commentssettings.h"
#include "completionsettings.h"
#include "texteditoroptionspage.h"


#include <QPointer>

namespace TextEditor {
namespace Internal {

namespace Ui { class CompletionSettingsPage; }

class CompletionSettingsPage : public TextEditorOptionsPage
{
    Q_OBJECT

public:
    CompletionSettingsPage(QObject *parent);
    ~CompletionSettingsPage();

    QWidget *widget();
    void apply();
    void finish();

    const CompletionSettings & completionSettings();
    const CommentsSettings & commentsSettings();

signals:
    void completionSettingsChanged(const CompletionSettings &);
    void commentsSettingsChanged(const CommentsSettings &);

private:
    CaseSensitivity caseSensitivity() const;
    CompletionTrigger completionTrigger() const;
    void settingsFromUi(CompletionSettings &completion, CommentsSettings &comment) const;

    void onCompletionTriggerChanged();

    Ui::CompletionSettingsPage *m_page;
    QPointer<QWidget> m_widget;
    CommentsSettings m_commentsSettings;
    CompletionSettings m_completionSettings;
};

} // namespace Internal
} // namespace TextEditor
