/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GITSUBMITEDITORWIDGET_H
#define GITSUBMITEDITORWIDGET_H

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

struct GitSubmitEditorPanelInfo;
struct GitSubmitEditorPanelData;
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

private slots:
    void authorInformationChanged();
    void commitOnlySlot();
    void commitAndPushSlot();
    void commitAndPushToGerritSlot();

private:
    bool emailIsValid() const;
    void setPanelData(const GitSubmitEditorPanelData &data);
    void setPanelInfo(const GitSubmitEditorPanelInfo &info);

    PushAction m_pushAction;
    QWidget *m_gitSubmitPanel;
    LogChangeWidget *m_logChangeWidget;
    Ui::GitSubmitPanel m_gitSubmitPanelUi;
    QValidator *m_emailValidator;
    QString m_originalAuthor;
    QString m_originalEmail;
    bool m_hasUnmerged;
    bool m_isInitialized;
};

} // namespace Internal
} // namespace Git

#endif // GITSUBMITEDITORWIDGET_H
