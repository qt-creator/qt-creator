// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatutils.h"

#include "clangformatconstants.h"

#include <coreplugin/icore.h>

#include <cppeditor/cppcodestylesettings.h>

#include <texteditor/icodestylepreferences.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>

#include <projectexplorer/editorconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <utils/qtcassert.h>

#include <QCryptographicHash>

using namespace clang;
using namespace format;
using namespace llvm;
using namespace CppEditor;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace ClangFormat {

clang::format::FormatStyle qtcStyle()
{
    clang::format::FormatStyle style = getLLVMStyle();
    style.Language = FormatStyle::LK_Cpp;
    style.AccessModifierOffset = -4;
    style.AlignAfterOpenBracket = FormatStyle::BAS_Align;
#if LLVM_VERSION_MAJOR >= 15
    style.AlignConsecutiveAssignments = {false, false, false, false, false};
    style.AlignConsecutiveDeclarations = {false, false, false, false, false};
#elif LLVM_VERSION_MAJOR >= 12
    style.AlignConsecutiveAssignments = FormatStyle::ACS_None;
    style.AlignConsecutiveDeclarations = FormatStyle::ACS_None;
#else
    style.AlignConsecutiveAssignments = false;
    style.AlignConsecutiveDeclarations = false;
#endif
    style.AlignEscapedNewlines = FormatStyle::ENAS_DontAlign;
#if LLVM_VERSION_MAJOR >= 11
    style.AlignOperands = FormatStyle::OAS_Align;
#else
    style.AlignOperands = true;
#endif
#if LLVM_VERSION_MAJOR >= 16
    style.AlignTrailingComments = {FormatStyle::TCAS_Always, 0};
#else
    style.AlignTrailingComments = true;
#endif
    style.AllowAllParametersOfDeclarationOnNextLine = true;
#if LLVM_VERSION_MAJOR >= 10
    style.AllowShortBlocksOnASingleLine = FormatStyle::SBS_Never;
#else
    style.AllowShortBlocksOnASingleLine = false;
#endif
    style.AllowShortCaseLabelsOnASingleLine = false;
    style.AllowShortFunctionsOnASingleLine = FormatStyle::SFS_Inline;
#if LLVM_VERSION_MAJOR >= 9
    style.AllowShortIfStatementsOnASingleLine = FormatStyle::SIS_Never;
#else
    style.AllowShortIfStatementsOnASingleLine = false;
#endif
    style.AllowShortLoopsOnASingleLine = false;
    style.AlwaysBreakAfterReturnType = FormatStyle::RTBS_None;
    style.AlwaysBreakBeforeMultilineStrings = false;
    style.AlwaysBreakTemplateDeclarations = FormatStyle::BTDS_Yes;
    style.BinPackArguments = false;
    style.BinPackParameters = false;
    style.BraceWrapping.AfterClass = true;
#if LLVM_VERSION_MAJOR >= 10
    style.BraceWrapping.AfterControlStatement = FormatStyle::BWACS_Never;
#else
    style.BraceWrapping.AfterControlStatement = false;
#endif
    style.BraceWrapping.AfterEnum = false;
    style.BraceWrapping.AfterFunction = true;
    style.BraceWrapping.AfterNamespace = false;
    style.BraceWrapping.AfterObjCDeclaration = false;
    style.BraceWrapping.AfterStruct = true;
    style.BraceWrapping.AfterUnion = false;
    style.BraceWrapping.BeforeCatch = false;
    style.BraceWrapping.BeforeElse = false;
    style.BraceWrapping.IndentBraces = false;
    style.BraceWrapping.SplitEmptyFunction = false;
    style.BraceWrapping.SplitEmptyRecord = false;
    style.BraceWrapping.SplitEmptyNamespace = false;
    style.BreakBeforeBinaryOperators = FormatStyle::BOS_All;
    style.BreakBeforeBraces = FormatStyle::BS_Custom;
    style.BreakBeforeTernaryOperators = true;
    style.BreakConstructorInitializers = FormatStyle::BCIS_BeforeComma;
    style.BreakAfterJavaFieldAnnotations = false;
    style.BreakStringLiterals = true;
    style.ColumnLimit = 100;
    style.CommentPragmas = "^ IWYU pragma:";
    style.CompactNamespaces = false;
#if LLVM_VERSION_MAJOR >= 15
    style.PackConstructorInitializers = FormatStyle::PCIS_BinPack;
#else
    style.ConstructorInitializerAllOnOneLineOrOnePerLine = false;
#endif
    style.ConstructorInitializerIndentWidth = 4;
    style.ContinuationIndentWidth = 4;
    style.Cpp11BracedListStyle = true;
    style.DerivePointerAlignment = false;
    style.DisableFormat = false;
    style.ExperimentalAutoDetectBinPacking = false;
    style.FixNamespaceComments = true;
    style.ForEachMacros = {"forever", "foreach", "Q_FOREACH", "BOOST_FOREACH"};
#if LLVM_VERSION_MAJOR >= 12
    style.IncludeStyle.IncludeCategories = {{"^<Q.*", 200, 200, true}};
#else
    style.IncludeStyle.IncludeCategories = {{"^<Q.*", 200, 200}};
#endif
    style.IncludeStyle.IncludeIsMainRegex = "(Test)?$";
    style.IndentCaseLabels = false;
    style.IndentWidth = 4;
    style.IndentWrappedFunctionNames = false;
    style.JavaScriptQuotes = FormatStyle::JSQS_Leave;
    style.JavaScriptWrapImports = true;
    style.KeepEmptyLinesAtTheStartOfBlocks = false;
    // Do not add QT_BEGIN_NAMESPACE/QT_END_NAMESPACE as this will indent lines in between.
    style.MacroBlockBegin = "";
    style.MacroBlockEnd = "";
    style.MaxEmptyLinesToKeep = 1;
    style.NamespaceIndentation = FormatStyle::NI_None;
    style.ObjCBlockIndentWidth = 4;
    style.ObjCSpaceAfterProperty = false;
    style.ObjCSpaceBeforeProtocolList = true;
    style.PenaltyBreakAssignment = 150;
    style.PenaltyBreakBeforeFirstCallParameter = 300;
    style.PenaltyBreakComment = 500;
    style.PenaltyBreakFirstLessLess = 400;
    style.PenaltyBreakString = 600;
    style.PenaltyExcessCharacter = 50;
    style.PenaltyReturnTypeOnItsOwnLine = 300;
    style.PointerAlignment = FormatStyle::PAS_Right;
    style.ReflowComments = false;
#if LLVM_VERSION_MAJOR >= 13
    style.SortIncludes = FormatStyle::SI_CaseSensitive;
#else
    style.SortIncludes = true;
#endif
#if LLVM_VERSION_MAJOR >= 16
    style.SortUsingDeclarations = FormatStyle::SUD_Lexicographic;
#else
    style.SortUsingDeclarations = true;
#endif
    style.SpaceAfterCStyleCast = true;
    style.SpaceAfterTemplateKeyword = false;
    style.SpaceBeforeAssignmentOperators = true;
    style.SpaceBeforeParens = FormatStyle::SBPO_ControlStatements;
    style.SpaceInEmptyParentheses = false;
    style.SpacesBeforeTrailingComments = 1;
#if LLVM_VERSION_MAJOR >= 13
    style.SpacesInAngles = FormatStyle::SIAS_Never;
#else
    style.SpacesInAngles = false;
#endif
    style.SpacesInContainerLiterals = false;
    style.SpacesInCStyleCastParentheses = false;
    style.SpacesInParentheses = false;
    style.SpacesInSquareBrackets = false;
    style.StatementMacros.emplace_back("Q_OBJECT");
    style.StatementMacros.emplace_back("QT_BEGIN_NAMESPACE");
    style.StatementMacros.emplace_back("QT_END_NAMESPACE");
    style.Standard = FormatStyle::LS_Cpp11;
    style.TabWidth = 4;
    style.UseTab = FormatStyle::UT_Never;
    return style;
}

QString projectUniqueId(ProjectExplorer::Project *project)
{
    if (!project)
        return QString();

    return QString::fromUtf8(QCryptographicHash::hash(project->projectFilePath().toString().toUtf8(),
                                                      QCryptographicHash::Md5)
                                 .toHex(0));
}

bool getProjectUseGlobalSettings(const ProjectExplorer::Project *project)
{
    const QVariant projectUseGlobalSettings = project ? project->namedSettings(
                                                  Constants::USE_GLOBAL_SETTINGS)
                                                      : QVariant();

    return projectUseGlobalSettings.isValid() ? projectUseGlobalSettings.toBool() : true;
}

bool getProjectOverriddenSettings(const ProjectExplorer::Project *project)
{
    const QVariant projectOverride = project ? project->namedSettings(Constants::OVERRIDE_FILE_ID)
                                             : QVariant();

    return projectOverride.isValid()
               ? projectOverride.toBool()
               : ClangFormatSettings::instance().overrideDefaultFile();
}

bool getCurrentOverriddenSettings(const Utils::FilePath &filePath)
{
    const ProjectExplorer::Project *project = ProjectExplorer::SessionManager::projectForFile(
        filePath);

    return getProjectUseGlobalSettings(project)
               ? ClangFormatSettings::instance().overrideDefaultFile()
               : getProjectOverriddenSettings(project);
}

ClangFormatSettings::Mode getProjectIndentationOrFormattingSettings(
    const ProjectExplorer::Project *project)
{
    const QVariant projectIndentationOrFormatting = project
                                                        ? project->namedSettings(Constants::MODE_ID)
                                                        : QVariant();

    return projectIndentationOrFormatting.isValid()
               ? static_cast<ClangFormatSettings::Mode>(projectIndentationOrFormatting.toInt())
               : ClangFormatSettings::instance().mode();
}

ClangFormatSettings::Mode getCurrentIndentationOrFormattingSettings(const Utils::FilePath &filePath)
{
    const ProjectExplorer::Project *project = ProjectExplorer::SessionManager::projectForFile(
        filePath);

    return getProjectUseGlobalSettings(project)
               ? ClangFormatSettings::instance().mode()
               : getProjectIndentationOrFormattingSettings(project);
}

Utils::FilePath findConfig(const Utils::FilePath &fileName)
{
    Utils::FilePath parentDirectory = fileName.parentDir();
    while (parentDirectory.exists()) {
        Utils::FilePath settingsFilePath = parentDirectory / Constants::SETTINGS_FILE_NAME;
        if (settingsFilePath.exists())
            return settingsFilePath;

        Utils::FilePath settingsAltFilePath = parentDirectory / Constants::SETTINGS_FILE_ALT_NAME;
        if (settingsAltFilePath.exists())
            return settingsAltFilePath;

        parentDirectory = parentDirectory.parentDir();
    }
    return Utils::FilePath();
}

Utils::FilePath configForFile(const Utils::FilePath &fileName)
{
    if (!getCurrentOverriddenSettings(fileName))
        return findConfig(fileName);

    const ProjectExplorer::Project *projectForFile
        = ProjectExplorer::SessionManager::projectForFile(fileName);

    const TextEditor::ICodeStylePreferences *preferences
        = projectForFile
              ? projectForFile->editorConfiguration()->codeStyle("Cpp")->currentPreferences()
              : TextEditor::TextEditorSettings::codeStyle("Cpp")->currentPreferences();

    return filePathToCurrentSettings(preferences);
}

void addQtcStatementMacros(clang::format::FormatStyle &style)
{
    static const std::vector<std::string> macros = {"Q_OBJECT",
                                                    "QT_BEGIN_NAMESPACE",
                                                    "QT_END_NAMESPACE"};
    for (const std::string &macro : macros) {
        if (std::find(style.StatementMacros.begin(), style.StatementMacros.end(), macro)
            == style.StatementMacros.end())
            style.StatementMacros.emplace_back(macro);
    }
}

Utils::FilePath filePathToCurrentSettings(const TextEditor::ICodeStylePreferences *codeStyle)
{
    return Core::ICore::userResourcePath() / "clang-format/"
           / Utils::FileUtils::fileSystemFriendlyName(codeStyle->displayName())
           / QLatin1String(Constants::SETTINGS_FILE_NAME);
}
} // namespace ClangFormat
