/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppmodelmanagerinterface.h"

#include <cplusplus/pp-engine.h>

#include <projectexplorer/headerpath.h>
#include <projectexplorer/toolchain.h>

#include <QSet>

using namespace CppTools;
using namespace ProjectExplorer;

/*!
    \enum CppTools::CppModelManagerInterface::ProgressNotificationMode

    This enum type specifies whether a progress bar notification should be
    shown if more than one file is requested to update via
    CppModelManagerInterface::updateSourceFiles().

    \value ForcedProgressNotification
           Notify regardless of the number of files requested for update.
    \value ReservedProgressNotification
           Notify only if more than one file is requested for update.
*/

/*!
    \enum CppTools::CppModelManagerInterface::QtVersion

    Allows C++ parser engine to inject headers or change inner settings as
    needed to parse Qt language extensions for concrete major Qt version

    \value UnknownQt
           Parser may choose any policy
    \value NoQt
           Parser must not use special tricks, because it parses non-qt project
    \value Qt4
           Parser may enable tricks for Qt v4.x
    \value Qt5
           Parser may enable tricks for Qt v5.x
*/

/*!
    \fn virtual QFuture<void> updateProjectInfo(const ProjectInfo &pinfo) = 0;
    \param pinfo Updated ProjectInfo.
    \return A future that reports progress and allows to cancel the reparsing operation.

    This function is expected to be called by the project managers to update the
    code model with new project information.

    In particular, the function should be called in case:
        1. A new project is opened/created
        2. The project configuration changed. This includes
             2.1 Changes of defines, includes, framework paths
             2.2 Addition/Removal of project files

    \sa CppTools::CppModelManagerInterface::updateSourceFiles()
*/

/*!
    \fn virtual QFuture<void> updateSourceFiles(const QStringList &sourceFiles, ProgressNotificationMode mode = ReservedProgressNotification) = 0;
    \param sourceFiles List of source file to update. The items are absolute paths.
    \param mode The progress modification mode.
    \return A future that reports progress and allows to cancel the reparsing operation.

    Trigger an asynchronous reparsing of the given source files.

    This function is not meant to be called by the project managers.

    \sa CppTools::CppModelManagerInterface::ProgressNotificationMode
    \sa CppTools::CppModelManagerInterface::updateProjectInfo()
*/

ProjectPart::ProjectPart()
    : project(0)
    , cVersion(C89)
    , cxxVersion(CXX11)
    , cxxExtensions(NoExtensions)
    , qtVersion(UnknownQt)
    , cWarningFlags(ProjectExplorer::ToolChain::WarningsDefault)
    , cxxWarningFlags(ProjectExplorer::ToolChain::WarningsDefault)

{
}

/*!
    \brief Retrieves info from concrete compiler using it's flags.

    \param tc Either nullptr or toolchain for project's active target.
    \param cxxflags C++ or Objective-C++ flags.
    \param cflags C or ObjectiveC flags if possible, \a cxxflags otherwise.
*/
void ProjectPart::evaluateToolchain(const ToolChain *tc,
                                    const QStringList &cxxflags,
                                    const QStringList &cflags,
                                    const Utils::FileName &sysRoot)
{
    if (!tc)
        return;

    ToolChain::CompilerFlags cxx = tc->compilerFlags(cxxflags);
    ToolChain::CompilerFlags c = (cxxflags == cflags)
            ? cxx : tc->compilerFlags(cflags);

    if (c & ToolChain::StandardC11)
        cVersion = C11;
    else if (c & ToolChain::StandardC99)
        cVersion = C99;
    else
        cVersion = C89;

    if (cxx & ToolChain::StandardCxx11)
        cxxVersion = CXX11;
    else
        cxxVersion = CXX98;

    if (cxx & ToolChain::BorlandExtensions)
        cxxExtensions |= BorlandExtensions;
    if (cxx & ToolChain::GnuExtensions)
        cxxExtensions |= GnuExtensions;
    if (cxx & ToolChain::MicrosoftExtensions)
        cxxExtensions |= MicrosoftExtensions;
    if (cxx & ToolChain::OpenMP)
        cxxExtensions |= OpenMPExtensions;

    cWarningFlags = tc->warningFlags(cflags);
    cxxWarningFlags = tc->warningFlags(cxxflags);

    const QList<HeaderPath> headers = tc->systemHeaderPaths(cxxflags, sysRoot);
    foreach (const HeaderPath &header, headers)
        if (header.kind() == HeaderPath::FrameworkHeaderPath)
            frameworkPaths << header.path();
        else
            includePaths << header.path();

    toolchainDefines = tc->predefinedMacros(cxxflags);
}

const QString CppModelManagerInterface::configurationFileName()
{ return CPlusPlus::Preprocessor::configurationFileName; }

const QString CppModelManagerInterface::editorConfigurationFileName()
{
    return QLatin1String("<per-editor-defines>");
}

CppModelManagerInterface::CppModelManagerInterface(QObject *parent)
    : CPlusPlus::CppModelManagerBase(parent)
{ }

CppModelManagerInterface::~CppModelManagerInterface()
{ }

CppModelManagerInterface *CppModelManagerInterface::instance()
{
    return qobject_cast<CppModelManagerInterface *>(CPlusPlus::CppModelManagerBase::instance());
}

void CppModelManagerInterface::ProjectInfo::clearProjectParts()
{
    m_projectParts.clear();
    m_includePaths.clear();
    m_frameworkPaths.clear();
    m_sourceFiles.clear();
    m_defines.clear();
}

void CppModelManagerInterface::ProjectInfo::appendProjectPart(const ProjectPart::Ptr &part)
{
    if (!part)
        return;

    m_projectParts.append(part);

    // Update include paths
    QSet<QString> incs = QSet<QString>::fromList(m_includePaths);
    foreach (const QString &ins, part->includePaths)
        incs.insert(ins);
    m_includePaths = incs.toList();

    // Update framework paths
    QSet<QString> frms = QSet<QString>::fromList(m_frameworkPaths);
    foreach (const QString &frm, part->frameworkPaths)
        frms.insert(frm);
    m_frameworkPaths = frms.toList();

    // Update source files
    QSet<QString> srcs = QSet<QString>::fromList(m_sourceFiles);
    foreach (const ProjectFile &file, part->files)
        srcs.insert(file.path);
    m_sourceFiles = srcs.toList();

    // Update defines
    if (!m_defines.isEmpty())
        m_defines.append('\n');
    m_defines.append(part->toolchainDefines);
    m_defines.append(part->projectDefines);
}
