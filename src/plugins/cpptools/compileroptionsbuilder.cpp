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

#include <coreplugin/icore.h>

#include <projectexplorer/headerpath.h>
#include <projectexplorer/language.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacro.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QRegularExpression>

namespace CppTools {

static const char defineOption[] = "-D";
static const char undefineOption[] = "-U";

static const char includeUserPathOption[] = "-I";
static const char includeSystemPathOption[] = "-isystem";

static const char includeFileOption[] = "-include";

static QByteArray macroOption(const ProjectExplorer::Macro &macro)
{
    switch (macro.type) {
        case ProjectExplorer::MacroType::Define:   return defineOption;
        case ProjectExplorer::MacroType::Undefine: return undefineOption;
        default: return QByteArray();
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

CompilerOptionsBuilder::CompilerOptionsBuilder(const ProjectPart &projectPart,
                                               UseSystemHeader useSystemHeader,
                                               UseToolchainMacros useToolchainMacros,
                                               UseTweakedHeaderPaths useTweakedHeaderPaths,
                                               UseLanguageDefines useLanguageDefines,
                                               const QString &clangVersion,
                                               const QString &clangResourceDirectory)
    : m_projectPart(projectPart)
    , m_useSystemHeader(useSystemHeader)
    , m_useToolchainMacros(useToolchainMacros)
    , m_useTweakedHeaderPaths(useTweakedHeaderPaths)
    , m_useLanguageDefines(useLanguageDefines)
    , m_clangVersion(clangVersion)
    , m_clangResourceDirectory(clangResourceDirectory)
{
}

QStringList CompilerOptionsBuilder::build(ProjectFile::Kind fileKind,
                                          UsePrecompiledHeaders usePrecompiledHeaders)
{
    m_options.clear();

    if (fileKind == ProjectFile::CHeader || fileKind == ProjectFile::CSource) {
        QTC_ASSERT(m_projectPart.languageVersion <= ProjectExplorer::LanguageVersion::LatestC,
                   return QStringList(););
    }

    if (fileKind == ProjectFile::CXXHeader || fileKind == ProjectFile::CXXSource) {
        QTC_ASSERT(m_projectPart.languageVersion > ProjectExplorer::LanguageVersion::LatestC,
                   return QStringList(););
    }

    add("-c");

    addWordWidth();
    addTargetTriple();
    addExtraCodeModelFlags();

    updateLanguageOption(fileKind);
    addOptionsForLanguage(/*checkForBorlandExtensions*/ true);
    enableExceptions();

    addToolchainAndProjectMacros();
    undefineClangVersionMacrosForMsvc();
    undefineCppLanguageFeatureMacrosForMsvc2015();
    addDefineFunctionMacrosMsvc();
    addBoostWorkaroundMacros();

    addToolchainFlags();
    addPrecompiledHeaderOptions(usePrecompiledHeaders);
    addHeaderPathOptions();
    addProjectConfigFileInclude();

    addMsvcCompatibilityVersion();

    addExtraOptions();

    insertWrappedQtHeaders();

    return options();
}

static QStringList createLanguageOptionGcc(ProjectFile::Kind fileKind, bool objcExt)
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
    if (!m_projectPart.toolChainTargetTriple.isEmpty()) {
        add("-target");
        add(m_projectPart.toolChainTargetTriple);
    }
}

void CompilerOptionsBuilder::addExtraCodeModelFlags()
{
    // extraCodeModelFlags keep build architecture for cross-compilation.
    // In case of iOS build target triple has aarch64 archtecture set which makes
    // code model fail with CXError_Failure. To fix that we explicitly provide architecture.
    m_options.append(m_projectPart.extraCodeModelFlags);
}

void CompilerOptionsBuilder::enableExceptions()
{
    if (m_projectPart.languageVersion > ProjectExplorer::LanguageVersion::LatestC)
        add("-fcxx-exceptions");
    add("-fexceptions");
}

static QString creatorResourcePath()
{
#ifndef UNIT_TESTS
    return Core::ICore::resourcePath();
#else
    return QDir::toNativeSeparators(QString::fromUtf8(QTC_RESOURCE_DIR ""));
#endif
}

static QString clangIncludeDirectory(const QString &clangVersion,
                                     const QString &clangResourceDirectory)
{
#ifndef UNIT_TESTS
    return Core::ICore::clangIncludeDirectory(clangVersion, clangResourceDirectory);
#else
    Q_UNUSED(clangVersion);
    Q_UNUSED(clangResourceDirectory);
    return QDir::toNativeSeparators(QString::fromUtf8(CLANG_RESOURCE_DIR ""));
#endif
}

static int lastIncludeIndex(const QStringList &options, const QRegularExpression &includePathRegEx)
{
    int index = options.lastIndexOf(includePathRegEx);

    while (index > 0 && options[index - 1] != includeUserPathOption
           && options[index - 1] != includeSystemPathOption) {
        index = options.lastIndexOf(includePathRegEx, index - 1);
    }

    if (index == 0)
        index = -1;

    return index;
}

static int includeIndexForResourceDirectory(const QStringList &options, bool isMacOs = false)
{
    // include/c++, include/g++, libc++\include and libc++abi\include
    static const QString cppIncludes = R"((.*[\/\\]include[\/\\].*(g\+\+|c\+\+).*))"
                                       R"(|(.*libc\+\+[\/\\]include))"
                                       R"(|(.*libc\+\+abi[\/\\]include))";
    static const QRegularExpression includeRegExp("\\A(" + cppIncludes + ")\\z");

    // The same as includeRegExp but also matches /usr/local/include
    static const QRegularExpression includeRegExpMac(
        "\\A(" + cppIncludes + R"(|([\/\\]usr[\/\\]local[\/\\]include))" + ")\\z");

    const int cppIncludeIndex = lastIncludeIndex(options,
                                                 isMacOs ? includeRegExpMac : includeRegExp);

    if (cppIncludeIndex > 0)
        return cppIncludeIndex + 1;

    return -1;
}

void CompilerOptionsBuilder::insertWrappedQtHeaders()
{
    if (m_useTweakedHeaderPaths == UseTweakedHeaderPaths::No)
        return;

    QStringList wrappedQtHeaders;
    addWrappedQtHeadersIncludePath(wrappedQtHeaders);

    const int index = m_options.indexOf(QRegularExpression("\\A-I.*\\z"));
    if (index < 0)
        m_options.append(wrappedQtHeaders);
    else
        m_options = m_options.mid(0, index) + wrappedQtHeaders + m_options.mid(index);
}

static bool excludeHeaderPath(const QString &headerPath)
{
    // Always exclude clang system includes (including intrinsics) which do not come with libclang
    // that Qt Creator uses for code model.
    // For example GCC on macOS uses system clang include path which makes clang code model
    // include incorrect system headers.
    static const QRegularExpression clangIncludeDir(
        R"(\A.*[\/\\]lib\d*[\/\\]clang[\/\\]\d+\.\d+(\.\d+)?[\/\\]include\z)");
    return clangIncludeDir.match(headerPath).hasMatch();
}

void CompilerOptionsBuilder::addHeaderPathOptions()
{
    using ProjectExplorer::HeaderPathType;

    QStringList includes;
    QStringList systemIncludes;
    QStringList builtInIncludes;

    for (const ProjectExplorer::HeaderPath &headerPath : qAsConst(m_projectPart.headerPaths)) {
        if (headerPath.path.isEmpty())
            continue;

        if (excludeHeaderPath(headerPath.path))
            continue;

        switch (headerPath.type) {
        case HeaderPathType::Framework:
            includes.append("-F");
            includes.append(QDir::toNativeSeparators(headerPath.path));
            break;
        case HeaderPathType::User:
            includes.append(includeDirOptionForPath(headerPath.path));
            includes.append(QDir::toNativeSeparators(headerPath.path));
            break;
        case HeaderPathType::BuiltIn:
            builtInIncludes.append(includeSystemPathOption);
            builtInIncludes.append(QDir::toNativeSeparators(headerPath.path));
            break;
        case HeaderPathType::System:
            systemIncludes.append(m_useSystemHeader == UseSystemHeader::Yes
                                      ? QLatin1String(includeSystemPathOption)
                                      : QLatin1String(includeUserPathOption));
            systemIncludes.append(QDir::toNativeSeparators(headerPath.path));
            break;
        }
    }

    m_options.append(includes);
    m_options.append(systemIncludes);

    if (m_useTweakedHeaderPaths == UseTweakedHeaderPaths::No)
        return;

    // Exclude all built-in includes except Clang resource directory.
    m_options.prepend("-nostdlibinc");

    if (!m_clangVersion.isEmpty()) {
        // Exclude all built-in includes and Clang resource directory.
        m_options.prepend("-nostdinc");

        const QString clangIncludePath = clangIncludeDirectory(m_clangVersion,
                                                               m_clangResourceDirectory);
        const int includeIndexForResourceDir
            = includeIndexForResourceDirectory(builtInIncludes,
                                               m_projectPart.toolChainTargetTriple.contains(
                                                   "darwin"));

        if (includeIndexForResourceDir >= 0) {
            builtInIncludes.insert(includeIndexForResourceDir, clangIncludePath);
            builtInIncludes.insert(includeIndexForResourceDir, includeSystemPathOption);
        } else {
            builtInIncludes.prepend(clangIncludePath);
            builtInIncludes.prepend(includeSystemPathOption);
        }
    }

    m_options.append(builtInIncludes);
}

void CompilerOptionsBuilder::addPrecompiledHeaderOptions(UsePrecompiledHeaders usePrecompiledHeaders)
{
    if (usePrecompiledHeaders == UsePrecompiledHeaders::No)
        return;

    QStringList options;

    for (const QString &pchFile : m_projectPart.precompiledHeaders) {
        if (QFile::exists(pchFile)) {
            options += includeFileOption;
            options += QDir::toNativeSeparators(pchFile);
        }
    }

    m_options.append(options);
}

void CompilerOptionsBuilder::addToolchainAndProjectMacros()
{
    if (m_useToolchainMacros == UseToolchainMacros::Yes)
        addMacros(m_projectPart.toolChainMacros);
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

    m_options.append(options);
}

void CompilerOptionsBuilder::updateLanguageOption(ProjectFile::Kind fileKind)
{
    const bool objcExt = m_projectPart.languageExtensions
                         & ProjectExplorer::LanguageExtension::ObjectiveC;
    const QStringList options = createLanguageOptionGcc(fileKind, objcExt);
    if (options.isEmpty())
        return;

    QTC_ASSERT(options.size() == 2, return;);
    int langOptIndex = m_options.indexOf("-x");
    if (langOptIndex == -1)
        m_options.append(options);
    else
        m_options[langOptIndex + 1] = options[1];
}

void CompilerOptionsBuilder::addOptionsForLanguage(bool checkForBorlandExtensions)
{
    using ProjectExplorer::LanguageExtension;
    using ProjectExplorer::LanguageVersion;

    QStringList options;
    const ProjectExplorer::LanguageExtensions languageExtensions = m_projectPart.languageExtensions;
    const bool gnuExtensions = languageExtensions & LanguageExtension::Gnu;

    switch (m_projectPart.languageVersion) {
    case LanguageVersion::C89:
        options << (gnuExtensions ? QLatin1String("-std=gnu89") : QLatin1String("-std=c89"));
        break;
    case LanguageVersion::C99:
        options << (gnuExtensions ? QLatin1String("-std=gnu99") : QLatin1String("-std=c99"));
        break;
    case LanguageVersion::C11:
        options << (gnuExtensions ? QLatin1String("-std=gnu11") : QLatin1String("-std=c11"));
        break;
    case LanguageVersion::C18:
        // Clang 6, 7 and current trunk do not accept "gnu18"/"c18", so use the "*17" variants.
        options << (gnuExtensions ? QLatin1String("-std=gnu17") : QLatin1String("-std=c17"));
        break;
    case LanguageVersion::CXX11:
        options << (gnuExtensions ? QLatin1String("-std=gnu++11") : QLatin1String("-std=c++11"));
        break;
    case LanguageVersion::CXX98:
        options << (gnuExtensions ? QLatin1String("-std=gnu++98") : QLatin1String("-std=c++98"));
        break;
    case LanguageVersion::CXX03:
        options << (gnuExtensions ? QLatin1String("-std=gnu++03") : QLatin1String("-std=c++03"));
        break;
    case LanguageVersion::CXX14:
        options << (gnuExtensions ? QLatin1String("-std=gnu++14") : QLatin1String("-std=c++14"));
        break;
    case LanguageVersion::CXX17:
        options << (gnuExtensions ? QLatin1String("-std=gnu++17") : QLatin1String("-std=c++17"));
        break;
    case LanguageVersion::CXX2a:
        options << (gnuExtensions ? QLatin1String("-std=gnu++2a") : QLatin1String("-std=c++2a"));
        break;
    }

    if (languageExtensions & LanguageExtension::Microsoft)
        options << "-fms-extensions";

    if (languageExtensions & LanguageExtension::OpenMP)
        options << "-fopenmp";

    if (checkForBorlandExtensions && (languageExtensions & LanguageExtension::Borland))
        options << "-fborland-extensions";

    m_options.append(options);
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

void CompilerOptionsBuilder::addMsvcCompatibilityVersion()
{
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID) {
        const ProjectExplorer::Macros macros = m_projectPart.toolChainMacros
                                               + m_projectPart.projectMacros;
        const QByteArray msvcVersion = msCompatibilityVersionFromDefines(macros);

        if (!msvcVersion.isEmpty())
            add(QLatin1String("-fms-compatibility-version=") + msvcVersion);
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

void CompilerOptionsBuilder::addBoostWorkaroundMacros()
{
    if (m_projectPart.toolchainType != ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID
          && m_projectPart.toolchainType != ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID) {
        addMacros({{"BOOST_TYPE_INDEX_CTTI_USER_DEFINED_PARSING", "(39, 1, true, \"T = \")"}});
    }
}

QString CompilerOptionsBuilder::includeDirOptionForPath(const QString &path) const
{
    if (m_useSystemHeader == UseSystemHeader::No
            || path.startsWith(m_projectPart.project->rootProjectDirectory().toString())) {
        return QLatin1String(includeUserPathOption);
    }

    return QLatin1String(includeSystemPathOption);
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
        list.append(includeDirOptionForPath(wrappedQtHeadersPath));
        list.append(QDir::toNativeSeparators(wrappedQtHeadersPath));
        list.append(includeDirOptionForPath(wrappedQtHeadersPath));
        list.append(QDir::toNativeSeparators(wrappedQtCoreHeaderPath));
    }
}

void CompilerOptionsBuilder::addToolchainFlags()
{
    // In case of MSVC we need builtin clang defines to correctly handle clang includes
    if (m_projectPart.toolchainType != ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID
            && m_projectPart.toolchainType != ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID) {
        if (m_useToolchainMacros == UseToolchainMacros::Yes)
            add("-undef");
        else
            add("-fPIC");
    }
}

void CompilerOptionsBuilder::addProjectConfigFileInclude()
{
    if (!m_projectPart.projectConfigFile.isEmpty()) {
        add(includeFileOption);
        add(QDir::toNativeSeparators(m_projectPart.projectConfigFile));
    }
}

void CompilerOptionsBuilder::undefineClangVersionMacrosForMsvc()
{
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID) {
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

} // namespace CppTools
