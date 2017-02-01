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

#include <QDir>
#include <QRegularExpression>

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

struct Macro {
    static Macro fromDefineDirective(const QByteArray &defineDirective);
    QByteArray toDefineOption(const QByteArray &option) const;

    QByteArray name;
    QByteArray value;
};

Macro Macro::fromDefineDirective(const QByteArray &defineDirective)
{
    const QByteArray str = defineDirective.mid(8);
    const int spaceIdx = str.indexOf(' ');
    const bool hasValue = spaceIdx != -1;

    Macro macro;
    macro.name = str.left(hasValue ? spaceIdx : str.size());
    if (hasValue)
        macro.value = str.mid(spaceIdx + 1);

    return macro;
}

QByteArray Macro::toDefineOption(const QByteArray &option) const
{
    QByteArray result;

    result.append(option);
    result.append(name);
    result.append('=');
    if (!value.isEmpty())
        result.append(value);

    return result;
}

void CompilerOptionsBuilder::addDefine(const QByteArray &defineDirective)
{
    m_options.append(defineDirectiveToDefineOption(defineDirective));
}

void CompilerOptionsBuilder::addWordWidth()
{
    const QString argument = m_projectPart.toolChainWordWidth == ProjectPart::WordWidth64Bit
            ? QLatin1String("-m64")
            : QLatin1String("-m32");
    add(argument);
}

void CompilerOptionsBuilder::addTargetTriple()
{
    if (!m_projectPart.toolChainTargetTriple.isEmpty()) {
        m_options.append(QLatin1String("-target"));
        m_options.append(m_projectPart.toolChainTargetTriple);
    }
}

void CompilerOptionsBuilder::enableExceptions()
{
    add(QLatin1String("-fcxx-exceptions"));
    add(QLatin1String("-fexceptions"));
}

void CompilerOptionsBuilder::addHeaderPathOptions()
{
    typedef ProjectPartHeaderPath HeaderPath;
    const QString defaultPrefix = includeDirOption();

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

        result.append(prefix + QDir::toNativeSeparators(headerPath.path));
    }

    m_options.append(result);
}

void CompilerOptionsBuilder::addPrecompiledHeaderOptions(PchUsage pchUsage)
{
    if (pchUsage == PchUsage::None)
        return;

    QStringList result;

    const QString includeOptionString = includeOption();
    foreach (const QString &pchFile, m_projectPart.precompiledHeaders) {
        if (QFile::exists(pchFile)) {
            result += includeOptionString;
            result += QDir::toNativeSeparators(pchFile);
        }
    }

    m_options.append(result);
}

void CompilerOptionsBuilder::addToolchainAndProjectDefines()
{
    addDefines(m_projectPart.toolchainDefines);
    addDefines(m_projectPart.projectDefines);
}

void CompilerOptionsBuilder::addDefines(const QByteArray &defineDirectives)
{
    QStringList result;

    foreach (QByteArray def, defineDirectives.split('\n')) {
        if (def.isEmpty() || excludeDefineDirective(def))
            continue;

        const QString defineOption = defineDirectiveToDefineOption(def);
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
    case ProjectFile::Unsupported:
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

void CompilerOptionsBuilder::addDefineToAvoidIncludingGccOrMinGwIntrinsics()
{
    // In gcc headers, lots of built-ins are referenced that clang does not understand.
    // Therefore, prevent the inclusion of the header that references them. Of course, this
    // will break if code actually requires stuff from there, but that should be the less common
    // case.

    const Core::Id type = m_projectPart.toolchainType;
    if (type == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID
            || type == ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID) {
        addDefine("#define _X86INTRIN_H_INCLUDED");
    }
}

static QByteArray toMsCompatibilityVersionFormat(const QByteArray &mscFullVer)
{
    return mscFullVer.left(2)
         + QByteArray(".")
         + mscFullVer.mid(2, 2);
}

static QByteArray msCompatibilityVersionFromDefines(const QByteArray &defineDirectives)
{
    foreach (QByteArray defineDirective, defineDirectives.split('\n')) {
        if (defineDirective.isEmpty())
            continue;

        const Macro macro = Macro::fromDefineDirective(defineDirective);
        if (macro.name == "_MSC_FULL_VER")
            return toMsCompatibilityVersionFormat(macro.value);
    }

    return QByteArray();
}

void CompilerOptionsBuilder::addMsvcCompatibilityVersion()
{
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID) {
        const QByteArray defines = m_projectPart.toolchainDefines + m_projectPart.projectDefines;
        const QByteArray msvcVersion = msCompatibilityVersionFromDefines(defines);

        if (!msvcVersion.isEmpty()) {
            const QString option = QLatin1String("-fms-compatibility-version=")
                    + QLatin1String(msvcVersion);
            m_options.append(option);
        }
    }
}

static QStringList languageFeatureMacros()
{
    // Collected with:
    //  $ CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86
    //  $ D:\usr\llvm-3.8.0\bin\clang++.exe -fms-compatibility-version=19 -std=c++1y -dM -E D:\empty.cpp | grep __cpp_
    static QStringList macros {
        QLatin1String("__cpp_aggregate_nsdmi"),
        QLatin1String("__cpp_alias_templates"),
        QLatin1String("__cpp_attributes"),
        QLatin1String("__cpp_binary_literals"),
        QLatin1String("__cpp_constexpr"),
        QLatin1String("__cpp_decltype"),
        QLatin1String("__cpp_decltype_auto"),
        QLatin1String("__cpp_delegating_constructors"),
        QLatin1String("__cpp_digit_separators"),
        QLatin1String("__cpp_generic_lambdas"),
        QLatin1String("__cpp_inheriting_constructors"),
        QLatin1String("__cpp_init_captures"),
        QLatin1String("__cpp_initializer_lists"),
        QLatin1String("__cpp_lambdas"),
        QLatin1String("__cpp_nsdmi"),
        QLatin1String("__cpp_range_based_for"),
        QLatin1String("__cpp_raw_strings"),
        QLatin1String("__cpp_ref_qualifiers"),
        QLatin1String("__cpp_return_type_deduction"),
        QLatin1String("__cpp_rtti"),
        QLatin1String("__cpp_rvalue_references"),
        QLatin1String("__cpp_static_assert"),
        QLatin1String("__cpp_unicode_characters"),
        QLatin1String("__cpp_unicode_literals"),
        QLatin1String("__cpp_user_defined_literals"),
        QLatin1String("__cpp_variable_templates"),
        QLatin1String("__cpp_variadic_templates"),
    };

    return macros;
}

void CompilerOptionsBuilder::undefineCppLanguageFeatureMacrosForMsvc2015()
{
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID
            && m_projectPart.isMsvc2015Toolchain) {
        // Undefine the language feature macros that are pre-defined in clang-cl 3.8.0,
        // but not in MSVC2015's cl.exe.
        foreach (const QString &macroName, languageFeatureMacros())
            m_options.append(undefineOption() + macroName);
    }
}

void CompilerOptionsBuilder::addDefineFloat128ForMingw()
{
    // TODO: Remove once this is fixed in clang >= 3.9.
    // https://llvm.org/bugs/show_bug.cgi?id=30685
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID)
        addDefine("#define __float128 void");
}

QString CompilerOptionsBuilder::includeDirOption() const
{
    return QLatin1String("-I");
}

QString CompilerOptionsBuilder::defineDirectiveToDefineOption(const QByteArray &defineDirective)
{
    const Macro macro = Macro::fromDefineDirective(defineDirective);
    const QByteArray option = macro.toDefineOption(defineOption().toLatin1());

    return QString::fromLatin1(option);
}

QString CompilerOptionsBuilder::defineOption() const
{
    return QLatin1String("-D");
}

QString CompilerOptionsBuilder::undefineOption() const
{
    return QLatin1String("-U");
}

QString CompilerOptionsBuilder::includeOption() const
{
    return QLatin1String("-include");
}

static bool isGccOrMinGwToolchain(const Core::Id &toolchainType)
{
    return toolchainType == ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID
        || toolchainType == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID;
}

bool CompilerOptionsBuilder::excludeDefineDirective(const QByteArray &defineDirective) const
{
    // This is a quick fix for QTCREATORBUG-11501.
    // TODO: do a proper fix, see QTCREATORBUG-11709.
    if (defineDirective.startsWith("#define __cplusplus"))
        return true;

    // gcc 4.9 has:
    //    #define __has_include(STR) __has_include__(STR)
    //    #define __has_include_next(STR) __has_include_next__(STR)
    // The right-hand sides are gcc built-ins that clang does not understand, and they'd
    // override clang's own (non-macro, it seems) definitions of the symbols on the left-hand
    // side.
    if (isGccOrMinGwToolchain(m_projectPart.toolchainType)
            && defineDirective.contains("has_include")) {
        return true;
    }

    // If _FORTIFY_SOURCE is defined (typically in release mode), it will
    // enable the inclusion of extra headers to help catching buffer overflows
    // (e.g. wchar.h includes wchar2.h). These extra headers use
    // __builtin_va_arg_pack, which clang does not support (yet), so avoid
    // including those.
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID
            && defineDirective.startsWith("#define _FORTIFY_SOURCE")) {
        return true;
    }

    // MinGW 6 supports some fancy asm output flags and uses them in an
    // intrinsics header pulled in by windows.h. Clang does not know them.
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID
            && defineDirective.startsWith("#define __GCC_ASM_FLAG_OUTPUTS__")) {
        return true;
    }

    return false;
}

bool CompilerOptionsBuilder::excludeHeaderPath(const QString &headerPath) const
{
    // A clang tool chain might have another version and passing in the
    // intrinsics path from that version will lead to errors (unknown
    // intrinsics, unfavorable order with regard to include_next).
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID) {
        static QRegularExpression clangIncludeDir(
                    QLatin1String("\\A.*/lib/clang/\\d+\\.\\d+(\\.\\d+)?/include\\z"));
        return clangIncludeDir.match(headerPath).hasMatch();
    }

    return false;
}

} // namespace CppTools
