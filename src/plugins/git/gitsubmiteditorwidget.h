/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef GITSUBMITEDITORWIDGET_H
#define GITSUBMITEDITORWIDGET_H

#include "ui_gitsubmitpanel.h"

#include <utils/submiteditorwidget.h>

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

class GitSubmitEditorWidget : public Utils::SubmitEditorWidget
{
    Q_OBJECT

public:
    explicit GitSubmitEditorWidget(QWidget *parent = 0);


    GitSubmitEditorPanelData panelData() const;
    void setPanelData(const GitSubmitEditorPanelData &data);

    void setPanelInfo(const GitSubmitEditorPanelInfo &info);

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
};

} // namespace Internal
} // namespace Perforce

#endif // GITSUBMITEDITORWIDGET_H
