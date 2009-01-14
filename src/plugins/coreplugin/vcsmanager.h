/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef VCSMANAGER_H
#define VCSMANAGER_H

#include "core_global.h"

#include <QtCore/QString>

namespace Core {

struct VCSManagerPrivate;
class IVersionControl;

// The VCSManager has only one notable function:
// findVersionControlFor(), which returns the IVersionControl * for a given
// filename. Note that the VCSManager assumes that if a IVersionControl *
// manages a directory, then it also manages all the files and all the
// subdirectories.
//
// It works by asking all IVersionControl * if they manage the file, and ask
// for the topmost directory it manages. This information is cached and
// VCSManager thus knows pretty fast which IVersionControl * is responsible.

class CORE_EXPORT VCSManager
{
    Q_DISABLE_COPY(VCSManager)
public:
    VCSManager();
    virtual ~VCSManager();

    IVersionControl *findVersionControlForDirectory(const QString &directory);

    // Enable the VCS managing a certain directory only. This should
    // be used by project manager classes.
    void setVCSEnabled(const QString &directory);
    // Enable all VCS.
    void setAllVCSEnabled();

    // Shows a confirmation dialog, whether the file should also be deleted
    // from revision control Calls sccDelete on the file. Returns false
    // if a failure occurs
    bool showDeleteDialog(const QString &fileName);

private:
    VCSManagerPrivate *m_d;
};

} // namespace Core

#endif // VCSMANAGER_H
