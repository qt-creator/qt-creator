/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
