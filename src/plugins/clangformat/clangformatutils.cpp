/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "clangformatutils.h"

#include "clangformatconstants.h"
#include "clangformatsettings.h"

#include <coreplugin/icore.h>
#include <cpptools/cppcodestylesettings.h>
#include <texteditor/tabsettings.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <QCryptographicHash>

using namespace clang;
using namespace format;
using namespace llvm;
using namespace CppTools;
using namespace ProjectExplorer;
using namespace TextEditor;

namespace ClangFormat {

static clang::format::FormatStyle qtcStyle()
{
    clang::format::FormatStyle style = getLLVMStyle();
    style.Language = FormatStyle::LK_Cpp;
    style.AccessModifierOffset = -4;
    style.AlignAfterOpenBracket = FormatStyle::BAS_Align;
    style.AlignConsecutiveAssignments = false;
    style.AlignConsecutiveDeclarations = false;
    style.AlignEscapedNewlines = FormatStyle::ENAS_DontAlign;
    style.AlignOperands = true;
    style.AlignTrailingComments = true;
    style.AllowAllParametersOfDeclarationOnNextLine = true;
    style.AllowShortBlocksOnASingleLine = false;
    style.AllowShortCaseLabelsOnASingleLine = false;
    style.AllowShortFunctionsOnASingleLine = FormatStyle::SFS_Inline;
    style.AllowShortIfStatementsOnASingleLine = false;
    style.AllowShortLoopsOnASingleLine = false;
    style.AlwaysBreakAfterReturnType = FormatStyle::RTBS_None;
    style.AlwaysBreakBeforeMultilineStrings = false;
    style.AlwaysBreakTemplateDeclarations = FormatStyle::BTDS_Yes;
    style.BinPackArguments = false;
    style.BinPackParameters = false;
    style.BraceWrapping.AfterClass = true;
    style.BraceWrapping.AfterControlStatement = false;
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
    style.ConstructorInitializerAllOnOneLineOrOnePerLine = false;
    style.ConstructorInitializerIndentWidth = 4;
    style.ContinuationIndentWidth = 4;
    style.Cpp11BracedListStyle = true;
    style.DerivePointerAlignment = false;
    style.DisableFormat = false;
    style.ExperimentalAutoDetectBinPacking = false;
    style.FixNamespaceComments = true;
    style.ForEachMacros = {"forever", "foreach", "Q_FOREACH", "BOOST_FOREACH"};
    style.IncludeStyle.IncludeCategories = {{"^<Q.*", 200}};
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
    style.SortIncludes = true;
    style.SortUsingDeclarations = true;
    style.SpaceAfterCStyleCast = true;
    style.SpaceAfterTemplateKeyword = false;
    style.SpaceBeforeAssignmentOperators = true;
    style.SpaceBeforeParens = FormatStyle::SBPO_ControlStatements;
    style.SpaceInEmptyParentheses = false;
    style.SpacesBeforeTrailingComments = 1;
    style.SpacesInAngles = false;
    style.SpacesInContainerLiterals = false;
    style.SpacesInCStyleCastParentheses = false;
    style.SpacesInParentheses = false;
    style.SpacesInSquareBrackets = false;
    style.Standard = FormatStyle::LS_Cpp11;
    style.TabWidth = 4;
    style.UseTab = FormatStyle::UT_Never;
    return style;
}

static bool useGlobalOverriddenSettings()
{
    return ClangFormatSettings::instance().overrideDefaultFile();
}

QString currentProjectUniqueId()
{
    const Project *project = SessionManager::startupProject();
    if (!project)
        return QString();

    return QString::fromUtf8(QCryptographicHash::hash(project->projectFilePath().toString().toUtf8(),
                                                      QCryptographicHash::Md5)
                                 .toHex(0));
}

static bool useProjectOverriddenSettings()
{
    const Project *project = SessionManager::startupProject();
    return project ? project->namedSettings(Constants::OVERRIDE_FILE_ID).toBool() : false;
}

static Utils::FileName globalPath()
{
    return Utils::FileName::fromString(Core::ICore::userResourcePath());
}

static Utils::FileName projectPath()
{
    const Project *project = SessionManager::startupProject();
    if (project)
        return globalPath().appendPath("clang-format").appendPath(currentProjectUniqueId());

    return Utils::FileName();
}

static QString findConfig(Utils::FileName fileName)
{
    QDir parentDir(fileName.parentDir().toString());
    while (!parentDir.exists(Constants::SETTINGS_FILE_NAME)
           && !parentDir.exists(Constants::SETTINGS_FILE_ALT_NAME)) {
        if (!parentDir.cdUp())
            return QString();
    }

    if (parentDir.exists(Constants::SETTINGS_FILE_NAME))
        return parentDir.filePath(Constants::SETTINGS_FILE_NAME);
    return parentDir.filePath(Constants::SETTINGS_FILE_ALT_NAME);
}

static QString configForFile(Utils::FileName fileName, bool checkForSettings)
{
    QDir overrideDir;
    if (!checkForSettings || useProjectOverriddenSettings()) {
        overrideDir = projectPath().toString();
        if (!overrideDir.isEmpty() && overrideDir.exists(Constants::SETTINGS_FILE_NAME))
            return overrideDir.filePath(Constants::SETTINGS_FILE_NAME);
    }

    if (!checkForSettings || useGlobalOverriddenSettings()) {
        overrideDir = globalPath().toString();
        if (!overrideDir.isEmpty() && overrideDir.exists(Constants::SETTINGS_FILE_NAME))
            return overrideDir.filePath(Constants::SETTINGS_FILE_NAME);
    }

    return findConfig(fileName);
}

QString configForFile(Utils::FileName fileName)
{
    return configForFile(fileName, true);
}

Utils::FileName assumedPathForConfig(const QString &configFile)
{
    Utils::FileName fileName = Utils::FileName::fromString(configFile);
    return fileName.parentDir().appendPath("test.cpp");
}

static clang::format::FormatStyle constructStyle(const QByteArray &baseStyle = QByteArray())
{
    if (!baseStyle.isEmpty()) {
        // Try to get the style for this base style.
        Expected<FormatStyle> style = getStyle(baseStyle.toStdString(),
                                               "dummy.cpp",
                                               baseStyle.toStdString());
        if (style)
            return *style;

        handleAllErrors(style.takeError(), [](const ErrorInfoBase &) {
            // do nothing
        });
        // Fallthrough to the default style.
    }

    return qtcStyle();
}

void createStyleFileIfNeeded(bool isGlobal)
{
    Utils::FileName path = isGlobal ? globalPath() : projectPath();
    const QString configFile = path.appendPath(Constants::SETTINGS_FILE_NAME).toString();

    if (QFile::exists(configFile))
        return;

    QDir().mkpath(path.parentDir().toString());
    if (!isGlobal) {
        const Project *project = SessionManager::startupProject();
        Utils::FileName possibleProjectConfig = project->rootProjectDirectory().appendPath(
            Constants::SETTINGS_FILE_NAME);
        if (possibleProjectConfig.exists()) {
            // Just copy th .clang-format if current project has one.
            QFile::copy(possibleProjectConfig.toString(), configFile);
            return;
        }
    }

    std::fstream newStyleFile(configFile.toStdString(), std::fstream::out);
    if (newStyleFile.is_open()) {
        newStyleFile << clang::format::configurationAsText(constructStyle());
        newStyleFile.close();
    }
}

static QByteArray configBaseStyleName(const QString &configFile)
{
    if (configFile.isEmpty())
        return QByteArray();

    QFile config(configFile);
    if (!config.open(QIODevice::ReadOnly))
        return QByteArray();

    const QByteArray content = config.readAll();
    const char basedOnStyle[] = "BasedOnStyle:";
    int basedOnStyleIndex = content.indexOf(basedOnStyle);
    if (basedOnStyleIndex < 0)
        return QByteArray();

    basedOnStyleIndex += sizeof(basedOnStyle) - 1;
    const int endOfLineIndex = content.indexOf('\n', basedOnStyleIndex);
    return content
        .mid(basedOnStyleIndex, endOfLineIndex < 0 ? -1 : endOfLineIndex - basedOnStyleIndex)
        .trimmed();
}

static clang::format::FormatStyle styleForFile(Utils::FileName fileName, bool checkForSettings)
{
    QString configFile = configForFile(fileName, checkForSettings);
    if (configFile.isEmpty()) {
        // If no configuration is found create a global one (if it does not yet exist) and use it.
        createStyleFileIfNeeded(true);
        configFile = globalPath().appendPath(Constants::SETTINGS_FILE_NAME).toString();
    }

    fileName = assumedPathForConfig(configFile);
    Expected<FormatStyle> style = format::getStyle("file",
                                                   fileName.toString().toStdString(),
                                                   "none");
    if (style)
        return *style;

    handleAllErrors(style.takeError(), [](const ErrorInfoBase &) {
        // do nothing
    });

    return constructStyle(configBaseStyleName(configFile));
}

clang::format::FormatStyle styleForFile(Utils::FileName fileName)
{
    return styleForFile(fileName, true);
}

clang::format::FormatStyle currentProjectStyle()
{
    return styleForFile(projectPath().appendPath(Constants::SAMPLE_FILE_NAME), false);
}

clang::format::FormatStyle currentGlobalStyle()
{
    return styleForFile(globalPath().appendPath(Constants::SAMPLE_FILE_NAME), false);
}
} // namespace ClangFormat
