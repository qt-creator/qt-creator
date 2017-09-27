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

#pragma once

#include "cpptools_global.h"

#include "projectpart.h"

namespace CppTools {

class CPPTOOLS_EXPORT CompilerOptionsBuilder
{
public:
    enum class PchUsage {
        None,
        Use
    };

    CompilerOptionsBuilder(const ProjectPart &projectPart,
                           const QString &clangVersion = QString(),
                           const QString &clangResourceDirectory = QString());
    virtual ~CompilerOptionsBuilder() {}

    virtual void addTargetTriple();
    virtual void enableExceptions();
    virtual void addPredefinedHeaderPathsOptions();
    virtual void addLanguageOption(ProjectFile::Kind fileKind);
    virtual void addOptionsForLanguage(bool checkForBorlandExtensions = true);

    virtual void addExtraOptions() {}

    QStringList build(ProjectFile::Kind fileKind,
                      PchUsage pchUsage);
    QStringList options() const;

    // Add custom options
    void add(const QString &option);
    void addDefine(const ProjectExplorer::Macro &marco);

    // Add options based on project part
    void addWordWidth();
    void addHeaderPathOptions();
    void addPrecompiledHeaderOptions(PchUsage pchUsage);
    void addToolchainAndProjectMacros();
    void addMacros(const ProjectExplorer::Macros &macros);

    void addMsvcCompatibilityVersion();
    void undefineCppLanguageFeatureMacrosForMsvc2015();

    void addDefineFloat128ForMingw();
    void addProjectConfigFileInclude();
    void undefineClangVersionMacrosForMsvc();

protected:
    virtual bool excludeDefineDirective(const ProjectExplorer::Macro &macro) const;
    virtual bool excludeHeaderPath(const QString &headerPath) const;

    virtual QString defineOption() const;
    virtual QString undefineOption() const;
    virtual QString includeOption() const;
    virtual QString includeDirOption() const;

    const ProjectPart m_projectPart;

private:
    QByteArray macroOption(const ProjectExplorer::Macro &macro) const;
    QByteArray toDefineOption(const ProjectExplorer::Macro &macro) const;
    QString defineDirectiveToDefineOption(const ProjectExplorer::Macro &marco) const;
    QString clangIncludeDirectory() const;

    QStringList m_options;
    QString m_clangVersion;
    QString m_clangResourceDirectory;
};

} // namespace CppTools
