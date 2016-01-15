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

#include "compileroptionsbuilder.h"

#include <projectexplorer/projectexplorerconstants.h>

namespace CppTools {

CompilerOptionsBuilder::CompilerOptionsBuilder(const ProjectPart &projectPart)
    : m_projectPart(projectPart)
{
}

QStringList CompilerOptionsBuilder::options() const
{
    return m_options;
}

void CompilerOptionsBuilder::add(const QString &option)
{
    m_options.append(option);
}

QString CompilerOptionsBuilder::defineLineToDefineOption(const QByteArray &defineLine)
{
    QByteArray str = defineLine.mid(8);
    int spaceIdx = str.indexOf(' ');
    const QString option = defineOption();
    const bool hasValue = spaceIdx != -1;
    QString arg = option + QLatin1String(str.left(hasValue ? spaceIdx : str.size()) + '=');
    if (hasValue)
        arg += QLatin1String(str.mid(spaceIdx + 1));
    return arg;
}

void CompilerOptionsBuilder::addDefine(const QByteArray &defineLine)
{
    m_options.append(defineLineToDefineOption(defineLine));
}

void CompilerOptionsBuilder::addHeaderPathOptions()
{
    typedef ProjectPartHeaderPath HeaderPath;
    const QString defaultPrefix = includeOption();

    QStringList result;

    foreach (const HeaderPath &headerPath , m_projectPart.headerPaths) {
        if (headerPath.path.isEmpty())
            continue;

        if (excludeHeaderPath(headerPath.path))
            continue;

        QString prefix;
        switch (headerPath.type) {
        case HeaderPath::FrameworkPath:
            prefix = QLatin1String("-F");
            break;
        default: // This shouldn't happen, but let's be nice..:
            // intentional fall-through:
        case HeaderPath::IncludePath:
            prefix = defaultPrefix;
            break;
        }

        result.append(prefix + headerPath.path);
    }

    m_options.append(result);
}

void CompilerOptionsBuilder::addToolchainAndProjectDefines()
{
    QByteArray extendedDefines = m_projectPart.toolchainDefines + m_projectPart.projectDefines;
    QStringList result;

    foreach (QByteArray def, extendedDefines.split('\n')) {
        if (def.isEmpty() || excludeDefineLine(def))
            continue;

        const QString defineOption = defineLineToDefineOption(def);
        if (!result.contains(defineOption))
            result.append(defineOption);
    }

    m_options.append(result);
}

static QStringList createLanguageOptionGcc(ProjectFile::Kind fileKind, bool objcExt)
{
    QStringList opts;

    switch (fileKind) {
    case ProjectFile::Unclassified:
        break;
    case ProjectFile::CHeader:
        if (objcExt)
            opts += QLatin1String("objective-c-header");
        else
            opts += QLatin1String("c-header");
        break;

    case ProjectFile::CXXHeader:
    default:
        if (!objcExt) {
            opts += QLatin1String("c++-header");
            break;
        } // else: fall-through!
    case ProjectFile::ObjCHeader:
    case ProjectFile::ObjCXXHeader:
        opts += QLatin1String("objective-c++-header");
        break;

    case ProjectFile::CSource:
        if (!objcExt) {
            opts += QLatin1String("c");
            break;
        } // else: fall-through!
    case ProjectFile::ObjCSource:
        opts += QLatin1String("objective-c");
        break;

    case ProjectFile::CXXSource:
        if (!objcExt) {
            opts += QLatin1String("c++");
            break;
        } // else: fall-through!
    case ProjectFile::ObjCXXSource:
        opts += QLatin1String("objective-c++");
        break;

    case ProjectFile::OpenCLSource:
        opts += QLatin1String("cl");
        break;
    case ProjectFile::CudaSource:
        opts += QLatin1String("cuda");
        break;
    }

    if (!opts.isEmpty())
        opts.prepend(QLatin1String("-x"));

    return opts;
}

void CompilerOptionsBuilder::addLanguageOption(ProjectFile::Kind fileKind)
{
    const bool objcExt = m_projectPart.languageExtensions & ProjectPart::ObjectiveCExtensions;
    const QStringList options = createLanguageOptionGcc(fileKind, objcExt);
    m_options.append(options);
}

void CompilerOptionsBuilder::addOptionsForLanguage(bool checkForBorlandExtensions)
{
    QStringList opts;
    const ProjectPart::LanguageExtensions languageExtensions = m_projectPart.languageExtensions;
    const bool gnuExtensions = languageExtensions & ProjectPart::GnuExtensions;
    switch (m_projectPart.languageVersion) {
    case ProjectPart::C89:
        opts << (gnuExtensions ? QLatin1String("-std=gnu89") : QLatin1String("-std=c89"));
        break;
    case ProjectPart::C99:
        opts << (gnuExtensions ? QLatin1String("-std=gnu99") : QLatin1String("-std=c99"));
        break;
    case ProjectPart::C11:
        opts << (gnuExtensions ? QLatin1String("-std=gnu11") : QLatin1String("-std=c11"));
        break;
    case ProjectPart::CXX11:
        opts << (gnuExtensions ? QLatin1String("-std=gnu++11") : QLatin1String("-std=c++11"));
        break;
    case ProjectPart::CXX98:
        opts << (gnuExtensions ? QLatin1String("-std=gnu++98") : QLatin1String("-std=c++98"));
        break;
    case ProjectPart::CXX03:
        // Clang 3.6 does not know -std=gnu++03.
        opts << QLatin1String("-std=c++03");
        break;
    case ProjectPart::CXX14:
        opts << (gnuExtensions ? QLatin1String("-std=gnu++14") : QLatin1String("-std=c++14"));
        break;
    case ProjectPart::CXX17:
        // TODO: Change to (probably) "gnu++17"/"c++17" at some point in the future.
        opts << (gnuExtensions ? QLatin1String("-std=gnu++1z") : QLatin1String("-std=c++1z"));
        break;
    }

    if (languageExtensions & ProjectPart::MicrosoftExtensions)
        opts << QLatin1String("-fms-extensions");

    if (checkForBorlandExtensions && (languageExtensions & ProjectPart::BorlandExtensions))
        opts << QLatin1String("-fborland-extensions");

    m_options.append(opts);
}

QString CompilerOptionsBuilder::includeOption() const
{
    return QLatin1String("-I");
}

QString CompilerOptionsBuilder::defineOption() const
{
    return QLatin1String("-D");
}

bool CompilerOptionsBuilder::excludeDefineLine(const QByteArray &defineLine) const
{
    // This is a quick fix for QTCREATORBUG-11501.
    // TODO: do a proper fix, see QTCREATORBUG-11709.
    if (defineLine.startsWith("#define __cplusplus"))
        return true;

    // gcc 4.9 has:
    //    #define __has_include(STR) __has_include__(STR)
    //    #define __has_include_next(STR) __has_include_next__(STR)
    // The right-hand sides are gcc built-ins that clang does not understand, and they'd
    // override clang's own (non-macro, it seems) definitions of the symbols on the left-hand
    // side.
    const bool isGccToolchain = m_projectPart.toolchainType == ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID;
    if (isGccToolchain && defineLine.contains("has_include"))
        return true;

    return false;
}

bool CompilerOptionsBuilder::excludeHeaderPath(const QString &headerPath) const
{
    Q_UNUSED(headerPath);
    return false;
}

} // namespace CppTools
