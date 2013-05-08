/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <projectexplorer/toolchain.h>
#include <projectexplorer/headerpath.h>
#include <cplusplus/pp-engine.h>

#include <QtCore/QSet>

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

using namespace CppTools;
using namespace ProjectExplorer;

ProjectPart::ProjectPart()
    : cVersion(C89)
    , cxxVersion(CXX11)
    , cxxExtensions(NoExtensions)
    , qtVersion(UnknownQt)
    , cWarningFlags(ProjectExplorer::ToolChain::WarningsDefault)
    , cxxWarningFlags(ProjectExplorer::ToolChain::WarningsDefault)
{
}

/**
 * @brief Retrieves info from concrete compiler using it's flags.
 * @param tc Either nullptr or toolchain for project's active target.
 * @param cxxflags C++ or Objective-C++ flags.
 * @param cflags C or ObjectiveC flags if possible, \a cxxflags otherwise.
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

    if (c | ToolChain::StandardC11)
        cVersion = C11;
    else if (c | ToolChain::StandardC99)
        cVersion = C99;
    else
        cVersion = C89;

    if (cxx | ToolChain::StandardCxx11)
        cxxVersion = CXX11;
    else
        cxxVersion = CXX98;

    if (cxx | ToolChain::BorlandExtensions)
        cxxExtensions |= BorlandExtensions;
    if (cxx | ToolChain::GnuExtensions)
        cxxExtensions |= GnuExtensions;
    if (cxx | ToolChain::MicrosoftExtensions)
        cxxExtensions |= MicrosoftExtensions;
    if (cxx | ToolChain::OpenMP)
        cxxExtensions |= OpenMP;

    cWarningFlags = tc->warningFlags(cflags);
    cxxWarningFlags = tc->warningFlags(cxxflags);

    QList<HeaderPath> headers = tc->systemHeaderPaths(cxxflags, sysRoot);
    foreach (const HeaderPath &header, headers)
        if (header.kind() == HeaderPath::FrameworkHeaderPath)
            frameworkPaths << header.path();
        else
            includePaths << header.path();

    QByteArray macros = tc->predefinedMacros(cxxflags);
    if (!macros.isEmpty()) {
        if (!defines.isEmpty())
            defines += '\n';
        defines += macros;
        defines += '\n';
    }
}

static CppModelManagerInterface *g_instance = 0;

const QString CppModelManagerInterface::configurationFileName()
{ return CPlusPlus::Preprocessor::configurationFileName; }

CppModelManagerInterface::CppModelManagerInterface(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(! g_instance);
    g_instance = this;
}

CppModelManagerInterface::~CppModelManagerInterface()
{
    Q_ASSERT(g_instance == this);
    g_instance = 0;
}

CppModelManagerInterface *CppModelManagerInterface::instance()
{
    return g_instance;
}


void CppModelManagerInterface::ProjectInfo::clearProjectParts()
{
    m_projectParts.clear();
    m_includePaths.clear();
    m_frameworkPaths.clear();
    m_sourceFiles.clear();
    m_defines.clear();
}

void CppModelManagerInterface::ProjectInfo::appendProjectPart(
        const ProjectPart::Ptr &part)
{
    if (!part)
        return;

    m_projectParts.append(part);

    // update include paths
    QSet<QString> incs = QSet<QString>::fromList(m_includePaths);
    foreach (const QString &ins, part->includePaths)
        incs.insert(ins);
    m_includePaths = incs.toList();

    // update framework paths
    QSet<QString> frms = QSet<QString>::fromList(m_frameworkPaths);
    foreach (const QString &frm, part->frameworkPaths)
        frms.insert(frm);
    m_frameworkPaths = frms.toList();

    // update source files
    QSet<QString> srcs = QSet<QString>::fromList(m_sourceFiles);
    foreach (const ProjectFile &file, part->files)
        srcs.insert(file.path);
    m_sourceFiles = srcs.toList();

    // update defines
    if (!m_defines.isEmpty())
        m_defines.append('\n');
    m_defines.append(part->defines);
}
