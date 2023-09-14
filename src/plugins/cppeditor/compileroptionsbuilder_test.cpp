// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compileroptionsbuilder_test.h"

#include "compileroptionsbuilder.h"
#include "projectinfo.h"
#include "projectpart.h"

#include <projectexplorer/headerpath.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/algorithm.h>
#include <utils/temporarydirectory.h>

#include <QtTest>

#include <memory>

using namespace ProjectExplorer;

namespace CppEditor::Internal {

namespace {
class TestHelper
{
public:
    const ProjectPart &finalize()
    {
        QFile pchFile(pchFileNativePath());
        pchFile.open(QIODevice::WriteOnly);
        RawProjectPart rpp;
        rpp.setPreCompiledHeaders({pchFileNativePath()});
        rpp.setMacros({Macro{"projectFoo", "projectBar"}});
        rpp.setQtVersion(Utils::QtMajorVersion::Qt5);
        rpp.setHeaderPaths(headerPaths);
        rpp.setConfigFileName(projectConfigFile);
        ToolChainInfo tcInfo;
        tcInfo.type = toolchainType;
        tcInfo.targetTriple = targetTriple;
        tcInfo.abi = Abi::fromString(targetTriple);
        if (!tcInfo.abi.isValid()) {
            tcInfo.abi = Abi(Abi::X86Architecture, Abi::DarwinOS, Abi::FreeBsdFlavor,
                             Abi::MachOFormat, 64);
        }
        tcInfo.isMsvc2015ToolChain = isMsvc2015;
        tcInfo.extraCodeModelFlags = extraFlags;
        tcInfo.macroInspectionRunner = [this](const QStringList &) {
            return ToolChain::MacroInspectionReport{toolchainMacros, languageVersion};
        };
        RawProjectPartFlags rppFlags;
        rppFlags.commandLineFlags = flags;
        projectPart = ProjectPart::create({}, rpp, {}, {}, Utils::Language::Cxx, languageExtensions,
                                          rppFlags, tcInfo);
        compilerOptionsBuilder.emplace(CompilerOptionsBuilder(*projectPart));
        return *projectPart;
    }

    static HeaderPath builtIn(const QString &path) { return HeaderPath::makeBuiltIn(path); }

    QString toNative(const QString &toNative) const
    {
        return QDir::toNativeSeparators(toNative);
    }

    QString pchFileNativePath() const
    {
        return toNative(Utils::TemporaryDirectory::masterDirectoryPath()
                        + "/compileroptionsbuilder.pch");
    }

    QStringList flags;
    Utils::Id toolchainType = Constants::CLANG_TOOLCHAIN_TYPEID;
    QString targetTriple = "x86_64-apple-darwin10";
    HeaderPaths headerPaths{builtIn("/tmp/builtin_path"),
                HeaderPath::makeSystem("/tmp/system_path"),
                HeaderPath::makeUser("/tmp/path")};
    Utils::LanguageVersion languageVersion = Utils::LanguageVersion::CXX17;
    Utils::LanguageExtensions languageExtensions;
    Macros toolchainMacros{
        Macro{"foo", "bar"}, Macro{"__cplusplus", "2"}, Macro{"__STDC_VERSION__", "2"},
        Macro{"_MSVC_LANG", "2"}, Macro{"_MSC_BUILD", "2"}, Macro{"_MSC_FULL_VER", "1900"},
        Macro{"_MSC_VER", "19"}};
    QString projectConfigFile;
    QStringList extraFlags;
    bool isMsvc2015 = false;

    std::optional<CompilerOptionsBuilder> compilerOptionsBuilder;

private:
    ProjectPart::ConstPtr projectPart;
};
}

void CompilerOptionsBuilderTest::testAddProjectMacros()
{
    TestHelper t;
    t.finalize();
    t.compilerOptionsBuilder->addProjectMacros();

    QCOMPARE(t.compilerOptionsBuilder->options(), QStringList("-DprojectFoo=projectBar"));
}

void CompilerOptionsBuilderTest::testUnknownFlagsAreForwarded()
{
    TestHelper t;
    t.flags = QStringList{"-fancyFlag"};
    ProjectPart part = t.finalize();
    CompilerOptionsBuilder compilerOptionsBuilder{part, UseSystemHeader::No,
                UseTweakedHeaderPaths::No, UseLanguageDefines::Yes};
    compilerOptionsBuilder.build(ProjectFile::CXXSource, UsePrecompiledHeaders::No);

    QVERIFY(compilerOptionsBuilder.options().contains(part.compilerFlags.first()));
}

void CompilerOptionsBuilderTest::testWarningsFlagsAreNotFilteredIfRequested()
{
    TestHelper t;
    t.flags = QStringList{"-Whello"};
    ProjectPart part = t.finalize();
    CompilerOptionsBuilder compilerOptionsBuilder{part, UseSystemHeader::No,
                UseTweakedHeaderPaths::No, UseLanguageDefines::No,
                UseBuildSystemWarnings::Yes};
    compilerOptionsBuilder.build(ProjectFile::CXXSource, UsePrecompiledHeaders::No);

    QVERIFY(compilerOptionsBuilder.options().contains(part.compilerFlags.first()));
}

void CompilerOptionsBuilderTest::testDiagnosticOptionsAreRemoved()
{
    TestHelper t;
    t.flags = QStringList{"-Wbla", "-pedantic"};
    ProjectPart part = t.finalize();
    CompilerOptionsBuilder compilerOptionsBuilder{part, UseSystemHeader::No,
                UseTweakedHeaderPaths::No, UseLanguageDefines::Yes};
    compilerOptionsBuilder.build(ProjectFile::CXXSource, UsePrecompiledHeaders::No);

    QVERIFY(!compilerOptionsBuilder.options().contains(part.compilerFlags.at(0)));
    QVERIFY(!compilerOptionsBuilder.options().contains(part.compilerFlags.at(1)));
}

void CompilerOptionsBuilderTest::testCLanguageVersionIsRewritten()
{
    TestHelper t;
    // We need to set the language version here to overcome a QTC_ASSERT checking
    // consistency between ProjectFile::Kind and ProjectPart::LanguageVersion
    t.flags = QStringList{"-std=c18"};
    t.languageVersion = Utils::LanguageVersion::C18;
    ProjectPart part = t.finalize();

    CompilerOptionsBuilder compilerOptionsBuilder{part, UseSystemHeader::No,
                UseTweakedHeaderPaths::No, UseLanguageDefines::Yes};
    compilerOptionsBuilder.build(ProjectFile::CSource, UsePrecompiledHeaders::No);

    QVERIFY(!compilerOptionsBuilder.options().contains(part.compilerFlags.first()));
    QVERIFY(compilerOptionsBuilder.options().contains("-std=c17"));
}

void CompilerOptionsBuilderTest::testLanguageVersionIsExplicitlySetIfNotProvided()
{
    TestHelper t;
    CompilerOptionsBuilder compilerOptionsBuilder{t.finalize(), UseSystemHeader::No,
                UseTweakedHeaderPaths::No, UseLanguageDefines::Yes};
    compilerOptionsBuilder.build(ProjectFile::CXXSource, UsePrecompiledHeaders::No);

    QVERIFY(compilerOptionsBuilder.options().contains("-std=c++17"));
}

void CompilerOptionsBuilderTest::testLanguageVersionIsExplicitlySetIfNotProvidedMsvc()
{
    TestHelper t;
    t.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    CompilerOptionsBuilder compilerOptionsBuilder{t.finalize(), UseSystemHeader::No,
                UseTweakedHeaderPaths::No, UseLanguageDefines::Yes};
    compilerOptionsBuilder.build(ProjectFile::CXXSource, UsePrecompiledHeaders::No);

    QVERIFY(compilerOptionsBuilder.options().contains("-clang:-std=c++17"));
}

void CompilerOptionsBuilderTest::testAddWordWidth()
{
    TestHelper t;
    t.finalize();
    t.compilerOptionsBuilder->addWordWidth();

    QCOMPARE(t.compilerOptionsBuilder->options(), QStringList("-m64"));
}

void CompilerOptionsBuilderTest::testHeaderPathOptionsOrder()
{
    TestHelper t;
    CompilerOptionsBuilder compilerOptionsBuilder{t.finalize(), UseSystemHeader::No,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "/dummy"};
    compilerOptionsBuilder.addHeaderPathOptions();

    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++", "-I" + t.toNative("/tmp/path"),
                         "-I" + t.toNative("/tmp/system_path"), "-isystem", t.toNative("/dummy"),
                          "-isystem", t.toNative("/tmp/builtin_path")}));
}

void CompilerOptionsBuilderTest::testHeaderPathOptionsOrderMsvc()
{
    TestHelper t;
    t.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    CompilerOptionsBuilder compilerOptionsBuilder{t.finalize(), UseSystemHeader::No,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "/dummy"};
    compilerOptionsBuilder.evaluateCompilerFlags();
    compilerOptionsBuilder.addHeaderPathOptions();

    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++", "-I" + t.toNative("/tmp/path"),
                            "-I" + t.toNative("/tmp/system_path"), "/clang:-isystem",
                          "/clang:" + t.toNative("/dummy"), "/clang:-isystem",
                          "/clang:" + t.toNative("/tmp/builtin_path")}));
}

void CompilerOptionsBuilderTest::testUseSystemHeader()
{
    TestHelper t;
    CompilerOptionsBuilder compilerOptionsBuilder{t.finalize(), UseSystemHeader::Yes,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "/dummy"};
    compilerOptionsBuilder.addHeaderPathOptions();

    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++", "-I" + t.toNative("/tmp/path"),
                          "-isystem", t.toNative("/tmp/system_path"),
                          "-isystem", t.toNative("/dummy"),
                          "-isystem", t.toNative("/tmp/builtin_path")}));
}

void CompilerOptionsBuilderTest::testNoClangHeadersPath()
{
    TestHelper t;
    t.finalize();
    t.compilerOptionsBuilder->addHeaderPathOptions();

    QCOMPARE(t.compilerOptionsBuilder->options(),
             (QStringList{"-I" + t.toNative("/tmp/path"), "-I" + t.toNative("/tmp/system_path")}));
}

void CompilerOptionsBuilderTest::testClangHeadersAndCppIncludePathsOrderMacOs()
{
    TestHelper t;
    const HeaderPaths additionalHeaderPaths = {
        t.builtIn("/usr/include/c++/4.2.1"),
        t.builtIn("/usr/include/c++/4.2.1/backward"),
        t.builtIn("/usr/local/include"),
        t.builtIn("/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/6.0/include"),
        t.builtIn("/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include"),
        t.builtIn("/usr/include")};
    t.headerPaths = additionalHeaderPaths + t.headerPaths;
    CompilerOptionsBuilder compilerOptionsBuilder(t.finalize(), UseSystemHeader::No,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "/dummy");
    compilerOptionsBuilder.addHeaderPathOptions();

    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++", "-I" + t.toNative("/tmp/path"),
                          "-I" + t.toNative("/tmp/system_path"),
                          "-isystem", t.toNative("/usr/include/c++/4.2.1"),
                          "-isystem", t.toNative("/usr/include/c++/4.2.1/backward"),
                          "-isystem", t.toNative("/usr/local/include"),
                          "-isystem", t.toNative("/dummy"),
                          "-isystem", t.toNative("/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include"),
                          "-isystem", t.toNative("/usr/include"),
                          "-isystem", t.toNative("/tmp/builtin_path")}));
}

void CompilerOptionsBuilderTest::testClangHeadersAndCppIncludePathsOrderLinux()
{
    TestHelper t;
    t.targetTriple = "x86_64-linux-gnu";
    t.headerPaths = {
        t.builtIn("/usr/include/c++/4.8"),
        t.builtIn("/usr/include/c++/4.8/backward"),
        t.builtIn("/usr/include/x86_64-linux-gnu/c++/4.8"),
        t.builtIn("/usr/local/include"),
        t.builtIn("/usr/lib/gcc/x86_64-linux-gnu/4.8/include"),
        t.builtIn("/usr/include/x86_64-linux-gnu"),
        t.builtIn("/usr/lib64/clang/16/include"),
        t.builtIn("/usr/lib/clang/15.0.7/include"),
        t.builtIn("/usr/include")};
    CompilerOptionsBuilder compilerOptionsBuilder(t.finalize(), UseSystemHeader::No,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "/dummy");
    compilerOptionsBuilder.addHeaderPathOptions();

    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++",
                          "-isystem", t.toNative("/usr/include/c++/4.8"),
                          "-isystem", t.toNative("/usr/include/c++/4.8/backward"),
                          "-isystem", t.toNative("/usr/include/x86_64-linux-gnu/c++/4.8"),
                          "-isystem", t.toNative("/usr/local/include"),
                          "-isystem", t.toNative("/dummy"),
                          "-isystem", t.toNative("/usr/lib/gcc/x86_64-linux-gnu/4.8/include"),
                          "-isystem", t.toNative("/usr/include/x86_64-linux-gnu"),
                          "-isystem", t.toNative("/usr/include")}));
}

void CompilerOptionsBuilderTest::testClangHeadersAndCppIncludePathsOrderNoVersion()
{
    TestHelper t;
    t.targetTriple = "x86_64-w64-windows-gnu";
    t.headerPaths = {
        t.builtIn("C:/mingw530/i686-w64-mingw32/include"),
        t.builtIn("C:/mingw530/i686-w64-mingw32/include/c++"),
        t.builtIn("C:/mingw530/i686-w64-mingw32/include/c++/i686-w64-mingw32"),
        t.builtIn("C:/mingw530/i686-w64-mingw32/include/c++/backward")};
    CompilerOptionsBuilder compilerOptionsBuilder(t.finalize(), UseSystemHeader::No,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "/dummy");
    compilerOptionsBuilder.addHeaderPathOptions();

    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++",
                          "-isystem", t.toNative("C:/mingw530/i686-w64-mingw32/include/c++"),
                          "-isystem", t.toNative("C:/mingw530/i686-w64-mingw32/include/c++/i686-w64-mingw32"),
                          "-isystem", t.toNative("C:/mingw530/i686-w64-mingw32/include/c++/backward"),
                          "-isystem", t.toNative("/dummy"),
                          "-isystem", t.toNative("C:/mingw530/i686-w64-mingw32/include")}));
}

void CompilerOptionsBuilderTest::testClangHeadersAndCppIncludePathsOrderAndroidClang()
{
    TestHelper t;
    t.targetTriple = "i686-linux-android";
    t.headerPaths = {
        t.builtIn("C:/Android/sdk/ndk-bundle/sysroot/usr/include/i686-linux-android"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++/include"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sources/android/support/include"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++abi/include"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sysroot/usr/include")};
    CompilerOptionsBuilder compilerOptionsBuilder(t.finalize(), UseSystemHeader::No,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "/dummy");
    compilerOptionsBuilder.addHeaderPathOptions();

    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++",
                          "-isystem", t.toNative("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++/include"),
                          "-isystem", t.toNative("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++abi/include"),
                          "-isystem", t.toNative("/dummy"),
                          "-isystem", t.toNative("C:/Android/sdk/ndk-bundle/sysroot/usr/include/i686-linux-android"),
                          "-isystem", t.toNative("C:/Android/sdk/ndk-bundle/sources/android/support/include"),
                          "-isystem", t.toNative("C:/Android/sdk/ndk-bundle/sysroot/usr/include")}));
}

void CompilerOptionsBuilderTest::testNoPrecompiledHeader()
{
    TestHelper t;
    t.finalize();
    t.compilerOptionsBuilder->addPrecompiledHeaderOptions(UsePrecompiledHeaders::No);

    QVERIFY(t.compilerOptionsBuilder->options().empty());
}

void CompilerOptionsBuilderTest::testUsePrecompiledHeader()
{
    TestHelper t;
    t.finalize();
    t.compilerOptionsBuilder->addPrecompiledHeaderOptions(UsePrecompiledHeaders::Yes);

    QCOMPARE(t.compilerOptionsBuilder->options(), (QStringList{"-include", t.pchFileNativePath()}));
}

void CompilerOptionsBuilderTest::testUsePrecompiledHeaderMsvc()
{
    TestHelper t;
    t.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    CompilerOptionsBuilder compilerOptionsBuilder{t.finalize()};
    compilerOptionsBuilder.evaluateCompilerFlags();
    compilerOptionsBuilder.addPrecompiledHeaderOptions(UsePrecompiledHeaders::Yes);

    QCOMPARE(compilerOptionsBuilder.options(), (QStringList{"/FI", t.pchFileNativePath()}));
}

void CompilerOptionsBuilderTest::testAddMacros()
{
    TestHelper t;
    t.finalize();
    t.compilerOptionsBuilder->addMacros(Macros{Macro{"key", "value"}});

    QCOMPARE(t.compilerOptionsBuilder->options(), QStringList("-Dkey=value"));
}

void CompilerOptionsBuilderTest::testAddTargetTriple()
{
    TestHelper t;
    t.finalize();
    t.compilerOptionsBuilder->addTargetTriple();

    QCOMPARE(t.compilerOptionsBuilder->options(), QStringList("--target=x86_64-apple-darwin10"));
}

void CompilerOptionsBuilderTest::testEnableCExceptions()
{
    TestHelper t;
    t.languageVersion = Utils::LanguageVersion::C99;
    t.finalize();
    t.compilerOptionsBuilder->enableExceptions();

    QCOMPARE(t.compilerOptionsBuilder->options(), QStringList("-fexceptions"));
}

void CompilerOptionsBuilderTest::testEnableCxxExceptions()
{
    TestHelper t;
    t.finalize();
    t.compilerOptionsBuilder->enableExceptions();

    QCOMPARE(t.compilerOptionsBuilder->options(), (QStringList{"-fcxx-exceptions", "-fexceptions"}));
}

void CompilerOptionsBuilderTest::testInsertWrappedQtHeaders()
{
    TestHelper t;
    CompilerOptionsBuilder compilerOptionsBuilder{t.finalize(), UseSystemHeader::Yes,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No};
    compilerOptionsBuilder.insertWrappedQtHeaders();

    QVERIFY(Utils::contains(compilerOptionsBuilder.options(),
                            [](const QString &o) { return o.contains("wrappedQtHeaders"); }));
}

void CompilerOptionsBuilderTest::testInsertWrappedMingwHeadersWithNonMingwToolchain()
{
    TestHelper t;
    CompilerOptionsBuilder builder{t.finalize(), UseSystemHeader::Yes, UseTweakedHeaderPaths::Yes,
                UseLanguageDefines::No, UseBuildSystemWarnings::No};
    builder.insertWrappedMingwHeaders();

    QVERIFY(!Utils::contains(builder.options(),
                            [](const QString &o) { return o.contains("wrappedMingwHeaders"); }));
}

void CompilerOptionsBuilderTest::testInsertWrappedMingwHeadersWithMingwToolchain()
{
    TestHelper t;
    t.toolchainType = Constants::MINGW_TOOLCHAIN_TYPEID;
    CompilerOptionsBuilder builder{t.finalize(), UseSystemHeader::Yes, UseTweakedHeaderPaths::Yes,
                UseLanguageDefines::No, UseBuildSystemWarnings::No};
    builder.insertWrappedMingwHeaders();

    QVERIFY(Utils::contains(builder.options(),
                            [](const QString &o) { return o.contains("wrappedMingwHeaders"); }));
}

void CompilerOptionsBuilderTest::testSetLanguageVersion()
{
    TestHelper t;
    t.finalize();
    t.compilerOptionsBuilder->updateFileLanguage(ProjectFile::CXXSource);

    QCOMPARE(t.compilerOptionsBuilder->options(), (QStringList{"-x", "c++"}));
}

void CompilerOptionsBuilderTest::testSetLanguageVersionMsvc()
{
    TestHelper t;
    t.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    CompilerOptionsBuilder compilerOptionsBuilder{t.finalize()};
    compilerOptionsBuilder.evaluateCompilerFlags();
    compilerOptionsBuilder.updateFileLanguage(ProjectFile::CXXSource);

    QCOMPARE(compilerOptionsBuilder.options(), QStringList("/TP"));
}

void CompilerOptionsBuilderTest::testHandleLanguageExtension()
{
    TestHelper t;
    t.languageVersion = Utils::LanguageVersion::CXX17;
    t.languageExtensions = Utils::LanguageExtension::ObjectiveC;
    t.finalize();
    t.compilerOptionsBuilder->updateFileLanguage(ProjectFile::CXXSource);

    QCOMPARE(t.compilerOptionsBuilder->options(), (QStringList{"-x", "objective-c++"}));
}

void CompilerOptionsBuilderTest::testUpdateLanguageVersion()
{
    TestHelper t;
    t.finalize();
    t.compilerOptionsBuilder->updateFileLanguage(ProjectFile::CXXSource);
    t.compilerOptionsBuilder->updateFileLanguage(ProjectFile::CXXHeader);

    QCOMPARE(t.compilerOptionsBuilder->options(), (QStringList{"-x", "c++-header"}));
}

void CompilerOptionsBuilderTest::testUpdateLanguageVersionMsvc()
{
    TestHelper t;
    t.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    CompilerOptionsBuilder compilerOptionsBuilder{t.finalize()};
    compilerOptionsBuilder.evaluateCompilerFlags();
    compilerOptionsBuilder.updateFileLanguage(ProjectFile::CXXSource);
    compilerOptionsBuilder.updateFileLanguage(ProjectFile::CSource);

    QCOMPARE(compilerOptionsBuilder.options(), QStringList("/TC"));
}

void CompilerOptionsBuilderTest::testAddMsvcCompatibilityVersion()
{
    TestHelper t;
    t.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    t.toolchainMacros.append(Macro{"_MSC_FULL_VER", "190000000"});
    t.finalize();
    t.compilerOptionsBuilder->addMsvcCompatibilityVersion();

    QCOMPARE(t.compilerOptionsBuilder->options(), QStringList("-fms-compatibility-version=19.00"));
}

void CompilerOptionsBuilderTest::testUndefineCppLanguageFeatureMacrosForMsvc2015()
{
    TestHelper t;
    t.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    t.isMsvc2015 = true;
    t.finalize();
    t.compilerOptionsBuilder->undefineCppLanguageFeatureMacrosForMsvc2015();

    QVERIFY(t.compilerOptionsBuilder->options().contains("-U__cpp_aggregate_bases"));
}

void CompilerOptionsBuilderTest::testAddDefineFunctionMacrosMsvc()
{
    TestHelper t;
    t.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    t.finalize();
    t.compilerOptionsBuilder->addDefineFunctionMacrosMsvc();

    QVERIFY(t.compilerOptionsBuilder->options().contains(
        "-D__FUNCTION__=\"someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580\""));
}

void CompilerOptionsBuilderTest::testAddProjectConfigFileInclude()
{
    TestHelper t;
    t.projectConfigFile = "dummy_file.h";
    t.finalize();
    t.compilerOptionsBuilder->addProjectConfigFileInclude();

    QCOMPARE(t.compilerOptionsBuilder->options(), (QStringList{"-include", "dummy_file.h"}));
}

void CompilerOptionsBuilderTest::testAddProjectConfigFileIncludeMsvc()
{
    TestHelper t;
    t.projectConfigFile = "dummy_file.h";
    t.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    CompilerOptionsBuilder compilerOptionsBuilder{t.finalize()};
    compilerOptionsBuilder.evaluateCompilerFlags();
    compilerOptionsBuilder.addProjectConfigFileInclude();

    QCOMPARE(compilerOptionsBuilder.options(), (QStringList{"/FI", "dummy_file.h"}));
}

void CompilerOptionsBuilderTest::testNoUndefineClangVersionMacrosForNewMsvc()
{
    TestHelper t;
    t.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    t.finalize();
    t.compilerOptionsBuilder->undefineClangVersionMacrosForMsvc();

    QVERIFY(!t.compilerOptionsBuilder->options().contains("-U__clang__"));
}

void CompilerOptionsBuilderTest::testUndefineClangVersionMacrosForOldMsvc()
{
    TestHelper t;
    t.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    t.toolchainMacros = {Macro{"_MSC_FULL_VER", "1300"}, Macro{"_MSC_VER", "13"}};
    t.finalize();
    t.compilerOptionsBuilder->undefineClangVersionMacrosForMsvc();

    QVERIFY(t.compilerOptionsBuilder->options().contains("-U__clang__"));
}

void CompilerOptionsBuilderTest::testBuildAllOptions()
{
    TestHelper t;
    t.extraFlags = QStringList{"-arch", "x86_64"};
    CompilerOptionsBuilder compilerOptionsBuilder(t.finalize(), UseSystemHeader::No,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "/dummy");
    compilerOptionsBuilder.build(ProjectFile::CXXSource, UsePrecompiledHeaders::No);

    const QString wrappedQtHeadersPath = Utils::findOrDefault(compilerOptionsBuilder.options(),
            [](const QString &o) { return o.contains("wrappedQtHeaders"); });
    const QString wrappedQtCoreHeadersPath = Utils::findOrDefault(compilerOptionsBuilder.options(),
            [&t](const QString &o) { return o.contains(t.toNative("wrappedQtHeaders/QtCore")); });
    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++", "-arch", "x86_64", "-fsyntax-only", "-m64",
                          "--target=x86_64-apple-darwin10", "-x", "c++", "-std=c++17",
                          "-DprojectFoo=projectBar",
                          "-DQT_ANNOTATE_FUNCTION(x)=__attribute__((annotate(#x)))",
                          wrappedQtHeadersPath,         // contains -I already
                          wrappedQtCoreHeadersPath,     // contains -I already
                          "-I" + t.toNative("/tmp/path"),
                          "-I" + t.toNative("/tmp/system_path"),
                          "-isystem", t.toNative("/dummy"),
                          "-isystem", t.toNative("/tmp/builtin_path")}));
}

void CompilerOptionsBuilderTest::testBuildAllOptionsMsvc()
{
    TestHelper t;
    t.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    CompilerOptionsBuilder compilerOptionsBuilder(t.finalize(), UseSystemHeader::No,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "/dummy");
    compilerOptionsBuilder.build(ProjectFile::CXXSource, UsePrecompiledHeaders::No);

    const QString wrappedQtHeadersPath = Utils::findOrDefault(compilerOptionsBuilder.options(),
            [](const QString &o) { return o.contains("wrappedQtHeaders"); });
    const QString wrappedQtCoreHeadersPath = Utils::findOrDefault(compilerOptionsBuilder.options(),
            [&t](const QString &o) { return o.contains(t.toNative("wrappedQtHeaders/QtCore")); });
    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++", "--driver-mode=cl", "/Zs", "-m64",
                          "--target=x86_64-apple-darwin10", "/TP", "-clang:-std=c++17",
                          "-fms-compatibility-version=19.00", "-DprojectFoo=projectBar",
                          "-D__FUNCSIG__=\"void __cdecl someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580(void)\"",
                          "-D__FUNCTION__=\"someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580\"",
                          "-D__FUNCDNAME__=\"?someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580@@YAXXZ\"",
                          "-DQT_ANNOTATE_FUNCTION(x)=__attribute__((annotate(#x)))",
                          wrappedQtHeadersPath,         // contains -I already
                          wrappedQtCoreHeadersPath,     // contains -I already
                          "-I" + t.toNative("/tmp/path"),
                          "-I" + t.toNative("/tmp/system_path"),
                          "/clang:-isystem", "/clang:" + t.toNative("/dummy"),
                          "/clang:-isystem", "/clang:" + t.toNative("/tmp/builtin_path")}));
}

void CompilerOptionsBuilderTest::testBuildAllOptionsMsvcWithExceptions()
{
    TestHelper t;
    t.toolchainType = Constants::MSVC_TOOLCHAIN_TYPEID;
    t.toolchainMacros.append(Macro{"_CPPUNWIND", "1"});
    CompilerOptionsBuilder compilerOptionsBuilder(t.finalize(), UseSystemHeader::No,
                UseTweakedHeaderPaths::Yes, UseLanguageDefines::No, UseBuildSystemWarnings::No,
                "/dummy");
    compilerOptionsBuilder.build(ProjectFile::CXXSource, UsePrecompiledHeaders::No);

    const QString wrappedQtHeadersPath = Utils::findOrDefault(compilerOptionsBuilder.options(),
            [](const QString &o) { return o.contains("wrappedQtHeaders"); });
    const QString wrappedQtCoreHeadersPath = Utils::findOrDefault(compilerOptionsBuilder.options(),
            [&t](const QString &o) { return o.contains(t.toNative("wrappedQtHeaders/QtCore")); });
    QCOMPARE(compilerOptionsBuilder.options(),
             (QStringList{"-nostdinc", "-nostdinc++", "--driver-mode=cl", "/Zs", "-m64",
                          "--target=x86_64-apple-darwin10", "/TP", "-clang:-std=c++17", "-fcxx-exceptions",
                          "-fexceptions", "-fms-compatibility-version=19.00",
                          "-DprojectFoo=projectBar",
                          "-D__FUNCSIG__=\"void __cdecl someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580(void)\"",
                          "-D__FUNCTION__=\"someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580\"",
                          "-D__FUNCDNAME__=\"?someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580@@YAXXZ\"",
                          "-DQT_ANNOTATE_FUNCTION(x)=__attribute__((annotate(#x)))",
                          wrappedQtHeadersPath,             // contains -I already
                          wrappedQtCoreHeadersPath,         // contains -I already
                          "-I" + t.toNative("/tmp/path"),
                          "-I" + t.toNative("/tmp/system_path"),
                          "/clang:-isystem", "/clang:" + t.toNative("/dummy"),
                          "/clang:-isystem", "/clang:" + t.toNative("/tmp/builtin_path")}));
}

} // namespace CppEditor::Internal
