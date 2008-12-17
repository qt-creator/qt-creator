/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef GITSUBMITEDITORWIDGET_H
#define GITSUBMITEDITORWIDGET_H

#include "ui_gitsubmitpanel.h"
#include <utils/submiteditorwidget.h>

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

class GitSubmitEditorWidget : public Core::Utils::SubmitEditorWidget
{

public:
    explicit GitSubmitEditorWidget(QWidget *parent = 0);


    GitSubmitEditorPanelData panelData() const;
    void setPanelData(const GitSubmitEditorPanelData &data);

    void setPanelInfo(const GitSubmitEditorPanelInfo &info);

private:
    QWidget *m_gitSubmitPanel;
    Ui::GitSubmitPanel m_gitSubmitPanelUi;
};

} // namespace Internal
} // namespace Perforce

#endif // GITSUBMITEDITORWIDGET_H
