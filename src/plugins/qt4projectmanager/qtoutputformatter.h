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

#ifndef QTOUTPUTFORMATTER_H
#define QTOUTPUTFORMATTER_H

#include "qt4projectmanager_global.h"

#include <projectexplorer/outputformatter.h>
#include <utils/fileinprojectfinder.h>

#include <QtCore/QRegExp>
#include <QtCore/QWeakPointer>

QT_FORWARD_DECLARE_CLASS(QTextCursor)

namespace ProjectExplorer {
class Project;
} // namespace ProjectExplorer

namespace Qt4ProjectManager
{

struct LinkResult
{
    int start;
    int end;
    QString href;
};

class QT4PROJECTMANAGER_EXPORT QtOutputFormatter: public ProjectExplorer::OutputFormatter
{
public:
    QtOutputFormatter(ProjectExplorer::Project *project);

    virtual void appendApplicationOutput(const QString &text, bool onStdErr);

    virtual void handleLink(const QString &href);

private:
    LinkResult matchLine(const QString &line) const;
    void appendLine(QTextCursor & cursor, LinkResult lr, const QString &line, bool onStdError);

    QRegExp m_qmlError;
    QRegExp m_qtError;
    QRegExp m_qtAssert;
    QRegExp m_qtTestFail;
    QWeakPointer<ProjectExplorer::Project> m_project;
    QString m_lastLine;
    QString m_deferedText;
    Utils::FileInProjectFinder m_projectFinder;
};


} // namespace Qt4ProjectManager

#endif // QTOUTPUTFORMATTER_H
