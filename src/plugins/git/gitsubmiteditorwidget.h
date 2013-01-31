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

#ifndef GITSUBMITEDITORWIDGET_H
#define GITSUBMITEDITORWIDGET_H

#include "ui_gitsubmitpanel.h"

#include <vcsbase/submiteditorwidget.h>

QT_BEGIN_NAMESPACE
class QValidator;
QT_END_NAMESPACE

namespace Git {
namespace Internal {

struct GitSubmitEditorPanelInfo;
struct GitSubmitEditorPanelData;

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
    explicit GitSubmitEditorWidget(QWidget *parent = 0);

    GitSubmitEditorPanelData panelData() const;
    void setPanelData(const GitSubmitEditorPanelData &data);
    void setPanelInfo(const GitSubmitEditorPanelInfo &info);
    void setHasUnmerged(bool e);

protected:
    bool canSubmit() const;
    QString cleanupDescription(const QString &) const;

private slots:
    void authorInformationChanged();

private:
    bool emailIsValid() const;

    QWidget *m_gitSubmitPanel;
    Ui::GitSubmitPanel m_gitSubmitPanelUi;
    QValidator *m_emailValidator;
    bool m_hasUnmerged;
};

} // namespace Internal
} // namespace Perforce

#endif // GITSUBMITEDITORWIDGET_H
