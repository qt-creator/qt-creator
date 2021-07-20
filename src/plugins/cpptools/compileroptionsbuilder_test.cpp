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

#include "cpptoolsplugin.h"

#include "compileroptionsbuilder.h"
#include "projectpart.h"

#include <projectexplorer/headerpath.h>
#include <projectexplorer/project.h>
#include <utils/algorithm.h>
#include <utils/temporarydirectory.h>

#include <QtTest>

#include <memory>

using namespace ProjectExplorer;

namespace CppTools {
namespace Internal {

namespace {
class CompilerOptionsBuilderTest
{
public:
    CompilerOptionsBuilderTest()
    {
        QFile pchFile(pchFileNativePath());
        pchFile.open(QIODevice::WriteOnly);
        projectPart.project = project.get();
        projectPart.toolchainType = Constants::CLANG_TOOLCHAIN_TYPEID;
        projectPart.languageVersion = Utils::LanguageVersion::CXX17;
        projectPart.toolChainWordWidth = ProjectPart::WordWidth64Bit;
        projectPart.toolChainTargetTriple = "x86_64-apple-darwin10";
        projectPart.precompiledHeaders = QStringList{pchFileNativePath()};
        projectPart.toolChainMacros = {Macro{"foo", "bar"},
                                       Macro{"__cplusplus", "2"},
                                       Macro{"__STDC_VERSION__", "2"},
                                       Macro{"_MSVC_LANG", "2"},
                                       Macro{"_MSC_BUILD", "2"},
                                       Macro{"_MSC_FULL_VER", "1900"},
                                       Macro{"_MSC_VER", "19"}};
        projectPart.projectMacros = {Macro{"projectFoo", "projectBar"}};
        projectPart.qtVersion = Utils::QtVersion::Qt5;

        projectPart.headerPaths = {HeaderPath{"/tmp/builtin_path", HeaderPathType::BuiltIn},
                                   HeaderPath{"/tmp/system_path", HeaderPathType::System},
                                   HeaderPath{"/tmp/path", HeaderPathType::User}};
    }

    static HeaderPath builtIn(const QString &path)
    {
        return HeaderPath{path, HeaderPathType::BuiltIn};
    }

    QString toNative(const QString &toNative) const
    {
        return QDir::toNativeSeparators(toNative);
    }

    QString pchFileNativePath() const
    {
        return toNative(Utils::TemporaryDirectory::masterDirectoryPath()
                        + "/compileroptionsbuilder.pch");
    }

    std::unique_ptr<Project> project{std::make_unique<Project>(QString(), Utils::FilePath())};
    ProjectPart projectPart;
    CompilerOptionsBuilder compilerOptionsBuilder{projectPart};
};
}

void CppToolsPlugin::test_optionsBuilder_addProjectMacros()
{
    CompilerOptionsBuilderTest t;
    t.compilerOptionsBuilder.addProjectMacros();

    QCOMPARE(t.compilerOptionsBuilder.options(), QStringList("-DprojectFoo=projectBar"));
}

void CppToolsPlugin::test_optionsBuilder_unknownFlagsAreForwarded()
{
    CompilerOptionsBuilderTest t;
    ProjectPart part = t.projectPart;
    part.compilerFlags = QStringList{"-fancyFlag"};
    CompilerOptionsBuilder compilerOptionsBuilder{part, UseSystemHeader::No,
                UseTweakedHeaderPaths::No, UseLanguageDefines::Yes};
    compilerOptionsBuilder.build(ProjectFile::CXXSource, UsePrecompiledHeaders::No);

    QVERIFY(compilerOptionsBuilder.options().contains(part.compilerFlags.first()));
}

void CppToolsPlugin::test_optionsBuilder_warningsFlagsAreNotFilteredIfRequested()
{
    CompilerOptionsBuilderTest t;
    ProjectPart part = t.projectPart;
    part.compilerFlags = QStringList{"-Whello"};
    CompilerOptionsBuilder compilerOptionsBuilder{part, UseSystemHeader::No,
                UseTweakedHeaderPaths::No, UseLanguageDefines::No,
                UseBuildSystemWarnings::Yes};
    compilerOptionsBuilder.build(ProjectFile::CXXSource, UsePrecompiledHeaders::No);

    QVERIFY(compilerOptionsBuilder.options().contains(part.compilerFlags.first()));
}

void CppToolsPlugin::test_optionsBuilder_diagnosticOptionsAreRemoved()
{
    CompilerOptionsBuilderTest t;
    ProjectPart part = t.projectPart;
    part.compilerFlags = QStringList{"-Wbla", "-pedantic"};
    CompilerOptionsBuilder compilerOptionsBuilder{part, UseSystemHeader::No,
                UseTweakedHeaderPaths::No, UseLanguageDefines::Yes};
    compilerOptionsBuilder.build(ProjectFile::CXXSource, UsePrecompiledHeaders::No);

    QVERIFY(!compilerOptionsBuilder.options().contains(part.compilerFlags.at(0)));
    QVERIFY(!compilerOptionsBuilder.options().contains(part.compilerFlags.at(1)));
}

void CppToolsPlugin::test_optionsBuilder_cLanguageVersionIsRewritten()
{
    CompilerOptionsBuilderTest t;
    ProjectPart part = t.projectPart;
    part.compilerFlags = QStringList{"-std=c18"};

    // We need to set the language version here to overcome a QTC_ASSERT checking
    // consistency between ProjectFile::Kind and ProjectPart::LanguageVersion
    part.languageVersion = Utils::LanguageVersion::C18;

    CompilerOptionsBuilder compilerOptionsBuilder{part, UseSystemHeader::No,
                UseTweakedHeaderPaths::No, UseLanguageDefines::Yes};
    compilerOptionsBuilder.build(ProjectFile::CSource, UsePrecompiledHeaders::No);

    QVERIFY(!compilerOptionsBuilder.options().contains(part.compilerFlags.first()));
    QVERIFY(compilerOptionsBuilder.options().contains("-std=c17"));
}

void CppToolsPlugin::test_optionsBuilder_languageVersionIsExplicitlySetIfNotProvided()
{
    CompilerOptionsBuilderTest t;
    CompilerOptionsBuilder compilerOptionsBuilder{t.projectPart, UseSystemHeader::No,
                UseTweakedHeaderPaths::No, UseLanguageDefines::Yes};
    compilerOptionsBuilder.build(ProjectFile::CXXSource, UsePrecompiledHeaders::No);

    QVERIFY(compilerOptionsBuilder.options().contains("-std=c++17"));
}

void CppToolsPlugin::test_optionsBuilder_LanguageVersionIsExplicitlySetIfNotProvidedMsvc()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    CompilerOptionsBuilder compilerOptionsBuilder{t.projectPart, UseSystemHeader::No,
                UseTweakedHeaderPaths::No, UseLanguageDefines::Yes};
    compilerOptionsBuilder.build(ProjectFile::CXXSource, UsePrecompiledHeaders::No);

    QVERIFY(compilerOptionsBuilder.options().contains("/std:c++17"));
}

void CppToolsPlugin::test_optionsBuilder_addWordWidth()
{
    CompilerOptionsBuilderTest t;
    t.compilerOptionsBuilder.addWordWidth();

    QCOMPARE(t.compilerOptionsBuilder.options(), QStringList("-m64"));
}

void CppToolsPlugin::test_optionsBuilder_headerPathOptionsOrder()
{
    CompilerOptionsBuilderTest t;
    CompilerOptionsBuilder compilerOptionsBuilder{t.projectPart, UseSystemHeader::No,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "dummy_version", ""};
    compilerOptionsBuilder.addHeaderPathOptions();

    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++", "-I", t.toNative("/tmp/path"),
                         "-I", t.toNative("/tmp/system_path"), "-isystem", "", "-isystem",
                         t.toNative("/tmp/builtin_path")}));
}

void CppToolsPlugin::test_optionsBuilder_HeaderPathOptionsOrderMsvc()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    CompilerOptionsBuilder compilerOptionsBuilder{t.projectPart, UseSystemHeader::No,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "dummy_version", ""};
    compilerOptionsBuilder.evaluateCompilerFlags();
    compilerOptionsBuilder.addHeaderPathOptions();

    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++", "-I", t.toNative("/tmp/path"),
                            "-I", t.toNative("/tmp/system_path"), "/clang:-isystem",
                          "/clang:", "/clang:-isystem",
                          "/clang:" + t.toNative("/tmp/builtin_path")}));
}

void CppToolsPlugin::test_optionsBuilder_useSystemHeader()
{
    CompilerOptionsBuilderTest t;
    CompilerOptionsBuilder compilerOptionsBuilder{t.projectPart, UseSystemHeader::Yes,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "dummy_version", ""};
    compilerOptionsBuilder.addHeaderPathOptions();

    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++", "-I", t.toNative("/tmp/path"),
                          "-isystem", t.toNative("/tmp/system_path"),
                          "-isystem", "", "-isystem", t.toNative("/tmp/builtin_path")}));
}

void CppToolsPlugin::test_optionsBuilder_noClangHeadersPath()
{
    CompilerOptionsBuilderTest t;
    t.compilerOptionsBuilder.addHeaderPathOptions();

    QCOMPARE(t.compilerOptionsBuilder.options(),
             (QStringList{"-I", t.toNative("/tmp/path"), "-I", t.toNative("/tmp/system_path")}));
}

void CppToolsPlugin::test_optionsBuilder_clangHeadersAndCppIncludePathsOrderMacOs()
{
    CompilerOptionsBuilderTest t;
    const auto defaultPaths = t.projectPart.headerPaths;
    t.projectPart.headerPaths = {
        t.builtIn("/usr/include/c++/4.2.1"),
        t.builtIn("/usr/include/c++/4.2.1/backward"),
        t.builtIn("/usr/local/include"),
        t.builtIn("/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/6.0/include"),
        t.builtIn("/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include"),
        t.builtIn("/usr/include")
    };
    t.projectPart.headerPaths.append(defaultPaths);
    CompilerOptionsBuilder compilerOptionsBuilder(t.projectPart, UseSystemHeader::No,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "dummy_version", "");
    compilerOptionsBuilder.addHeaderPathOptions();

    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++", "-I", t.toNative("/tmp/path"),
                          "-I", t.toNative("/tmp/system_path"),
                          "-isystem", t.toNative("/usr/include/c++/4.2.1"),
                          "-isystem", t.toNative("/usr/include/c++/4.2.1/backward"),
                          "-isystem", t.toNative("/usr/local/include"),
                          "-isystem", "",
                          "-isystem", t.toNative("/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include"),
                          "-isystem", t.toNative("/usr/include"),
                          "-isystem", t.toNative("/tmp/builtin_path")}));
}

void CppToolsPlugin::test_optionsBuilder_clangHeadersAndCppIncludePathsOrderLinux()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.headerPaths = {
        t.builtIn("/usr/include/c++/4.8"),
        t.builtIn("/usr/include/c++/4.8/backward"),
        t.builtIn("/usr/include/x86_64-linux-gnu/c++/4.8"),
        t.builtIn("/usr/local/include"),
        t.builtIn("/usr/lib/gcc/x86_64-linux-gnu/4.8/include"),
        t.builtIn("/usr/include/x86_64-linux-gnu"),
        t.builtIn("/usr/include"),
    };
    t.projectPart.toolChainTargetTriple = "x86_64-linux-gnu";
    CompilerOptionsBuilder compilerOptionsBuilder(t.projectPart, UseSystemHeader::No,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "dummy_version", "");
    compilerOptionsBuilder.addHeaderPathOptions();

    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++",
                          "-isystem", t.toNative("/usr/include/c++/4.8"),
                          "-isystem", t.toNative("/usr/include/c++/4.8/backward"),
                          "-isystem", t.toNative("/usr/include/x86_64-linux-gnu/c++/4.8"),
                          "-isystem", t.toNative("/usr/local/include"),
                          "-isystem", "",
                          "-isystem", t.toNative("/usr/lib/gcc/x86_64-linux-gnu/4.8/include"),
                          "-isystem", t.toNative("/usr/include/x86_64-linux-gnu"),
                          "-isystem", t.toNative("/usr/include")}));
}

void CppToolsPlugin::test_optionsBuilder_clangHeadersAndCppIncludePathsOrderNoVersion()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.headerPaths = {
        t.builtIn("C:/mingw530/i686-w64-mingw32/include"),
        t.builtIn("C:/mingw530/i686-w64-mingw32/include/c++"),
        t.builtIn("C:/mingw530/i686-w64-mingw32/include/c++/i686-w64-mingw32"),
        t.builtIn("C:/mingw530/i686-w64-mingw32/include/c++/backward"),
    };
    t.projectPart.toolChainTargetTriple = "x86_64-w64-windows-gnu";
    CompilerOptionsBuilder compilerOptionsBuilder(t.projectPart, UseSystemHeader::No,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "dummy_version", "");
    compilerOptionsBuilder.addHeaderPathOptions();

    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++",
                          "-isystem", t.toNative("C:/mingw530/i686-w64-mingw32/include/c++"),
                          "-isystem", t.toNative("C:/mingw530/i686-w64-mingw32/include/c++/i686-w64-mingw32"),
                          "-isystem", t.toNative("C:/mingw530/i686-w64-mingw32/include/c++/backward"),
                          "-isystem", "",
                          "-isystem", t.toNative("C:/mingw530/i686-w64-mingw32/include")}));
}

void CppToolsPlugin::test_optionsBuilder_clangHeadersAndCppIncludePathsOrderAndroidClang()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.headerPaths = {
        t.builtIn("C:/Android/sdk/ndk-bundle/sysroot/usr/include/i686-linux-android"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++/include"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sources/android/support/include"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++abi/include"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sysroot/usr/include"),
    };
    t.projectPart.toolChainTargetTriple = "i686-linux-android";
    CompilerOptionsBuilder compilerOptionsBuilder(t.projectPart, UseSystemHeader::No,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "dummy_version", "");
    compilerOptionsBuilder.addHeaderPathOptions();

    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++",
                          "-isystem", t.toNative("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++/include"),
                          "-isystem", t.toNative("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++abi/include"),
                          "-isystem", t.toNative(""),
                          "-isystem", t.toNative("C:/Android/sdk/ndk-bundle/sysroot/usr/include/i686-linux-android"),
                          "-isystem", t.toNative("C:/Android/sdk/ndk-bundle/sources/android/support/include"),
                          "-isystem", t.toNative("C:/Android/sdk/ndk-bundle/sysroot/usr/include")}));
}

void CppToolsPlugin::test_optionsBuilder_noPrecompiledHeader()
{
    CompilerOptionsBuilderTest t;
    t.compilerOptionsBuilder.addPrecompiledHeaderOptions(UsePrecompiledHeaders::No);

    QVERIFY(t.compilerOptionsBuilder.options().empty());
}

void CppToolsPlugin::test_optionsBuilder_usePrecompiledHeader()
{
    CompilerOptionsBuilderTest t;
    t.compilerOptionsBuilder.addPrecompiledHeaderOptions(UsePrecompiledHeaders::Yes);

    QCOMPARE(t.compilerOptionsBuilder.options(), (QStringList{"-include", t.pchFileNativePath()}));
}

void CppToolsPlugin::test_optionsBuilder_usePrecompiledHeaderMsvc()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    CompilerOptionsBuilder compilerOptionsBuilder{t.projectPart};
    compilerOptionsBuilder.evaluateCompilerFlags();
    compilerOptionsBuilder.addPrecompiledHeaderOptions(UsePrecompiledHeaders::Yes);

    QCOMPARE(compilerOptionsBuilder.options(), (QStringList{"/FI", t.pchFileNativePath()}));
}

void CppToolsPlugin::test_optionsBuilder_addMacros()
{
    CompilerOptionsBuilderTest t;
    t.compilerOptionsBuilder.addMacros(Macros{Macro{"key", "value"}});

    QCOMPARE(t.compilerOptionsBuilder.options(), QStringList("-Dkey=value"));
}

void CppToolsPlugin::test_optionsBuilder_addTargetTriple()
{
    CompilerOptionsBuilderTest t;
    t.compilerOptionsBuilder.addTargetTriple();

    QCOMPARE(t.compilerOptionsBuilder.options(), QStringList("--target=x86_64-apple-darwin10"));
}

void CppToolsPlugin::test_optionsBuilder_enableCExceptions()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.languageVersion = Utils::LanguageVersion::C99;
    t.compilerOptionsBuilder.enableExceptions();

    QCOMPARE(t.compilerOptionsBuilder.options(), QStringList("-fexceptions"));
}

void CppToolsPlugin::test_optionsBuilder_enableCxxExceptions()
{
    CompilerOptionsBuilderTest t;
    t.compilerOptionsBuilder.enableExceptions();

    QCOMPARE(t.compilerOptionsBuilder.options(), (QStringList{"-fcxx-exceptions", "-fexceptions"}));
}

void CppToolsPlugin::test_optionsBuilder_insertWrappedQtHeaders()
{
    CompilerOptionsBuilderTest t;
    CompilerOptionsBuilder compilerOptionsBuilder{t.projectPart, UseSystemHeader::Yes,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "dummy_version", ""};
    compilerOptionsBuilder.insertWrappedQtHeaders();

    QVERIFY(Utils::contains(compilerOptionsBuilder.options(),
                            [](const QString &o) { return o.contains("wrappedQtHeaders"); }));
}

void CppToolsPlugin::test_optionsBuilder_insertWrappedMingwHeadersWithNonMingwToolchain()
{
    CompilerOptionsBuilderTest t;
    CompilerOptionsBuilder builder{t.projectPart,
                                             UseSystemHeader::Yes,
                                             UseTweakedHeaderPaths::Yes,
                                             UseLanguageDefines::No,
                                             UseBuildSystemWarnings::No,
                                             "dummy_version",
                                             ""};
    builder.insertWrappedMingwHeaders();

    QVERIFY(!Utils::contains(builder.options(),
                            [](const QString &o) { return o.contains("wrappedMingwHeaders"); }));
}

void CppToolsPlugin::test_optionsBuilder_insertWrappedMingwHeadersWithMingwToolchain()
{
    CompilerOptionsBuilderTest t;
    CompilerOptionsBuilder builder{t.projectPart, UseSystemHeader::Yes, UseTweakedHeaderPaths::Yes,
                UseLanguageDefines::No, UseBuildSystemWarnings::No, "dummy_version", ""};
    t.projectPart.toolchainType = Constants::MINGW_TOOLCHAIN_TYPEID;
    builder.insertWrappedMingwHeaders();

    QVERIFY(Utils::contains(builder.options(),
                            [](const QString &o) { return o.contains("wrappedMingwHeaders"); }));
}

void CppToolsPlugin::test_optionsBuilder_setLanguageVersion()
{
    CompilerOptionsBuilderTest t;
    t.compilerOptionsBuilder.updateFileLanguage(ProjectFile::CXXSource);

    QCOMPARE(t.compilerOptionsBuilder.options(), (QStringList{"-x", "c++"}));
}

void CppToolsPlugin::test_optionsBuilder_setLanguageVersionMsvc()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    CompilerOptionsBuilder compilerOptionsBuilder{t.projectPart};
    compilerOptionsBuilder.evaluateCompilerFlags();
    compilerOptionsBuilder.updateFileLanguage(ProjectFile::CXXSource);

    QCOMPARE(compilerOptionsBuilder.options(), QStringList("/TP"));
}

void CppToolsPlugin::test_optionsBuilder_handleLanguageExtension()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.languageExtensions = Utils::LanguageExtension::ObjectiveC;
    t.compilerOptionsBuilder.updateFileLanguage(ProjectFile::CXXSource);

    QCOMPARE(t.compilerOptionsBuilder.options(), (QStringList{"-x", "objective-c++"}));
}

void CppToolsPlugin::test_optionsBuilder_updateLanguageVersion()
{
    CompilerOptionsBuilderTest t;
    t.compilerOptionsBuilder.updateFileLanguage(ProjectFile::CXXSource);
    t.compilerOptionsBuilder.updateFileLanguage(ProjectFile::CXXHeader);

    QCOMPARE(t.compilerOptionsBuilder.options(), (QStringList{"-x", "c++-header"}));
}

void CppToolsPlugin::test_optionsBuilder_updateLanguageVersionMsvc()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    CompilerOptionsBuilder compilerOptionsBuilder{t.projectPart};
    compilerOptionsBuilder.evaluateCompilerFlags();
    compilerOptionsBuilder.updateFileLanguage(ProjectFile::CXXSource);
    compilerOptionsBuilder.updateFileLanguage(ProjectFile::CSource);

    QCOMPARE(compilerOptionsBuilder.options(), QStringList("/TC"));
}

void CppToolsPlugin::test_optionsBuilder_addMsvcCompatibilityVersion()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    t.projectPart.toolChainMacros.append(Macro{"_MSC_FULL_VER", "190000000"});
    t.compilerOptionsBuilder.addMsvcCompatibilityVersion();

    QCOMPARE(t.compilerOptionsBuilder.options(), QStringList("-fms-compatibility-version=19.00"));
}

void CppToolsPlugin::test_optionsBuilder_undefineCppLanguageFeatureMacrosForMsvc2015()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    t.projectPart.isMsvc2015Toolchain = true;
    t.compilerOptionsBuilder.undefineCppLanguageFeatureMacrosForMsvc2015();

    QVERIFY(t.compilerOptionsBuilder.options().contains("-U__cpp_aggregate_bases"));
}

void CppToolsPlugin::test_optionsBuilder_addDefineFunctionMacrosMsvc()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    t.compilerOptionsBuilder.addDefineFunctionMacrosMsvc();

    QVERIFY(t.compilerOptionsBuilder.options().contains(
        "-D__FUNCTION__=\"someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580\""));
}

void CppToolsPlugin::test_optionsBuilder_addProjectConfigFileInclude()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.projectConfigFile = "dummy_file.h";
    t.compilerOptionsBuilder.addProjectConfigFileInclude();

    QCOMPARE(t.compilerOptionsBuilder.options(), (QStringList{"-include", "dummy_file.h"}));
}

void CppToolsPlugin::test_optionsBuilder_addProjectConfigFileIncludeMsvc()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.projectConfigFile = "dummy_file.h";
    t.projectPart.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    CompilerOptionsBuilder compilerOptionsBuilder{t.projectPart};
    compilerOptionsBuilder.evaluateCompilerFlags();
    compilerOptionsBuilder.addProjectConfigFileInclude();

    QCOMPARE(compilerOptionsBuilder.options(), (QStringList{"/FI", "dummy_file.h"}));
}

void CppToolsPlugin::test_optionsBuilder_noUndefineClangVersionMacrosForNewMsvc()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    t.compilerOptionsBuilder.undefineClangVersionMacrosForMsvc();

    QVERIFY(!t.compilerOptionsBuilder.options().contains("-U__clang__"));
}

void CppToolsPlugin::test_optionsBuilder_undefineClangVersionMacrosForOldMsvc()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    t.projectPart.toolChainMacros = {Macro{"_MSC_FULL_VER", "1300"},
                                     Macro{"_MSC_VER", "13"}};
    t.compilerOptionsBuilder.undefineClangVersionMacrosForMsvc();

    QVERIFY(t.compilerOptionsBuilder.options().contains("-U__clang__"));
}

void CppToolsPlugin::test_optionsBuilder_buildAllOptions()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.extraCodeModelFlags = QStringList{"-arch", "x86_64"};
    CompilerOptionsBuilder compilerOptionsBuilder(t.projectPart, UseSystemHeader::No,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "dummy_version", "");
    compilerOptionsBuilder.build(ProjectFile::CXXSource, UsePrecompiledHeaders::No);

    const QString wrappedQtHeadersPath = Utils::findOrDefault(compilerOptionsBuilder.options(),
            [](const QString &o) { return o.contains("wrappedQtHeaders"); });
    const QString wrappedQtCoreHeadersPath = Utils::findOrDefault(compilerOptionsBuilder.options(),
            [&t](const QString &o) { return o.contains(t.toNative("wrappedQtHeaders/QtCore")); });
    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++", "-arch", "x86_64", "-fsyntax-only", "-m64",
                          "--target=x86_64-apple-darwin10", "-x", "c++", "-std=c++17",
                          "-DprojectFoo=projectBar", "-I", wrappedQtHeadersPath,
                          "-I", wrappedQtCoreHeadersPath,
                          "-I", t.toNative("/tmp/path"),
                          "-I", t.toNative("/tmp/system_path"),
                          "-isystem", "",
                          "-isystem", t.toNative("/tmp/builtin_path")}));
}

void CppToolsPlugin::test_optionsBuilder_buildAllOptionsMsvc()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    CompilerOptionsBuilder compilerOptionsBuilder(t.projectPart, UseSystemHeader::No,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "dummy_version", "");
    compilerOptionsBuilder.build(ProjectFile::CXXSource, UsePrecompiledHeaders::No);

    const QString wrappedQtHeadersPath = Utils::findOrDefault(compilerOptionsBuilder.options(),
            [](const QString &o) { return o.contains("wrappedQtHeaders"); });
    const QString wrappedQtCoreHeadersPath = Utils::findOrDefault(compilerOptionsBuilder.options(),
            [&t](const QString &o) { return o.contains(t.toNative("wrappedQtHeaders/QtCore")); });
    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++", "--driver-mode=cl", "/Zs", "-m64",
                          "--target=x86_64-apple-darwin10", "/TP", "/std:c++17",
                          "-fms-compatibility-version=19.00", "-DprojectFoo=projectBar",
                          "-D__FUNCSIG__=\"void __cdecl someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580(void)\"",
                          "-D__FUNCTION__=\"someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580\"",
                          "-D__FUNCDNAME__=\"?someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580@@YAXXZ\"",
                          "-I", wrappedQtHeadersPath,
                          "-I", wrappedQtCoreHeadersPath,
                          "-I", t.toNative("/tmp/path"),
                          "-I", t.toNative("/tmp/system_path"),
                          "/clang:-isystem", "/clang:",
                          "/clang:-isystem", "/clang:" + t.toNative("/tmp/builtin_path")}));
}

void CppToolsPlugin::test_optionsBuilder_buildAllOptionsMsvcWithExceptions()
{
    CompilerOptionsBuilderTest t;
    t.projectPart.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    t.projectPart.toolChainMacros.append(Macro{"_CPPUNWIND", "1"});
    CompilerOptionsBuilder compilerOptionsBuilder(t.projectPart, UseSystemHeader::No,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "dummy_version", "");
    compilerOptionsBuilder.build(ProjectFile::CXXSource, UsePrecompiledHeaders::No);

    const QString wrappedQtHeadersPath = Utils::findOrDefault(compilerOptionsBuilder.options(),
            [](const QString &o) { return o.contains("wrappedQtHeaders"); });
    const QString wrappedQtCoreHeadersPath = Utils::findOrDefault(compilerOptionsBuilder.options(),
            [&t](const QString &o) { return o.contains(t.toNative("wrappedQtHeaders/QtCore")); });
    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++", "--driver-mode=cl", "/Zs", "-m64",
                          "--target=x86_64-apple-darwin10", "/TP", "/std:c++17", "-fcxx-exceptions",
                          "-fexceptions", "-fms-compatibility-version=19.00",
                          "-DprojectFoo=projectBar",
                          "-D__FUNCSIG__=\"void __cdecl someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580(void)\"",
                          "-D__FUNCTION__=\"someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580\"",
                          "-D__FUNCDNAME__=\"?someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580@@YAXXZ\"",
                          "-I", wrappedQtHeadersPath,
                          "-I", wrappedQtCoreHeadersPath,
                          "-I", t.toNative("/tmp/path"),
                          "-I", t.toNative("/tmp/system_path"),
                          "/clang:-isystem", "/clang:",
                          "/clang:-isystem", "/clang:" + t.toNative("/tmp/builtin_path")}));
}

} // namespace Internal
} // namespace CppTools
