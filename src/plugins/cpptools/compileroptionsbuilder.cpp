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
#include <coreplugin/vcsmanager.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QRegularExpression>

namespace CppTools {

CompilerOptionsBuilder::CompilerOptionsBuilder(const ProjectPart &projectPart,
                                               UseSystemHeader useSystemHeader,
                                               SkipBuiltIn skipBuiltInHeaderPathsAndDefines,
                                               SkipLanguageDefines skipLanguageDefines,
                                               QString clangVersion,
                                               QString clangResourceDirectory)
    : m_projectPart(projectPart)
    , m_clangVersion(clangVersion)
    , m_clangResourceDirectory(clangResourceDirectory)
    , m_useSystemHeader(useSystemHeader)
    , m_skipBuiltInHeaderPathsAndDefines(skipBuiltInHeaderPathsAndDefines)
    , m_skipLanguageDefines(skipLanguageDefines)
{
}

QStringList CompilerOptionsBuilder::build(CppTools::ProjectFile::Kind fileKind, PchUsage pchUsage)
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
    addPrecompiledHeaderOptions(pchUsage);
    addHeaderPathOptions();
    addProjectConfigFileInclude();

    addMsvcCompatibilityVersion();

    addExtraOptions();

    insertWrappedQtHeaders();

    return options();
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
        }
        Q_FALLTHROUGH();
    case ProjectFile::ObjCHeader:
    case ProjectFile::ObjCXXHeader:
        opts += QLatin1String("objective-c++-header");
        break;

    case ProjectFile::CSource:
        if (!objcExt) {
            opts += QLatin1String("c");
            break;
        }
        Q_FALLTHROUGH();
    case ProjectFile::ObjCSource:
        opts += QLatin1String("objective-c");
        break;

    case ProjectFile::CXXSource:
        if (!objcExt) {
            opts += QLatin1String("c++");
            break;
        }
        Q_FALLTHROUGH();
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

QStringList CompilerOptionsBuilder::options() const
{
    return m_options;
}

void CompilerOptionsBuilder::add(const QString &option)
{
    m_options.append(option);
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
        add(QLatin1String("-fcxx-exceptions"));
    add(QLatin1String("-fexceptions"));
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
    return QDir::toNativeSeparators(QString::fromUtf8(CLANG_RESOURCE_DIR ""));
#endif
}

static QStringList insertResourceDirectory(const QStringList &options,
                                           const QString &resourceDir,
                                           bool isMacOs = false)
{
    // include/c++, include/g++, libc++\include and libc++abi\include
    static const QString cppIncludes = R"((.*[\/\\]include[\/\\].*(g\+\+|c\+\+).*))"
                                       R"(|(.*libc\+\+[\/\\]include))"
                                       R"(|(.*libc\+\+abi[\/\\]include))";

    QStringList optionsBeforeResourceDirectory;
    QStringList optionsAfterResourceDirectory;
    QRegularExpression includeRegExp;
    if (!isMacOs) {
        includeRegExp = QRegularExpression("\\A(" + cppIncludes + ")\\z");
    } else {
        // The same as includeRegExp but also matches /usr/local/include
        includeRegExp = QRegularExpression(
            "\\A(" + cppIncludes + R"(|([\/\\]usr[\/\\]local[\/\\]include))" + ")\\z");
    }

    for (const QString &option : options) {
        if (option == "-isystem")
            continue;

        if (includeRegExp.match(option).hasMatch()) {
            optionsBeforeResourceDirectory.push_back("-isystem");
            optionsBeforeResourceDirectory.push_back(option);
        } else {
            optionsAfterResourceDirectory.push_back("-isystem");
            optionsAfterResourceDirectory.push_back(option);
        }
    }

    optionsBeforeResourceDirectory.push_back("-isystem");
    optionsBeforeResourceDirectory.push_back(resourceDir);

    return optionsBeforeResourceDirectory + optionsAfterResourceDirectory;
}

void CompilerOptionsBuilder::insertWrappedQtHeaders()
{
    if (m_skipBuiltInHeaderPathsAndDefines == SkipBuiltIn::Yes)
        return;

    QStringList wrappedQtHeaders;
    addWrappedQtHeadersIncludePath(wrappedQtHeaders);

    const int index = m_options.indexOf(QRegularExpression("\\A-I.*\\z"));
    if (index < 0)
        m_options.append(wrappedQtHeaders);
    else
        m_options = m_options.mid(0, index) + wrappedQtHeaders + m_options.mid(index);
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
        default: // This shouldn't happen, but let's be nice..:
            // intentional fall-through:
        case HeaderPathType::User:
            includes.append(includeDirOptionForPath(headerPath.path));
            includes.append(QDir::toNativeSeparators(headerPath.path));
            break;
        case HeaderPathType::BuiltIn:
            builtInIncludes.append("-isystem");
            builtInIncludes.append(QDir::toNativeSeparators(headerPath.path));
            break;
        case HeaderPathType::System:
            systemIncludes.append(m_useSystemHeader == UseSystemHeader::No
                                  ? QLatin1String("-I")
                                  : QLatin1String("-isystem"));
            systemIncludes.append(QDir::toNativeSeparators(headerPath.path));
            break;
        }
    }

    m_options.append(includes);
    m_options.append(systemIncludes);

    if (m_skipBuiltInHeaderPathsAndDefines == SkipBuiltIn::Yes)
        return;

    // Exclude all built-in includes except Clang resource directory.
    m_options.prepend("-nostdlibinc");

    if (!m_clangVersion.isEmpty()) {
        // Exclude all built-in includes and Clang resource directory.
        m_options.prepend("-nostdinc");

        const QString clangIncludePath
                = clangIncludeDirectory(m_clangVersion, m_clangResourceDirectory);

        builtInIncludes = insertResourceDirectory(builtInIncludes,
                                                  clangIncludePath,
                                                  m_projectPart.toolChainTargetTriple.contains(
                                                      "darwin"));
    }

    m_options.append(builtInIncludes);
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

void CompilerOptionsBuilder::addToolchainAndProjectMacros()
{
    if (m_skipBuiltInHeaderPathsAndDefines == SkipBuiltIn::No)
        addMacros(m_projectPart.toolChainMacros);
    addMacros(m_projectPart.projectMacros);
}

void CompilerOptionsBuilder::addMacros(const ProjectExplorer::Macros &macros)
{
    QStringList result;

    for (const ProjectExplorer::Macro &macro : macros) {
        if (excludeDefineDirective(macro))
            continue;

        const QString defineOption = defineDirectiveToDefineOption(macro);
        if (!result.contains(defineOption))
            result.append(defineOption);
    }

    m_options.append(result);
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
    if (langOptIndex == -1) {
        m_options.append(options);
    } else {
        m_options[langOptIndex + 1] = options[1];
    }
}

void CompilerOptionsBuilder::addOptionsForLanguage(bool checkForBorlandExtensions)
{
    using ProjectExplorer::LanguageExtension;
    using ProjectExplorer::LanguageVersion;

    QStringList opts;
    const ProjectExplorer::LanguageExtensions languageExtensions = m_projectPart.languageExtensions;
    const bool gnuExtensions = languageExtensions & LanguageExtension::Gnu;

    switch (m_projectPart.languageVersion) {
    case LanguageVersion::C89:
        opts << (gnuExtensions ? QLatin1String("-std=gnu89") : QLatin1String("-std=c89"));
        break;
    case LanguageVersion::C99:
        opts << (gnuExtensions ? QLatin1String("-std=gnu99") : QLatin1String("-std=c99"));
        break;
    case LanguageVersion::C11:
        opts << (gnuExtensions ? QLatin1String("-std=gnu11") : QLatin1String("-std=c11"));
        break;
    case LanguageVersion::C18:
        // Clang 6, 7 and current trunk do not accept "gnu18"/"c18", so use the "*17" variants.
        opts << (gnuExtensions ? QLatin1String("-std=gnu17") : QLatin1String("-std=c17"));
        break;
    case LanguageVersion::CXX11:
        opts << (gnuExtensions ? QLatin1String("-std=gnu++11") : QLatin1String("-std=c++11"));
        break;
    case LanguageVersion::CXX98:
        opts << (gnuExtensions ? QLatin1String("-std=gnu++98") : QLatin1String("-std=c++98"));
        break;
    case LanguageVersion::CXX03:
        opts << (gnuExtensions ? QLatin1String("-std=gnu++03") : QLatin1String("-std=c++03"));
        break;
    case LanguageVersion::CXX14:
        opts << (gnuExtensions ? QLatin1String("-std=gnu++14") : QLatin1String("-std=c++14"));
        break;
    case LanguageVersion::CXX17:
        opts << (gnuExtensions ? QLatin1String("-std=gnu++17") : QLatin1String("-std=c++17"));
        break;
    case LanguageVersion::CXX2a:
        opts << (gnuExtensions ? QLatin1String("-std=gnu++2a") : QLatin1String("-std=c++2a"));
        break;
    }

    if (languageExtensions & LanguageExtension::Microsoft)
        opts << QLatin1String("-fms-extensions");

    if (languageExtensions & LanguageExtension::OpenMP)
        opts << QLatin1String("-fopenmp");

    if (checkForBorlandExtensions && (languageExtensions & LanguageExtension::Borland))
        opts << QLatin1String("-fborland-extensions");

    m_options.append(opts);
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
    static QStringList macros{
        QLatin1String("__cpp_aggregate_bases"),
        QLatin1String("__cpp_aggregate_nsdmi"),
        QLatin1String("__cpp_alias_templates"),
        QLatin1String("__cpp_aligned_new"),
        QLatin1String("__cpp_attributes"),
        QLatin1String("__cpp_binary_literals"),
        QLatin1String("__cpp_capture_star_this"),
        QLatin1String("__cpp_constexpr"),
        QLatin1String("__cpp_decltype"),
        QLatin1String("__cpp_decltype_auto"),
        QLatin1String("__cpp_deduction_guides"),
        QLatin1String("__cpp_delegating_constructors"),
        QLatin1String("__cpp_digit_separators"),
        QLatin1String("__cpp_enumerator_attributes"),
        QLatin1String("__cpp_exceptions"),
        QLatin1String("__cpp_fold_expressions"),
        QLatin1String("__cpp_generic_lambdas"),
        QLatin1String("__cpp_guaranteed_copy_elision"),
        QLatin1String("__cpp_hex_float"),
        QLatin1String("__cpp_if_constexpr"),
        QLatin1String("__cpp_inheriting_constructors"),
        QLatin1String("__cpp_init_captures"),
        QLatin1String("__cpp_initializer_lists"),
        QLatin1String("__cpp_inline_variables"),
        QLatin1String("__cpp_lambdas"),
        QLatin1String("__cpp_namespace_attributes"),
        QLatin1String("__cpp_nested_namespace_definitions"),
        QLatin1String("__cpp_noexcept_function_type"),
        QLatin1String("__cpp_nontype_template_args"),
        QLatin1String("__cpp_nontype_template_parameter_auto"),
        QLatin1String("__cpp_nsdmi"),
        QLatin1String("__cpp_range_based_for"),
        QLatin1String("__cpp_raw_strings"),
        QLatin1String("__cpp_ref_qualifiers"),
        QLatin1String("__cpp_return_type_deduction"),
        QLatin1String("__cpp_rtti"),
        QLatin1String("__cpp_rvalue_references"),
        QLatin1String("__cpp_static_assert"),
        QLatin1String("__cpp_structured_bindings"),
        QLatin1String("__cpp_template_auto"),
        QLatin1String("__cpp_threadsafe_static_init"),
        QLatin1String("__cpp_unicode_characters"),
        QLatin1String("__cpp_unicode_literals"),
        QLatin1String("__cpp_user_defined_literals"),
        QLatin1String("__cpp_variable_templates"),
        QLatin1String("__cpp_variadic_templates"),
        QLatin1String("__cpp_variadic_using"),
    };

    return macros;
}

void CompilerOptionsBuilder::undefineCppLanguageFeatureMacrosForMsvc2015()
{
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID
            && m_projectPart.isMsvc2015Toolchain) {
        // Undefine the language feature macros that are pre-defined in clang-cl,
        // but not in MSVC's cl.exe.
        foreach (const QString &macroName, languageFeatureMacros())
            m_options.append(undefineOption() + macroName);
    }
}

void CompilerOptionsBuilder::addDefineFunctionMacrosMsvc()
{
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID)
        addMacros({{"__FUNCSIG__", "\"\""}, {"__FUNCTION__", "\"\""}, {"__FUNCDNAME__", "\"\""}});
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
        return QString("-I");
    } else {
        return QString("-isystem");
    }
}

QByteArray CompilerOptionsBuilder::macroOption(const ProjectExplorer::Macro &macro) const
{
    switch (macro.type) {
        case ProjectExplorer::MacroType::Define:     return defineOption().toUtf8();
        case ProjectExplorer::MacroType::Undefine:   return undefineOption().toUtf8();
        default: return QByteArray();
    }
}

QByteArray CompilerOptionsBuilder::toDefineOption(const ProjectExplorer::Macro &macro) const
{
    return macro.toKeyValue(macroOption(macro));
}

QString CompilerOptionsBuilder::defineDirectiveToDefineOption(const ProjectExplorer::Macro &macro) const
{
    const QByteArray option = toDefineOption(macro);

    return QString::fromUtf8(option);
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
    if (m_skipLanguageDefines == SkipLanguageDefines::Yes
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

bool CompilerOptionsBuilder::excludeHeaderPath(const QString &headerPath) const
{
    // Always exclude clang system includes (including intrinsics) which do not come with libclang
    // that Qt Creator uses for code model.
    // For example GCC on macOS uses system clang include path which makes clang code model
    // include incorrect system headers.
    static QRegularExpression clangIncludeDir(
                QLatin1String("\\A.*[\\/\\\\]lib\\d*[\\/\\\\]clang[\\/\\\\]\\d+\\.\\d+(\\.\\d+)?[\\/\\\\]include\\z"));
    return clangIncludeDir.match(headerPath).hasMatch();
}

void CompilerOptionsBuilder::addWrappedQtHeadersIncludePath(QStringList &list)
{
    static const QString resourcePath = creatorResourcePath();
    static QString wrappedQtHeadersPath = resourcePath + "/cplusplus/wrappedQtHeaders";
    QTC_ASSERT(QDir(wrappedQtHeadersPath).exists(), return;);

    if (m_projectPart.qtVersion != CppTools::ProjectPart::NoQt) {
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
        if (m_skipBuiltInHeaderPathsAndDefines == SkipBuiltIn::No)
            add("-undef");
        else
            add("-fPIC");
    }
}

void CompilerOptionsBuilder::addProjectConfigFileInclude()
{
    if (!m_projectPart.projectConfigFile.isEmpty()) {
        add("-include");
        add(QDir::toNativeSeparators(m_projectPart.projectConfigFile));
    }
}

void CompilerOptionsBuilder::undefineClangVersionMacrosForMsvc()
{
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID) {
        const QByteArray msvcVer = msvcVersion();
        if (msvcVer.toFloat() < 14.f) {
            // Original fix was only for msvc 2013 (version 12.0)
            // Undefying them for newer versions is not necessary and breaks boost.
            static QStringList macroNames {
                "__clang__",
                "__clang_major__",
                "__clang_minor__",
                "__clang_patchlevel__",
                "__clang_version__"
            };

            foreach (const QString &macroName, macroNames)
                add(undefineOption() + macroName);
        }
    }
}

} // namespace CppTools
