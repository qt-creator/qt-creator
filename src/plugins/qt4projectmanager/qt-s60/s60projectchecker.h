/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef S60PROJECTCHECKER_H
#define S60PROJECTCHECKER_H

#include <projectexplorer/taskwindow.h>

namespace Qt4ProjectManager {
class QtVersion;
class Qt4Project;

namespace Internal {

class S60ProjectChecker
{
public:
    /// Check a .pro-file/Qt version combination on possible issues with
    /// its symbian setup.
    /// @return a list of tasks, ordered on severity (errors first, then
    ///         warnings and finally info items.
    static QList<ProjectExplorer::Task>
    reportIssues(const QString &proFile, const QtVersion *version);

    /// Check a project/Qt version combination on possible issues with
    /// its symbian setup.
    /// @return a list of tasks, ordered on severity (errors first, then
    ///         warnings and finally info items.
    static QList<ProjectExplorer::Task>
    reportIssues(const Qt4Project *project, const QtVersion *version);
};

} // namespace Internal
} // namespace Qt4ProjectExplorer

#endif // S60PROJECTCHECKER_H
