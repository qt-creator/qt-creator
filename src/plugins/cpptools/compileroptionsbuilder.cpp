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

#include "cppmodelmanager.h"
#include "headerpathfilter.h"

#include <coreplugin/icore.h>

#include <projectexplorer/headerpath.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacro.h>

#include <utils/cpplanguage_details.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QRegularExpression>
#include <QtGlobal>

namespace CppTools {

static const char defineOption[] = "-D";
static const char undefineOption[] = "-U";

static const char includeUserPathOption[] = "-I";
static const char includeUserPathOptionWindows[] = "/I";
static const char includeSystemPathOption[] = "-isystem";

static const char includeFileOptionGcc[] = "-include";
static const char includeFileOptionCl[] = "/FI";

static QByteArray macroOption(const ProjectExplorer::Macro &macro)
{
    switch (macro.type) {
    case ProjectExplorer::MacroType::Define:
        return defineOption;
    case ProjectExplorer::MacroType::Undefine:
        return undefineOption;
    default:
        return QByteArray();
    }
}

static QByteArray toDefineOption(const ProjectExplorer::Macro &macro)
{
    return macro.toKeyValue(macroOption(macro));
}

static QString defineDirectiveToDefineOption(const ProjectExplorer::Macro &macro)
{
    const QByteArray option = toDefineOption(macro);
    return QString::fromUtf8(option);
}

QStringList XclangArgs(const QStringList &args)
{
    QStringList options;
    for (const QString &arg : args) {
        options.append("-Xclang");
        options.append(arg);
    }
    return options;
}

QStringList clangArgsForCl(const QStringList &args)
{
    QStringList options;
    for (const QString &arg : args)
        options.append("/clang:" + arg);
    return options;
}

CompilerOptionsBuilder::CompilerOptionsBuilder(const ProjectPart &projectPart,
                                               UseSystemHeader useSystemHeader,
                                               UseTweakedHeaderPaths useTweakedHeaderPaths,
                                               UseLanguageDefines useLanguageDefines,
                                               UseBuildSystemWarnings useBuildSystemWarnings,
                                               const QString &clangVersion,
                                               const QString &clangResourceDirectory)
    : m_projectPart(projectPart)
    , m_useSystemHeader(useSystemHeader)
    , m_useTweakedHeaderPaths(useTweakedHeaderPaths)
    , m_useLanguageDefines(useLanguageDefines)
    , m_useBuildSystemWarnings(useBuildSystemWarnings)
    , m_clangVersion(clangVersion)
    , m_clangResourceDirectory(clangResourceDirectory)
{
}

QStringList CompilerOptionsBuilder::build(ProjectFile::Kind fileKind,
                                          UsePrecompiledHeaders usePrecompiledHeaders)
{
    m_options.clear();
    evaluateCompilerFlags();

    if (fileKind == ProjectFile::CHeader || fileKind == ProjectFile::CSource) {
        QTC_ASSERT(m_projectPart.languageVersion <= Utils::LanguageVersion::LatestC,
                   return QStringList(););
    }

    if (fileKind == ProjectFile::CXXHeader || fileKind == ProjectFile::CXXSource) {
        QTC_ASSERT(m_projectPart.languageVersion > Utils::LanguageVersion::LatestC,
                   return QStringList(););
    }

    addCompilerFlags();

    addSyntaxOnly();
    addWordWidth();
    addTargetTriple();
    updateFileLanguage(fileKind);
    addLanguageVersionAndExtensions();

    addPrecompiledHeaderOptions(usePrecompiledHeaders);
    addProjectConfigFileInclude();

    addExtraCodeModelFlags();

    addMsvcCompatibilityVersion();
    addProjectMacros();
    undefineClangVersionMacrosForMsvc();
    undefineCppLanguageFeatureMacrosForMsvc2015();
    addDefineFunctionMacrosMsvc();

    addHeaderPathOptions();

    addExtraOptions();

    insertWrappedQtHeaders();

    return options();
}

void CompilerOptionsBuilder::add(const QString &arg, bool gccOnlyOption)
{
    add(QStringList{arg}, gccOnlyOption);
}

void CompilerOptionsBuilder::add(const QStringList &args, bool gccOnlyOptions)
{
    m_options.append((gccOnlyOptions && isClStyle()) ? clangArgsForCl(args) : args);
}

void CompilerOptionsBuilder::addSyntaxOnly()
{
    isClStyle() ? add("/Zs") : add("-fsyntax-only");
}

QStringList createLanguageOptionGcc(ProjectFile::Kind fileKind, bool objcExt)
{
    QStringList options;

    switch (fileKind) {
    case ProjectFile::Unclassified:
    case ProjectFile::Unsupported:
        break;
    case ProjectFile::CHeader:
        if (objcExt)
            options += "objective-c-header";
        else
            options += "c-header";
        break;
    case ProjectFile::CXXHeader:
    default:
        if (!objcExt) {
            options += "c++-header";
            break;
        }
        Q_FALLTHROUGH();
    case ProjectFile::ObjCHeader:
    case ProjectFile::ObjCXXHeader:
        options += "objective-c++-header";
        break;

    case ProjectFile::CSource:
        if (!objcExt) {
            options += "c";
            break;
        }
        Q_FALLTHROUGH();
    case ProjectFile::ObjCSource:
        options += "objective-c";
        break;
    case ProjectFile::CXXSource:
        if (!objcExt) {
            options += "c++";
            break;
        }
        Q_FALLTHROUGH();
    case ProjectFile::ObjCXXSource:
        options += "objective-c++";
        break;
    case ProjectFile::OpenCLSource:
        options += "cl";
        break;
    case ProjectFile::CudaSource:
        options += "cuda";
        break;
    }

    if (!options.isEmpty())
        options.prepend("-x");

    return options;
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
    // Only "--target=" style is accepted in both g++ and cl driver modes.
    if (!m_projectPart.toolChainTargetTriple.isEmpty())
        add("--target=" + m_projectPart.toolChainTargetTriple);
}

void CompilerOptionsBuilder::addExtraCodeModelFlags()
{
    // extraCodeModelFlags keep build architecture for cross-compilation.
    // In case of iOS build target triple has aarch64 archtecture set which makes
    // code model fail with CXError_Failure. To fix that we explicitly provide architecture.
    add(m_projectPart.extraCodeModelFlags);
}

void CompilerOptionsBuilder::addPicIfCompilerFlagsContainsIt()
{
    if (m_projectPart.compilerFlags.contains("-fPIC"))
        add("-fPIC");
}

void CompilerOptionsBuilder::addCompilerFlags()
{
    add(m_compilerFlags.flags);
}

static QString creatorResourcePath()
{
#ifndef UNIT_TESTS
    return Core::ICore::resourcePath();
#else
    return QDir::toNativeSeparators(QString::fromUtf8(QTC_RESOURCE_DIR ""));
#endif
}

void CompilerOptionsBuilder::insertWrappedQtHeaders()
{
    if (m_useTweakedHeaderPaths == UseTweakedHeaderPaths::No)
        return;

    QStringList wrappedQtHeaders;
    addWrappedQtHeadersIncludePath(wrappedQtHeaders);

    const int index = m_options.indexOf(QRegularExpression("\\A-I.*\\z"));
    if (index < 0)
        add(wrappedQtHeaders);
    else
        m_options = m_options.mid(0, index) + wrappedQtHeaders + m_options.mid(index);
}

void CompilerOptionsBuilder::addHeaderPathOptions()
{
    HeaderPathFilter filter{m_projectPart,
                            m_useTweakedHeaderPaths,
                            m_clangVersion,
                            m_clangResourceDirectory};

    filter.process();

    using ProjectExplorer::HeaderPath;
    using ProjectExplorer::HeaderPathType;

    for (const HeaderPath &headerPath : filter.userHeaderPaths)
        addIncludeDirOptionForPath(headerPath);
    for (const HeaderPath &headerPath : filter.systemHeaderPaths)
        addIncludeDirOptionForPath(headerPath);

    if (m_useTweakedHeaderPaths == UseTweakedHeaderPaths::Yes) {
        QTC_CHECK(!m_clangVersion.isEmpty()
                  && "Clang resource directory is required with UseTweakedHeaderPaths::Yes.");

        // Exclude all built-in includes and Clang resource directory.
        m_options.prepend("-nostdinc++");
        m_options.prepend("-nostdinc");

        for (const HeaderPath &headerPath : filter.builtInHeaderPaths)
            addIncludeDirOptionForPath(headerPath);
    }
}

void CompilerOptionsBuilder::addPrecompiledHeaderOptions(UsePrecompiledHeaders usePrecompiledHeaders)
{
    if (usePrecompiledHeaders == UsePrecompiledHeaders::No)
        return;

    for (const QString &pchFile : m_projectPart.precompiledHeaders) {
        if (QFile::exists(pchFile)) {
            add({isClStyle() ? QLatin1String(includeFileOptionCl)
                             : QLatin1String(includeFileOptionGcc),
                 QDir::toNativeSeparators(pchFile)});
        }
    }
}

void CompilerOptionsBuilder::addProjectMacros()
{
    addMacros(m_projectPart.projectMacros);
}

void CompilerOptionsBuilder::addMacros(const ProjectExplorer::Macros &macros)
{
    QStringList options;

    for (const ProjectExplorer::Macro &macro : macros) {
        if (excludeDefineDirective(macro))
            continue;

        const QString defineOption = defineDirectiveToDefineOption(macro);
        if (!options.contains(defineOption))
            options.append(defineOption);
    }

    add(options);
}

void CompilerOptionsBuilder::updateFileLanguage(ProjectFile::Kind fileKind)
{
    if (isClStyle()) {
        QString option;
        if (ProjectFile::isC(fileKind))
            option = "/TC";
        else if (ProjectFile::isCxx(fileKind))
            option = "/TP";
        else
            return; // Do not add anything if we haven't set a file kind yet.

        int langOptIndex = m_options.indexOf("/TC");
        if (langOptIndex == -1)
            langOptIndex = m_options.indexOf("/TP");
        if (langOptIndex == -1)
            add(option);
        else
            m_options[langOptIndex] = option;
        return;
    }

    const bool objcExt = m_projectPart.languageExtensions
                         & Utils::LanguageExtension::ObjectiveC;
    const QStringList options = createLanguageOptionGcc(fileKind, objcExt);
    if (options.isEmpty())
        return;

    QTC_ASSERT(options.size() == 2, return;);
    int langOptIndex = m_options.indexOf("-x");
    if (langOptIndex == -1)
        add(options);
    else
        m_options[langOptIndex + 1] = options[1];
}

void CompilerOptionsBuilder::addLanguageVersionAndExtensions()
{
    using Utils::LanguageExtension;
    using Utils::LanguageVersion;

    if (m_compilerFlags.isLanguageVersionSpecified)
        return;

    QString option;
    if (isClStyle()) {
        switch (m_projectPart.languageVersion) {
        default:
            break;
        case LanguageVersion::CXX14:
            option = "/std:c++14";
            break;
        case LanguageVersion::CXX17:
            option = "/std:c++17";
            break;
        case LanguageVersion::CXX2a:
            option = "/std:c++latest";
            break;
        }

        if (!option.isEmpty()) {
            add(option);
            return;
        }

        // Continue in case no cl-style option could be chosen.
    }

    const Utils::LanguageExtensions languageExtensions = m_projectPart.languageExtensions;
    const bool gnuExtensions = languageExtensions & LanguageExtension::Gnu;

    switch (m_projectPart.languageVersion) {
    case LanguageVersion::C89:
        option = (gnuExtensions ? QLatin1String("-std=gnu89") : QLatin1String("-std=c89"));
        break;
    case LanguageVersion::C99:
        option = (gnuExtensions ? QLatin1String("-std=gnu99") : QLatin1String("-std=c99"));
        break;
    case LanguageVersion::C11:
        option = (gnuExtensions ? QLatin1String("-std=gnu11") : QLatin1String("-std=c11"));
        break;
    case LanguageVersion::C18:
        // Clang 6, 7 and current trunk do not accept "gnu18"/"c18", so use the "*17" variants.
        option = (gnuExtensions ? QLatin1String("-std=gnu17") : QLatin1String("-std=c17"));
        break;
    case LanguageVersion::CXX11:
        option = (gnuExtensions ? QLatin1String("-std=gnu++11") : QLatin1String("-std=c++11"));
        break;
    case LanguageVersion::CXX98:
        option = (gnuExtensions ? QLatin1String("-std=gnu++98") : QLatin1String("-std=c++98"));
        break;
    case LanguageVersion::CXX03:
        option = (gnuExtensions ? QLatin1String("-std=gnu++03") : QLatin1String("-std=c++03"));
        break;
    case LanguageVersion::CXX14:
        option = (gnuExtensions ? QLatin1String("-std=gnu++14") : QLatin1String("-std=c++14"));
        break;
    case LanguageVersion::CXX17:
        option = (gnuExtensions ? QLatin1String("-std=gnu++17") : QLatin1String("-std=c++17"));
        break;
    case LanguageVersion::CXX2a:
        option = (gnuExtensions ? QLatin1String("-std=gnu++2a") : QLatin1String("-std=c++2a"));
        break;
    }

    add(option, /*gccOnlyOption=*/true);
}

static QByteArray toMsCompatibilityVersionFormat(const QByteArray &mscFullVer)
{
    return mscFullVer.left(2)
         + QByteArray(".")
         + mscFullVer.mid(2, 2);
}

static QByteArray msCompatibilityVersionFromDefines(const ProjectExplorer::Macros &macros)
{
    for (const ProjectExplorer::Macro &macro : macros) {
        if (macro.key == "_MSC_FULL_VER")
            return toMsCompatibilityVersionFormat(macro.value);
    }

    return QByteArray();
}

QByteArray CompilerOptionsBuilder::msvcVersion() const
{
    const QByteArray version = msCompatibilityVersionFromDefines(m_projectPart.toolChainMacros);
    return !version.isEmpty() ? version
                              : msCompatibilityVersionFromDefines(m_projectPart.projectMacros);
}

void CompilerOptionsBuilder::addMsvcCompatibilityVersion()
{
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID
        || m_projectPart.toolchainType == ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID) {
        const QByteArray msvcVer = msvcVersion();
        if (!msvcVer.isEmpty())
            add(QLatin1String("-fms-compatibility-version=") + msvcVer);
    }
}

static QStringList languageFeatureMacros()
{
    // CLANG-UPGRADE-CHECK: Update known language features macros.
    // Collected with the following command line.
    //   * Use latest -fms-compatibility-version and -std possible.
    //   * Compatibility version 19 vs 1910 did not matter.
    //  $ clang++ -fms-compatibility-version=19 -std=c++1z -dM -E D:\empty.cpp | grep __cpp_
    static const QStringList macros{
        "__cpp_aggregate_bases",
        "__cpp_aggregate_nsdmi",
        "__cpp_alias_templates",
        "__cpp_aligned_new",
        "__cpp_attributes",
        "__cpp_binary_literals",
        "__cpp_capture_star_this",
        "__cpp_constexpr",
        "__cpp_decltype",
        "__cpp_decltype_auto",
        "__cpp_deduction_guides",
        "__cpp_delegating_constructors",
        "__cpp_digit_separators",
        "__cpp_enumerator_attributes",
        "__cpp_exceptions",
        "__cpp_fold_expressions",
        "__cpp_generic_lambdas",
        "__cpp_guaranteed_copy_elision",
        "__cpp_hex_float",
        "__cpp_if_constexpr",
        "__cpp_inheriting_constructors",
        "__cpp_init_captures",
        "__cpp_initializer_lists",
        "__cpp_inline_variables",
        "__cpp_lambdas",
        "__cpp_namespace_attributes",
        "__cpp_nested_namespace_definitions",
        "__cpp_noexcept_function_type",
        "__cpp_nontype_template_args",
        "__cpp_nontype_template_parameter_auto",
        "__cpp_nsdmi",
        "__cpp_range_based_for",
        "__cpp_raw_strings",
        "__cpp_ref_qualifiers",
        "__cpp_return_type_deduction",
        "__cpp_rtti",
        "__cpp_rvalue_references",
        "__cpp_static_assert",
        "__cpp_structured_bindings",
        "__cpp_template_auto",
        "__cpp_threadsafe_static_init",
        "__cpp_unicode_characters",
        "__cpp_unicode_literals",
        "__cpp_user_defined_literals",
        "__cpp_variable_templates",
        "__cpp_variadic_templates",
        "__cpp_variadic_using",
    };

    return macros;
}

void CompilerOptionsBuilder::undefineCppLanguageFeatureMacrosForMsvc2015()
{
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID
            && m_projectPart.isMsvc2015Toolchain) {
        // Undefine the language feature macros that are pre-defined in clang-cl,
        // but not in MSVC's cl.exe.
        const QStringList macroNames = languageFeatureMacros();
        for (const QString &macroName : macroNames)
            add(undefineOption + macroName);
    }
}

void CompilerOptionsBuilder::addDefineFunctionMacrosMsvc()
{
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID) {
        addMacros({{"__FUNCSIG__", "\"\""},
                   {"__FUNCTION__", "\"\""},
                   {"__FUNCDNAME__", "\"\""}});
    }
}

void CompilerOptionsBuilder::addIncludeDirOptionForPath(const ProjectExplorer::HeaderPath &path)
{
    if (path.type == ProjectExplorer::HeaderPathType::Framework) {
        QTC_ASSERT(!isClStyle(), return;);
        add({"-F", QDir::toNativeSeparators(path.path)});
        return;
    }

    bool systemPath = false;
    if (path.type == ProjectExplorer::HeaderPathType::BuiltIn) {
        systemPath = true;
    } else if (path.type == ProjectExplorer::HeaderPathType::System) {
        if (m_useSystemHeader == UseSystemHeader::Yes)
            systemPath = true;
    } else {
        // ProjectExplorer::HeaderPathType::User
        if (m_useSystemHeader == UseSystemHeader::Yes
            && !path.path.startsWith(m_projectPart.project->rootProjectDirectory().toString())) {
            systemPath = true;
        }
    }

    if (systemPath) {
        add({includeSystemPathOption, QDir::toNativeSeparators(path.path)}, true);
        return;
    }

    add({includeUserPathOption, QDir::toNativeSeparators(path.path)});
}

bool CompilerOptionsBuilder::excludeDefineDirective(const ProjectExplorer::Macro &macro) const
{
    // Avoid setting __cplusplus & co as this might conflict with other command line flags.
    // Clang should set __cplusplus based on -std= and -fms-compatibility-version version.
    static const auto languageDefines = {"__cplusplus",
                                         "__STDC_VERSION__",
                                         "_MSC_BUILD",
                                         "_MSVC_LANG",
                                         "_MSC_FULL_VER",
                                         "_MSC_VER"};
    if (m_useLanguageDefines == UseLanguageDefines::No
            && std::find(languageDefines.begin(),
                         languageDefines.end(),
                         macro.key) != languageDefines.end()) {
        return true;
    }

    // Ignore for all compiler toolchains since LLVM has it's own implementation for
    // __has_include(STR) and __has_include_next(STR)
    if (macro.key.startsWith("__has_include"))
        return true;

    // If _FORTIFY_SOURCE is defined (typically in release mode), it will
    // enable the inclusion of extra headers to help catching buffer overflows
    // (e.g. wchar.h includes wchar2.h). These extra headers use
    // __builtin_va_arg_pack, which clang does not support (yet), so avoid
    // including those.
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID
            && macro.key == "_FORTIFY_SOURCE") {
        return true;
    }

    // MinGW 6 supports some fancy asm output flags and uses them in an
    // intrinsics header pulled in by windows.h. Clang does not know them.
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID
            && macro.key == "__GCC_ASM_FLAG_OUTPUTS__") {
        return true;
    }

    return false;
}

void CompilerOptionsBuilder::addWrappedQtHeadersIncludePath(QStringList &list) const
{
    static const QString resourcePath = creatorResourcePath();
    static QString wrappedQtHeadersPath = resourcePath + "/cplusplus/wrappedQtHeaders";
    QTC_ASSERT(QDir(wrappedQtHeadersPath).exists(), return;);

    if (m_projectPart.qtVersion != ProjectPart::NoQt) {
        const QString wrappedQtCoreHeaderPath = wrappedQtHeadersPath + "/QtCore";
        list.append({includeUserPathOption,
                     QDir::toNativeSeparators(wrappedQtHeadersPath),
                     includeUserPathOption,
                     QDir::toNativeSeparators(wrappedQtCoreHeaderPath)});
    }
}

void CompilerOptionsBuilder::addProjectConfigFileInclude()
{
    if (!m_projectPart.projectConfigFile.isEmpty()) {
        add({isClStyle() ? QLatin1String(includeFileOptionCl) : QLatin1String(includeFileOptionGcc),
             QDir::toNativeSeparators(m_projectPart.projectConfigFile)});
    }
}

void CompilerOptionsBuilder::undefineClangVersionMacrosForMsvc()
{
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID) {
        const QByteArray msvcVer = msvcVersion();
        if (msvcVer.toFloat() < 14.f) {
            // Original fix was only for msvc 2013 (version 12.0)
            // Undefying them for newer versions is not necessary and breaks boost.
            static const QStringList macroNames {
                "__clang__",
                "__clang_major__",
                "__clang_minor__",
                "__clang_patchlevel__",
                "__clang_version__"
            };

            for (const QString &macroName : macroNames)
                add(undefineOption + macroName);
        }
    }
}

void CompilerOptionsBuilder::reset()
{
    m_options.clear();
}

// Some example command lines for a "Qt Console Application":
//  CMakeProject: -fPIC -std=gnu++11
//  QbsProject: -m64 -fPIC -std=c++11 -fexceptions
//  QMakeProject: -pipe -Whello -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC
void CompilerOptionsBuilder::evaluateCompilerFlags()
{
    static QStringList userBlackList = QString::fromLocal8Bit(
                                           qgetenv("QTC_CLANG_CMD_OPTIONS_BLACKLIST"))
                                           .split(';', QString::SkipEmptyParts);

    bool containsDriverMode = false;
    bool skipNext = false;
    for (const QString &option : m_projectPart.compilerFlags) {
        if (skipNext) {
            skipNext = false;
            continue;
        }

        if (userBlackList.contains(option))
            continue;

        // Ignore warning flags as these interfere with our user-configured diagnostics.
        // Note that once "-w" is provided, no warnings will be emitted, even if "-Wall" follows.
        if (m_useBuildSystemWarnings == UseBuildSystemWarnings::No
            && (option.startsWith("-w", Qt::CaseInsensitive)
                || option.startsWith("/w", Qt::CaseInsensitive) || option.startsWith("-pedantic"))) {
            // -w, -W, /w, /W...
            continue;
        }

        if (option == includeUserPathOption || option == includeSystemPathOption
            || option == includeUserPathOptionWindows) {
            skipNext = true;
            continue;
        }

        if (option.startsWith("-O", Qt::CaseSensitive) || option.startsWith("/O", Qt::CaseSensitive)
            || option.startsWith("/M", Qt::CaseSensitive)
            || option.startsWith(includeUserPathOption)
            || option.startsWith(includeSystemPathOption)
            || option.startsWith(includeUserPathOptionWindows)) {
            // Optimization and run-time flags.
            continue;
        }

        if (option.startsWith("/Y", Qt::CaseSensitive)
            || (option.startsWith("/F", Qt::CaseSensitive) && option != "/F")) {
            // Precompiled header flags.
            // Skip also the next option if it's not glued to the current one.
            if (option.size() > 3)
                skipNext = true;
            continue;
        }

        // Check whether a language version is already used.
        QString theOption = option;
        if (theOption.startsWith("-std=")) {
            m_compilerFlags.isLanguageVersionSpecified = true;
            theOption.replace("=c18", "=c17");
            theOption.replace("=gnu18", "=gnu17");
        } else if (theOption.startsWith("/std:") || theOption.startsWith("-std:")) {
            m_compilerFlags.isLanguageVersionSpecified = true;
        }

        if (theOption.startsWith("--driver-mode=")) {
            if (theOption.endsWith("cl"))
                m_clStyle = true;
            containsDriverMode = true;
        }

        m_compilerFlags.flags.append(theOption);
    }

    const Core::Id &toolChain = m_projectPart.toolchainType;
    if (!containsDriverMode
        && (toolChain == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID
            || toolChain == ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID)) {
        m_clStyle = true;
        m_compilerFlags.flags.prepend("--driver-mode=cl");
    }
}

bool CompilerOptionsBuilder::isClStyle() const
{
    return m_clStyle;
}

} // namespace CppTools
