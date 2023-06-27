// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatfile.h"
#include "clangformatsettings.h"
#include "clangformatutils.h"

#include <cppeditor/cppcodestylepreferences.h>
#include <cppeditor/cppcodestylesettings.h>

#include <projectexplorer/project.h>

#include <texteditor/icodestylepreferences.h>
#include <texteditor/tabsettings.h>

#include <utils/qtcassert.h>

#include <sstream>

using namespace ClangFormat;

ClangFormatFile::ClangFormatFile(const TextEditor::ICodeStylePreferences *preferences)
    : m_filePath(filePathToCurrentSettings(preferences))
{
    if (!m_filePath.exists()) {
        // create file and folder
        m_filePath.parentDir().createDir();
        std::fstream newStyleFile(m_filePath.path().toStdString(), std::fstream::out);
        if (newStyleFile.is_open()) {
            newStyleFile.close();
        }
        resetStyleToQtC(preferences);
        return;
    }

    m_style.Language = clang::format::FormatStyle::LK_Cpp;
    const std::error_code error = clang::format::parseConfiguration(m_filePath.fileContents()
                                                                        .value_or(QByteArray())
                                                                        .toStdString(),
                                                                    &m_style);
    if (error.value() != static_cast<int>(clang::format::ParseError::Success)) {
        resetStyleToQtC(preferences);
    }
}

clang::format::FormatStyle ClangFormatFile::style() {
    return m_style;
}

Utils::FilePath ClangFormatFile::filePath()
{
    return m_filePath;
}

void ClangFormatFile::setStyle(clang::format::FormatStyle style)
{
    m_style = style;
    saveNewFormat();
}

bool ClangFormatFile::isReadOnly() const
{
    return m_isReadOnly;
}

void ClangFormatFile::setIsReadOnly(bool isReadOnly)
{
    m_isReadOnly = isReadOnly;
}

void ClangFormatFile::resetStyleToQtC(const TextEditor::ICodeStylePreferences *preferences)
{
    m_style = currentQtStyle(preferences);
    saveStyleToFile(m_style, m_filePath);
}

void ClangFormatFile::setBasedOnStyle(QString styleName)
{
    changeField({"BasedOnStyle", styleName});
    saveNewFormat();
}

QString ClangFormatFile::setStyle(QString style)
{
    const std::error_code error = clang::format::parseConfiguration(style.toStdString(), &m_style);
    if (error.value() != static_cast<int>(clang::format::ParseError::Success)) {
        return QString::fromStdString(error.message());
    }

    saveNewFormat(style.toUtf8());
    return "";
}

QString ClangFormatFile::changeField(Field field)
{
    return changeFields({field});
}

QString ClangFormatFile::changeFields(QList<Field> fields)
{
    std::stringstream content;
    content << "---" << "\n";

    for (const auto &field : fields) {
        content << field.first.toStdString() << ": " << field.second.toStdString() << "\n";
    }

    return setStyle(QString::fromStdString(content.str()));
}

void ClangFormatFile::saveNewFormat()
{
    if (m_isReadOnly)
        return;

    saveStyleToFile(m_style, m_filePath);
}

void ClangFormatFile::saveNewFormat(QByteArray style)
{
    if (m_isReadOnly)
        return;

    m_filePath.writeFileContents(style);
}

void ClangFormatFile::saveStyleToFile(clang::format::FormatStyle style, Utils::FilePath filePath)
{
    std::string styleStr = clang::format::configurationAsText(style);

    // workaround: configurationAsText() add comment "# " before BasedOnStyle line
    const int pos = styleStr.find("# BasedOnStyle");
    if (pos != int(std::string::npos))
        styleStr.erase(pos, 2);
    styleStr.append("\n");
    filePath.writeFileContents(QByteArray::fromStdString(styleStr));
}

CppEditor::CppCodeStyleSettings ClangFormatFile::toCppCodeStyleSettings(
    ProjectExplorer::Project *project) const
{
    using namespace clang::format;
    auto settings = CppEditor::CppCodeStyleSettings::getProjectCodeStyle(project);

    FormatStyle style;
    style.Language = clang::format::FormatStyle::LK_Cpp;
    const std::error_code error
        = parseConfiguration(m_filePath.fileContents().value_or(QByteArray()).toStdString(), &style);
    QTC_ASSERT(error.value() == static_cast<int>(ParseError::Success), return settings);

    // Modifier offset should be opposite to indent width in order indentAccessSpecifiers
    // to be false
    settings.indentAccessSpecifiers = (style.AccessModifierOffset != -1 * int(style.IndentWidth));

    if (style.NamespaceIndentation == FormatStyle::NamespaceIndentationKind::NI_All) {
        settings.indentNamespaceBody = true;
        settings.indentNamespaceBraces = settings.indentNamespaceBody;
    }

    if (style.BreakBeforeBraces == FormatStyle::BS_Whitesmiths) {
        settings.indentClassBraces = true;
        settings.indentEnumBraces = settings.indentClassBraces;
        settings.indentBlockBraces = settings.indentClassBraces;
        settings.indentFunctionBraces = settings.indentClassBraces;
    }

    settings.indentSwitchLabels = style.IndentCaseLabels;
#if LLVM_VERSION_MAJOR >= 11
    settings.indentBlocksRelativeToSwitchLabels = style.IndentCaseBlocks;
#endif
    if (style.DerivePointerAlignment
        && ClangFormatSettings::instance().mode() == ClangFormatSettings::Mode::Formatting) {
        settings.bindStarToIdentifier = style.PointerAlignment == FormatStyle::PAS_Right;
        settings.bindStarToTypeName = style.PointerAlignment == FormatStyle::PAS_Left;
        settings.bindStarToLeftSpecifier = style.PointerAlignment == FormatStyle::PAS_Left;
        settings.bindStarToRightSpecifier = style.PointerAlignment == FormatStyle::PAS_Right;
    }

    settings.extraPaddingForConditionsIfConfusingAlign = style.BreakBeforeBinaryOperators
                                                         == FormatStyle::BOS_All;
    settings.alignAssignments = style.BreakBeforeBinaryOperators == FormatStyle::BOS_All
                                || style.BreakBeforeBinaryOperators
                                       == FormatStyle::BOS_NonAssignment;

    return settings;
}

void ClangFormatFile::fromCppCodeStyleSettings(const CppEditor::CppCodeStyleSettings &settings)
{
    ::fromCppCodeStyleSettings(m_style, settings);
    saveNewFormat();
}

TextEditor::TabSettings ClangFormatFile::toTabSettings(ProjectExplorer::Project *project) const
{
    using namespace clang::format;
    auto settings = CppEditor::CppCodeStyleSettings::getProjectTabSettings(project);

    FormatStyle style;
    style.Language = clang::format::FormatStyle::LK_Cpp;
    const std::error_code error
        = parseConfiguration(m_filePath.fileContents().value_or(QByteArray()).toStdString(), &style);
    QTC_ASSERT(error.value() == static_cast<int>(ParseError::Success), return settings);

    settings.m_indentSize = style.IndentWidth;
    settings.m_tabSize = style.TabWidth;

    switch (style.UseTab) {
#if LLVM_VERSION_MAJOR >= 11
    case FormatStyle::UT_AlignWithSpaces:
#endif
    case FormatStyle::UT_ForIndentation:
    case FormatStyle::UT_ForContinuationAndIndentation:
        settings.m_tabPolicy = TextEditor::TabSettings::TabPolicy::MixedTabPolicy;
        break;
    case FormatStyle::UT_Never:
        settings.m_tabPolicy = TextEditor::TabSettings::TabPolicy::SpacesOnlyTabPolicy;
        break;
    case FormatStyle::UT_Always:
        settings.m_tabPolicy = TextEditor::TabSettings::TabPolicy::TabsOnlyTabPolicy;
        break;
    }

    return settings;
}

void ClangFormatFile::fromTabSettings(const TextEditor::TabSettings &settings)
{
    ::fromTabSettings(m_style, settings);
    saveNewFormat();
}
