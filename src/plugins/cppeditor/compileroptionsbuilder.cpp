// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compileroptionsbuilder.h"

#include "cppmodelmanager.h"
#include "headerpathfilter.h"

#include <coreplugin/icore.h>

#include <projectexplorer/headerpath.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacro.h>

#include <qnx/qnxconstants.h>

#include <utils/algorithm.h>
#include <utils/cpplanguage_details.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QDir>
#include <QRegularExpression>
#include <QtGlobal>

using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor {

const char defineOption[] = "-D";
const char undefineOption[] = "-U";

const char includeUserPathOption[] = "-I";
const char includeUserPathOptionWindows[] = "/I";
const char includeSystemPathOption[] = "-isystem";

const char includeFileOptionGcc[] = "-include";
const char includeFileOptionCl[] = "/FI";

static QByteArray macroOption(const Macro &macro)
{
    switch (macro.type) {
    case MacroType::Define:
        return defineOption;
    case MacroType::Undefine:
        return undefineOption;
    default:
        return QByteArray();
    }
}

static QByteArray toDefineOption(const Macro &macro)
{
    return macro.toKeyValue(macroOption(macro));
}

static QString defineDirectiveToDefineOption(const Macro &macro)
{
    const QByteArray option = toDefineOption(macro);
    return QString::fromUtf8(option);
}

static QStringList cpuBlacklist()
{
    QStringList blacklist = qtcEnvironmentVariable("QTC_CLANGD_CPU_BLACKLIST")
                                .split(':', Qt::SkipEmptyParts);
    return blacklist << "cortex-a72.cortex-a53"; // See QTCREATORBUG-29304
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
                                               const FilePath &clangIncludeDirectory)
    : m_projectPart(projectPart)
    , m_useSystemHeader(useSystemHeader)
    , m_useTweakedHeaderPaths(useTweakedHeaderPaths)
    , m_useLanguageDefines(useLanguageDefines)
    , m_useBuildSystemWarnings(useBuildSystemWarnings)
    , m_clangIncludeDirectory(clangIncludeDirectory)
{
}

QStringList CompilerOptionsBuilder::build(ProjectFile::Kind fileKind,
                                          UsePrecompiledHeaders usePrecompiledHeaders)
{
    reset();
    evaluateCompilerFlags();

    if (fileKind == ProjectFile::CHeader || fileKind == ProjectFile::CSource) {
        QTC_ASSERT(m_projectPart.languageVersion <= LanguageVersion::LatestC, return {});
    }

    if (fileKind == ProjectFile::CXXHeader || fileKind == ProjectFile::CXXSource) {
        QTC_ASSERT(m_projectPart.languageVersion > LanguageVersion::LatestC, return {});
    }

    addCompilerFlags();

    addSyntaxOnly();
    addWordWidth();
    addTargetTriple();
    updateFileLanguage(fileKind);
    addLanguageVersionAndExtensions();
    addMsvcExceptions();

    addIncludedFiles(m_projectPart.includedFiles); // GCC adds these before precompiled headers.
    addPrecompiledHeaderOptions(usePrecompiledHeaders);
    addProjectConfigFileInclude();

    addMsvcCompatibilityVersion();
    addProjectMacros();
    undefineClangVersionMacrosForMsvc();
    undefineCppLanguageFeatureMacrosForMsvc2015();
    addDefineFunctionMacrosMsvc();
    addDefineFunctionMacrosQnx();
    addQtMacros();

    addHeaderPathOptions();

    insertWrappedQtHeaders();
    insertWrappedMingwHeaders();

    return options();
}

void CompilerOptionsBuilder::provideAdditionalMacros(const ProjectExplorer::Macros &macros)
{
    m_additionalMacros = macros;
}

void CompilerOptionsBuilder::add(const QString &arg, bool gccOnlyOption)
{
    add(QStringList{arg}, gccOnlyOption);
}

void CompilerOptionsBuilder::prepend(const QString &arg)
{
    m_options.prepend(arg);
}

void CompilerOptionsBuilder::add(const QStringList &args, bool gccOnlyOptions)
{
    m_options.append((gccOnlyOptions && isClStyle()) ? clangArgsForCl(args) : args);
}

void CompilerOptionsBuilder::addSyntaxOnly()
{
    if (m_nativeMode)
        return;

    isClStyle() ? add("/Zs") : add("-fsyntax-only");
}

QStringList createLanguageOptionGcc(Language language, ProjectFile::Kind fileKind, bool objcExt)
{
    QStringList options;

    switch (fileKind) {
    case ProjectFile::Unclassified:
    case ProjectFile::Unsupported:
        break;
    case ProjectFile::CHeader:
    case ProjectFile::AmbiguousHeader:
        if (language == Language::C)
            options += QLatin1String(objcExt ? "objective-c-header" : "c-header");
        else if (language == Language::Cxx)
            options += QLatin1String(objcExt ? "objective-c++-header" : "c++-header");
        break;
    case ProjectFile::CXXHeader:
        options += QLatin1String(objcExt ? "objective-c++-header" : "c++-header");
        break;
    case ProjectFile::ObjCHeader:
        options += QLatin1String(language == Language::C ? "objective-c-header"
                                                         : "objective-c++-header");
        break;
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
    if (m_projectPart.toolChainAbi.architecture() != Abi::X86Architecture)
        return;
    const QString argument = m_projectPart.toolChainAbi.wordWidth() == 64
                                 ? QLatin1String("-m64")
                                 : QLatin1String("-m32");
    add(argument);
}

void CompilerOptionsBuilder::addTargetTriple()
{
    if (m_nativeMode && m_projectPart.toolchainType != Constants::CLANG_TOOLCHAIN_TYPEID
            && m_projectPart.toolchainType != Constants::CLANG_CL_TOOLCHAIN_TYPEID) {
        return;
    }

    const QString target = m_explicitTarget.isEmpty() || m_projectPart.targetTripleIsAuthoritative
            ? m_projectPart.toolChainTargetTriple : m_explicitTarget;

    // Only "--target=" style is accepted in both g++ and cl driver modes.
    if (!target.isEmpty())
        add("--target=" + target);
}

void CompilerOptionsBuilder::addExtraCodeModelFlags()
{
    if (m_nativeMode)
        return;

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
    removeUnsupportedCpuFlags();
}

void CompilerOptionsBuilder::addMsvcExceptions()
{
    if (!m_clStyle)
        return;
    if (Utils::anyOf(m_projectPart.toolChainMacros, [](const Macro &macro) {
        return macro.key == "_CPPUNWIND";
    })) {
        enableExceptions();
    }
}

void CompilerOptionsBuilder::enableExceptions()
{
    if (m_nativeMode)
        return;

    // FIXME: Shouldn't this be dependent on the build system settings?

    // With "--driver-mode=cl" exceptions are disabled (clang 8).
    // This is most likely due to incomplete exception support of clang.
    // However, as we need exception support only in the frontend,
    // enabling them explicitly should be fine.
    if (m_projectPart.languageVersion > LanguageVersion::LatestC)
        add("-fcxx-exceptions");
    add("-fexceptions");
}

void CompilerOptionsBuilder::insertWrappedQtHeaders()
{
    if (m_useTweakedHeaderPaths == UseTweakedHeaderPaths::Yes)
        insertWrappedHeaders(wrappedQtHeadersIncludePath());
}

void CompilerOptionsBuilder::insertWrappedMingwHeaders()
{
    insertWrappedHeaders(wrappedMingwHeadersIncludePath());
}

static QString creatorResourcePath()
{
#ifndef UNIT_TESTS
    return Core::ICore::resourcePath().toString();
#else
    return QDir::toNativeSeparators(QString::fromUtf8(QTC_RESOURCE_DIR ""));
#endif
}

void CompilerOptionsBuilder::insertWrappedHeaders(const QStringList &relPaths)
{
    if (m_useTweakedHeaderPaths == UseTweakedHeaderPaths::No)
        return;
    if (relPaths.isEmpty())
        return;

    QStringList args;
    for (const QString &relPath : relPaths) {
        static const QString baseDir = creatorResourcePath() + "/cplusplus";
        const QString fullPath = baseDir + '/' + relPath;
        QTC_ASSERT(QDir(fullPath).exists(), continue);
        args << (includeUserPathOption + QDir::toNativeSeparators(fullPath));
    }

    const int index = m_options.indexOf(QRegularExpression("\\A-I.*\\z"));
    if (index < 0)
        add(args);
    else
        m_options = m_options.mid(0, index) + args + m_options.mid(index);
}

void CompilerOptionsBuilder::addHeaderPathOptions()
{
    Internal::HeaderPathFilter filter{
        m_projectPart,
        m_useTweakedHeaderPaths,
        m_clangIncludeDirectory};

    filter.process();

    for (const HeaderPath &headerPath : std::as_const(filter.userHeaderPaths))
        addIncludeDirOptionForPath(headerPath);
    for (const HeaderPath &headerPath : std::as_const(filter.systemHeaderPaths))
        addIncludeDirOptionForPath(headerPath);

    if (m_useTweakedHeaderPaths != UseTweakedHeaderPaths::No) {

        // Exclude all built-in includes and Clang resource directory.
        m_options.prepend("-nostdinc++");
        m_options.prepend("-nostdinc");

        for (const HeaderPath &headerPath : std::as_const(filter.builtInHeaderPaths))
            addIncludeDirOptionForPath(headerPath);
    }
}

void CompilerOptionsBuilder::addIncludeFile(const QString &file)
{
    if (QFileInfo::exists(file)) {
        add({isClStyle() ? QLatin1String(includeFileOptionCl)
                         : QLatin1String(includeFileOptionGcc),
             QDir::toNativeSeparators(file)});
    }
}

void CompilerOptionsBuilder::removeUnsupportedCpuFlags()
{
    const QStringList blacklist = cpuBlacklist();
    for (auto it = m_options.begin(); it != m_options.end();) {
        if (it->startsWith("-mcpu=") && blacklist.contains(it->mid(6)))
            it = m_options.erase(it);
        else
            ++it;
    }
}

void CompilerOptionsBuilder::addIncludedFiles(const QStringList &files)
{
    for (const QString &file : files) {
        if (m_projectPart.precompiledHeaders.contains(file))
            continue;

        addIncludeFile(file);
    }
}

void CompilerOptionsBuilder::addPrecompiledHeaderOptions(UsePrecompiledHeaders usePrecompiledHeaders)
{
    if (usePrecompiledHeaders == UsePrecompiledHeaders::No)
        return;

    for (const QString &pchFile : m_projectPart.precompiledHeaders) {
        addIncludeFile(pchFile);
    }
}

void CompilerOptionsBuilder::addProjectMacros()
{
    const int useMacros = qtcEnvironmentVariableIntValue("QTC_CLANG_USE_TOOLCHAIN_MACROS");

    if (m_projectPart.toolchainType == ProjectExplorer::Constants::CUSTOM_TOOLCHAIN_TYPEID
        || m_projectPart.toolchainType == Qnx::Constants::QNX_TOOLCHAIN_ID
        || m_projectPart.toolchainType.name().contains("BareMetal") || useMacros) {
        addMacros(m_projectPart.toolChainMacros);
    }

    addMacros(m_projectPart.projectMacros);
    addMacros(m_additionalMacros);
}

void CompilerOptionsBuilder::addMacros(const Macros &macros)
{
    QStringList options;

    for (const Macro &macro : macros) {
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
    if (isClStyle() && !ProjectFile::isObjC(fileKind)) {
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

    const bool objcExt = m_projectPart.languageExtensions & LanguageExtension::ObjectiveC;
    const QStringList options = createLanguageOptionGcc(m_projectPart.language, fileKind, objcExt);
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
    if (m_compilerFlags.isLanguageVersionSpecified)
        return;

    QString option;
    if (isClStyle()) {
        switch (m_projectPart.languageVersion) {
        default:
            break;
        case LanguageVersion::CXX14:
            option = "-clang:-std=c++14";
            break;
        case LanguageVersion::CXX17:
            option = "-clang:-std=c++17";
            break;
        case LanguageVersion::CXX20:
            option = "-clang:-std=c++20";
            break;
        case LanguageVersion::CXX2b:
            option = "-clang:-std=c++2b";
            break;
        }

        if (!option.isEmpty()) {
            if (m_nativeMode)
                option.replace("-clang:-std=", "/std:").replace("c++2b", "c++latest");
            add(option);
            return;
        }

        if (m_nativeMode)
            return;

        // Continue in case no cl-style option could be chosen.
    }

    const LanguageExtensions languageExtensions = m_projectPart.languageExtensions;
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
    case LanguageVersion::CXX20:
        option = (gnuExtensions ? QLatin1String("-std=gnu++20") : QLatin1String("-std=c++20"));
        break;
    case LanguageVersion::CXX2b:
        option = (gnuExtensions ? QLatin1String("-std=gnu++2b") : QLatin1String("-std=c++2b"));
        break;
    case LanguageVersion::None:
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

static QByteArray msCompatibilityVersionFromDefines(const Macros &macros)
{
    for (const Macro &macro : macros) {
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
        "__cpp_constexpr_in_decltype",
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
        "__cpp_impl_destroying_delete",
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
    if (m_nativeMode)
        return;

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
        addMacros({{"__FUNCSIG__", "\"void __cdecl someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580(void)\""},
                   {"__FUNCTION__", "\"someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580\""},
                   {"__FUNCDNAME__", "\"?someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580@@YAXXZ\""}});
    }
}

void CompilerOptionsBuilder::addIncludeDirOptionForPath(const HeaderPath &path)
{
    if (path.type == HeaderPathType::Framework) {
        QTC_ASSERT(!isClStyle(), return;);
        add({"-F", QDir::toNativeSeparators(path.path)});
        return;
    }

    bool systemPath = false;
    if (path.type == HeaderPathType::BuiltIn) {
        systemPath = true;
    } else if (path.type == HeaderPathType::System) {
        if (m_useSystemHeader == UseSystemHeader::Yes)
            systemPath = true;
    } else {
        // ProjectExplorer::HeaderPathType::User
        if (m_useSystemHeader == UseSystemHeader::Yes && m_projectPart.hasProject()
            && !Utils::FilePath::fromString(path.path).isChildOf(m_projectPart.topLevelProject)) {
            systemPath = true;
        }
    }

    if (systemPath) {
        add({includeSystemPathOption, QDir::toNativeSeparators(path.path)}, true);
        return;
    }

    add(includeUserPathOption + QDir::toNativeSeparators(path.path));
}

bool CompilerOptionsBuilder::excludeDefineDirective(const Macro &macro) const
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

QStringList CompilerOptionsBuilder::wrappedQtHeadersIncludePath() const
{
    if (m_projectPart.qtVersion == QtMajorVersion::None)
        return {};
    return {"wrappedQtHeaders", "wrappedQtHeaders/QtCore"};
}

QStringList CompilerOptionsBuilder::wrappedMingwHeadersIncludePath() const
{
    if (m_projectPart.toolchainType != ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID)
        return {};
    return {"wrappedMingwHeaders"};
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

void CompilerOptionsBuilder::addDefineFunctionMacrosQnx()
{
    // QNX 7.0+ uses GCC with LIBCPP from Clang, and in that context GCC is giving
    // the builtin operator new and delete.
    //
    // In our case we have only Clang and need to instruct LIBCPP that it doesn't
    // have these operators. This makes the code model happy and doesn't produce errors.
    if (m_projectPart.toolchainType == Qnx::Constants::QNX_TOOLCHAIN_ID)
        addMacros({{"_LIBCPP_HAS_NO_BUILTIN_OPERATOR_NEW_DELETE"}});
}

void CompilerOptionsBuilder::addQtMacros()
{
    if (m_projectPart.qtVersion != QtMajorVersion::None)
        addMacros({{"QT_ANNOTATE_FUNCTION(x)", "__attribute__((annotate(#x)))"}});
}

void CompilerOptionsBuilder::reset()
{
    m_options.clear();
    m_explicitTarget.clear();
}

// Some example command lines for a "Qt Console Application":
//  CMakeProject: -fPIC -std=gnu++11
//  QbsProject: -m64 -fPIC -std=c++11 -fexceptions
//  QMakeProject: -pipe -Whello -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC
void CompilerOptionsBuilder::evaluateCompilerFlags()
{
    const QStringList userBlackList = qtcEnvironmentVariable("QTC_CLANG_CMD_OPTIONS_BLACKLIST")
                                           .split(';', Qt::SkipEmptyParts);

    const Id toolChain = m_projectPart.toolchainType;
    bool containsDriverMode = false;
    bool skipNext = false;
    bool nextIsTarget = false;
    bool nextIsGccToolchain = false;
    const QStringList allFlags = m_projectPart.extraCodeModelFlags + m_projectPart.compilerFlags;
    for (const QString &option : allFlags) {
        if (skipNext) {
            skipNext = false;
            continue;
        }
        if (nextIsTarget) {
            nextIsTarget = false;
            m_explicitTarget = option;
            continue;
        }
        if (nextIsGccToolchain) {
            nextIsGccToolchain = false;
            m_compilerFlags.flags.append("--gcc-toolchain=" + option);
            continue;
        }

        if (userBlackList.contains(option))
            continue;

        // TODO: Make it possible that the clang binary/driver ignores unknown options,
        // as it is done for libclang/clangd (not checking for OPT_UNKNOWN).
        if (toolChain == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID) {
            if (option == "-fkeep-inline-dllexport" || option == "-fno-keep-inline-dllexport")
                continue;
        }

        // Ignore warning flags as these interfere with our user-configured diagnostics.
        // Note that once "-w" is provided, no warnings will be emitted, even if "-Wall" follows.
        if (m_useBuildSystemWarnings == UseBuildSystemWarnings::No
            && (option.startsWith("-w", Qt::CaseInsensitive)
                || option.startsWith("/w", Qt::CaseInsensitive) || option.startsWith("-pedantic"))) {
            // -w, -W, /w, /W...
            continue;
        }

        // An explicit target triple from the build system takes precedence over the generic one
        // from the toolchain.
        if (option.startsWith("--target=")) {
            m_explicitTarget = option.mid(9);
            continue;
        }
        if (option == "-target") {
            nextIsTarget = true;
            continue;
        }

        if (option == "-gcc-toolchain") {
            nextIsGccToolchain = true;
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
            || option.startsWith(includeUserPathOptionWindows)
            || option.startsWith("-flto")) {
            // Optimization and run-time flags.
            continue;
        }

        // GCC option that clang doesn't know.
        if (option.contains("direct-extern-access"))
            continue;

        // These were already parsed into ProjectPart::includedFiles.
        if (option == includeFileOptionCl || option == includeFileOptionGcc) {
            skipNext = true;
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
        if (theOption.startsWith("-std=") || theOption.startsWith("--std=")) {
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

        if (!m_nativeMode) {
            // Transform the "/" starting commands into "-" commands, which if
            // unknown will not cause clang to fail because it thinks
            // it's a missing file.
            if (theOption.startsWith("/") &&
                    (toolChain == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID ||
                     toolChain == ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID)) {
                theOption[0] = '-';
            }

            if (toolChain == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID ||
                    toolChain == ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID) {
                theOption.replace("-std:c++latest", "-clang:-std=c++2b");
                theOption.replace("-std:c++", "-clang:-std=c++");
            }
        }

        m_compilerFlags.flags.append(theOption);
    }

    if (!m_nativeMode && !containsDriverMode
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

} // namespace CppEditor
