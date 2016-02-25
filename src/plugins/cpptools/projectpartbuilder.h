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

#ifndef CPPTOOLS_PROJECTPARTBUILDER_H
#define CPPTOOLS_PROJECTPARTBUILDER_H

#include "cpptools_global.h"

#include "projectinfo.h"
#include "projectpart.h"

#include <functional>

namespace ProjectExplorer {
class ToolChain;
}

namespace CppTools {

class CPPTOOLS_EXPORT ProjectPartBuilder
{
public:
    ProjectPartBuilder(ProjectInfo &m_pInfo);

    void setQtVersion(ProjectPart::QtVersion qtVersion);
    void setCFlags(const QStringList &flags);
    void setCxxFlags(const QStringList &flags);
    void setDefines(const QByteArray &defines);
    void setHeaderPaths(const ProjectPartHeaderPaths &headerPaths);
    void setIncludePaths(const QStringList &includePaths);
    void setPreCompiledHeaders(const QStringList &pchs);
    void setProjectFile(const QString &projectFile);
    void setDisplayName(const QString &displayName);
    void setConfigFileName(const QString &configFileName);

    using FileClassifier = std::function<ProjectFile::Kind (const QString &filePath)>;

    QList<Core::Id> createProjectPartsForFiles(const QStringList &files,
                                               FileClassifier fileClassifier = FileClassifier());

    static void evaluateProjectPartToolchain(ProjectPart *projectPart,
                                             const ProjectExplorer::ToolChain *toolChain,
                                             const QStringList &commandLineFlags,
                                             const Utils::FileName &sysRoot);

private:
    void createProjectPart(const QVector<ProjectFile> &theSources,
                           const QString &partName,
                           ProjectPart::LanguageVersion languageVersion,
                           ProjectPart::LanguageExtensions languageExtensions);

private:
    ProjectPart::Ptr m_templatePart;
    ProjectInfo &m_pInfo;
    QStringList m_cFlags, m_cxxFlags;
};

} // namespace CppTools

#endif // CPPTOOLS_PROJECTPARTBUILDER_H
