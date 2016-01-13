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
#include "projectpart.h"

#include <projectexplorer/headerpath.h>

#include <QDir>
#include <QTextStream>

namespace CppTools {

ProjectPart::ProjectPart()
    : project(0)
    , languageVersion(CXX14)
    , languageExtensions(NoExtensions)
    , qtVersion(UnknownQt)
    , warningFlags(ProjectExplorer::ToolChain::WarningsDefault)
    , selectedForBuilding(true)
{
}

static ProjectPartHeaderPath toProjectPartHeaderPath(const ProjectExplorer::HeaderPath &headerPath)
{
    const ProjectPartHeaderPath::Type headerPathType =
        headerPath.kind() == ProjectExplorer::HeaderPath::FrameworkHeaderPath
            ? ProjectPartHeaderPath::FrameworkPath
            : ProjectPartHeaderPath::IncludePath;

    return ProjectPartHeaderPath(headerPath.path(), headerPathType);
}

/*!
    \brief Retrieves info from concrete compiler using it's flags.

    \param tc Either nullptr or toolchain for project's active target.
    \param cxxflags C++ or Objective-C++ flags.
    \param cflags C or ObjectiveC flags if possible, \a cxxflags otherwise.
*/
void ProjectPart::evaluateToolchain(const ProjectExplorer::ToolChain *tc,
                                    const QStringList &commandLineFlags,
                                    const Utils::FileName &sysRoot)
{
    if (!tc)
        return;

    using namespace ProjectExplorer;
    ToolChain::CompilerFlags flags = tc->compilerFlags(commandLineFlags);

    if (flags & ToolChain::StandardC11)
        languageVersion = C11;
    else if (flags & ToolChain::StandardC99)
        languageVersion = C99;
    else if (flags & ToolChain::StandardCxx17)
        languageVersion = CXX17;
    else if (flags & ToolChain::StandardCxx14)
        languageVersion = CXX14;
    else if (flags & ToolChain::StandardCxx11)
        languageVersion = CXX11;
    else if (flags & ToolChain::StandardCxx98)
        languageVersion = CXX98;
    else
        languageVersion = CXX11;

    if (flags & ToolChain::BorlandExtensions)
        languageExtensions |= BorlandExtensions;
    if (flags & ToolChain::GnuExtensions)
        languageExtensions |= GnuExtensions;
    if (flags & ToolChain::MicrosoftExtensions)
        languageExtensions |= MicrosoftExtensions;
    if (flags & ToolChain::OpenMP)
        languageExtensions |= OpenMPExtensions;
    if (flags & ToolChain::ObjectiveC)
        languageExtensions |= ObjectiveCExtensions;

    warningFlags = tc->warningFlags(commandLineFlags);

    const QList<ProjectExplorer::HeaderPath> headers = tc->systemHeaderPaths(commandLineFlags, sysRoot);
    foreach (const ProjectExplorer::HeaderPath &header, headers) {
        const ProjectPartHeaderPath headerPath = toProjectPartHeaderPath(header);
        if (!headerPaths.contains(headerPath))
            headerPaths << headerPath;
    }

    toolchainDefines = tc->predefinedMacros(commandLineFlags);
    toolchainType = tc->typeId();
    updateLanguageFeatures();
}

void ProjectPart::updateLanguageFeatures()
{
    const bool hasQt = qtVersion != NoQt;
    languageFeatures.cxx11Enabled = languageVersion >= CXX11;
    languageFeatures.qtEnabled = hasQt;
    languageFeatures.qtMocRunEnabled = hasQt;
    if (!hasQt) {
        languageFeatures.qtKeywordsEnabled = false;
    } else {
        const QByteArray noKeywordsMacro = "#define QT_NO_KEYWORDS";
        const int noKeywordsIndex = projectDefines.indexOf(noKeywordsMacro);
        if (noKeywordsIndex == -1) {
            languageFeatures.qtKeywordsEnabled = true;
        } else {
            const char nextChar = projectDefines.at(noKeywordsIndex + noKeywordsMacro.length());
            // Detect "#define QT_NO_KEYWORDS" and "#define QT_NO_KEYWORDS 1", but exclude
            // "#define QT_NO_KEYWORDS_FOO"
            languageFeatures.qtKeywordsEnabled = nextChar != '\n' && nextChar != ' ';
        }
    }
}

ProjectPart::Ptr ProjectPart::copy() const
{
    return Ptr(new ProjectPart(*this));
}

QString ProjectPart::id() const
{
    QString projectPartId = QDir::fromNativeSeparators(projectFile);
    if (!displayName.isEmpty())
        projectPartId.append(QLatin1Char(' ') + displayName);
    return projectPartId;
}

QByteArray ProjectPart::readProjectConfigFile(const ProjectPart::Ptr &part)
{
    QByteArray result;

    QFile f(part->projectConfigFile);
    if (f.open(QIODevice::ReadOnly)) {
        QTextStream is(&f);
        result = is.readAll().toUtf8();
        f.close();
    }

    return result;
}

} // namespace CppTools
