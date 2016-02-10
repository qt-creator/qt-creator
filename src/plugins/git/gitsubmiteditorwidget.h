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

#include "ui_gitsubmitpanel.h"
#include "gitsettings.h"
#include "commitdata.h"

#include <texteditor/syntaxhighlighter.h>
#include <vcsbase/submiteditorwidget.h>

#include <QRegExp>
#include <QSyntaxHighlighter>

QT_BEGIN_NAMESPACE
class QValidator;
QT_END_NAMESPACE

namespace Git {
namespace Internal {

class GitSubmitEditorPanelInfo;
class GitSubmitEditorPanelData;
class LogChangeWidget;

/* Submit editor widget with 2 additional panes:
 * 1) Info with branch, description, etc
 * 2) Data, with author and email to edit.
 * The file contents is the submit message.
 * The previously added files will be added 'checked' to the file list, the
 * remaining un-added and untracked files will be added 'unchecked' for the
 * user to click. */

class GitSubmitEditorWidget : public VcsBase::SubmitEditorWidget
{
    Q_OBJECT

public:
    GitSubmitEditorWidget();

    GitSubmitEditorPanelData panelData() const;
    QString amendSHA1() const;
    void setHasUnmerged(bool e);
    void initialize(CommitType commitType,
                    const QString &repository,
                    const GitSubmitEditorPanelData &data,
                    const GitSubmitEditorPanelInfo &info,
                    bool enablePush);
    void refreshLog(const QString &repository);

protected:
    bool canSubmit() const override;
    QString cleanupDescription(const QString &) const override;
    QString commitName() const override;

signals:
    void show(const QString &commit);

private:
    void authorInformationChanged();
    void commitOnlySlot();
    void commitAndPushSlot();
    void commitAndPushToGerritSlot();

    bool emailIsValid() const;
    void setPanelData(const GitSubmitEditorPanelData &data);
    void setPanelInfo(const GitSubmitEditorPanelInfo &info);

    PushAction m_pushAction = NoPush;
    QWidget *m_gitSubmitPanel;
    LogChangeWidget *m_logChangeWidget = nullptr;
    Ui::GitSubmitPanel m_gitSubmitPanelUi;
    QValidator *m_emailValidator;
    QString m_originalAuthor;
    QString m_originalEmail;
    bool m_hasUnmerged = false;
    bool m_isInitialized = false;
};

} // namespace Internal
} // namespace Git
