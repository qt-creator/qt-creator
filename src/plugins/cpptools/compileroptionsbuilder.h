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

enum class UseSystemHeader : char
{
    Yes,
    No
};

enum class SkipBuiltIn : char
{
    Yes,
    No
};

enum class SkipLanguageDefines : char
{
    Yes,
    No
};

class CPPTOOLS_EXPORT CompilerOptionsBuilder
{
public:
    enum class PchUsage {
        None,
        Use
    };

    CompilerOptionsBuilder(const ProjectPart &projectPart,
                           UseSystemHeader useSystemHeader = UseSystemHeader::No,
                           SkipBuiltIn skipBuiltInHeaderPathsAndDefines = SkipBuiltIn::No,
                           SkipLanguageDefines skipLanguageDefines = SkipLanguageDefines::Yes,
                           QString clangVersion = QString(),
                           QString clangResourceDirectory = QString());

    QStringList build(ProjectFile::Kind fileKind,
                      PchUsage pchUsage);
    QStringList options() const;

    virtual void addExtraOptions() {}
    // Add options based on project part
    virtual void addToolchainAndProjectMacros();
    void addWordWidth();
    void addToolchainFlags();
    void addHeaderPathOptions();
    void addPrecompiledHeaderOptions(PchUsage pchUsage);
    void addMacros(const ProjectExplorer::Macros &macros);

    void addTargetTriple();
    void addExtraCodeModelFlags();
    void enableExceptions();
    void insertWrappedQtHeaders();
    void addOptionsForLanguage(bool checkForBorlandExtensions = true);
    void updateLanguageOption(ProjectFile::Kind fileKind);

    void addMsvcCompatibilityVersion();
    void undefineCppLanguageFeatureMacrosForMsvc2015();
    void addDefineFunctionMacrosMsvc();
    void addBoostWorkaroundMacros();

    void addProjectConfigFileInclude();
    void undefineClangVersionMacrosForMsvc();

protected:
    virtual bool excludeDefineDirective(const ProjectExplorer::Macro &macro) const;
    virtual bool excludeHeaderPath(const QString &headerPath) const;

    virtual QString defineOption() const;
    virtual QString undefineOption() const;
    virtual QString includeOption() const;

    // Add custom options
    void add(const QString &option);

    QString includeDirOptionForPath(const QString &path) const;

    const ProjectPart &m_projectPart;

private:
    QByteArray macroOption(const ProjectExplorer::Macro &macro) const;
    QByteArray toDefineOption(const ProjectExplorer::Macro &macro) const;
    QString defineDirectiveToDefineOption(const ProjectExplorer::Macro &marco) const;

    void addWrappedQtHeadersIncludePath(QStringList &list);

    QByteArray msvcVersion() const;

    QStringList m_options;

    QString m_clangVersion;
    QString m_clangResourceDirectory;

    UseSystemHeader m_useSystemHeader;
    SkipBuiltIn m_skipBuiltInHeaderPathsAndDefines;
    SkipLanguageDefines m_skipLanguageDefines;
};

} // namespace CppTools
