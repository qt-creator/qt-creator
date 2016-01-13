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

#ifndef CPPTOOLS_PROJECTINFO_H
#define CPPTOOLS_PROJECTINFO_H

#include "cpptools_global.h"

#include "projectpart.h"

#include <QHash>
#include <QPointer>
#include <QSet>

namespace CppTools {

class CPPTOOLS_EXPORT ProjectInfo
{
public:
    ProjectInfo();
    ProjectInfo(QPointer<ProjectExplorer::Project> project);

    bool isValid() const;

    bool operator ==(const ProjectInfo &other) const;
    bool operator !=(const ProjectInfo &other) const;
    bool definesChanged(const ProjectInfo &other) const;
    bool configurationChanged(const ProjectInfo &other) const;
    bool configurationOrFilesChanged(const ProjectInfo &other) const;

    QPointer<ProjectExplorer::Project> project() const;
    const QList<ProjectPart::Ptr> projectParts() const;

    void appendProjectPart(const ProjectPart::Ptr &part);
    void finish();

    const ProjectPartHeaderPaths headerPaths() const;
    const QSet<QString> sourceFiles() const;
    const QByteArray defines() const;

    // Source file --> List of compiler calls
    typedef QHash<QString, QList<QStringList>> CompilerCallData;
    void setCompilerCallData(const CompilerCallData &data);
    CompilerCallData compilerCallData() const;

private:
    QPointer<ProjectExplorer::Project> m_project;
    QList<ProjectPart::Ptr> m_projectParts;
    CompilerCallData m_compilerCallData;
    // The members below are (re)calculated from the project parts with finish()
    ProjectPartHeaderPaths m_headerPaths;
    QSet<QString> m_sourceFiles;
    QByteArray m_defines;
};

} // namespace CppTools

#endif // CPPTOOLS_PROJECTINFO_H
