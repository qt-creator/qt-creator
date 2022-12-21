// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "commitdata.h"

#include <texteditor/syntaxhighlighter.h>
#include <vcsbase/submiteditorwidget.h>
#include <utils/filepath.h>

#include <QSyntaxHighlighter>

QT_BEGIN_NAMESPACE
class QValidator;
QT_END_NAMESPACE

namespace Git::Internal {

class GitSubmitPanel;
class GitSubmitEditorPanelInfo;
class GitSubmitEditorPanelData;
class GitSubmitHighlighter;
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
    void initialize(const Utils::FilePath &repository, const CommitData &data);
    void refreshLog(const Utils::FilePath &repository);

protected:
    bool canSubmit(QString *whyNot) const override;
    QString cleanupDescription(const QString &) const override;
    QString commitName() const override;

signals:
    void showRequested(const QString &commit);

private:
    void authorInformationChanged();
    void commitOnlySlot();
    void commitAndPushSlot();
    void commitAndPushToGerritSlot();

    bool emailIsValid() const;
    void setPanelData(const GitSubmitEditorPanelData &data);
    void setPanelInfo(const GitSubmitEditorPanelInfo &info);

    PushAction m_pushAction = NoPush;
    GitSubmitPanel *m_gitSubmitPanel;
    GitSubmitHighlighter *m_highlighter = nullptr;
    LogChangeWidget *m_logChangeWidget = nullptr;
    QValidator *m_emailValidator;
    QString m_originalAuthor;
    QString m_originalEmail;
    bool m_hasUnmerged = false;
    bool m_isInitialized = false;
};

} // Git::Internal
