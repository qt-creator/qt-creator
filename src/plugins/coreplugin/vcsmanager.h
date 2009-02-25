/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

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
