/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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

    struct CompilerCallGroup {
        using CallsPerSourceFile = QHash<QString, QList<QStringList>>;

        QString groupId;
        CallsPerSourceFile callsPerSourceFile;
    };
    using CompilerCallData = QVector<CompilerCallGroup>;
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
