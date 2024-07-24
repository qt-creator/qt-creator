// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatfile.h"
#include "clangformatutils.h"

#include <cppeditor/cppcodestylepreferences.h>
#include <cppeditor/cppcodestylesettings.h>

#include <projectexplorer/project.h>

#include <texteditor/icodestylepreferences.h>
#include <texteditor/tabsettings.h>

#include <utils/qtcassert.h>

#include <fstream>

using namespace ClangFormat;

ClangFormatFile::ClangFormatFile(
    const TextEditor::ICodeStylePreferences *preferences, const Utils::FilePath &fromFilePath)
    : m_filePath(filePathToCurrentSettings(preferences))
{
    if (m_filePath.exists())
        return;

    // create folder
    m_filePath.parentDir().createDir();

    if (fromFilePath.exists() && fromFilePath.copyFile(m_filePath)) {
        auto fileContent = m_filePath.fileContents();
        if (fileContent && !fileContent->contains("yaml-language-server")) {
            fileContent->insert(
                0,
                "# yaml-language-server: "
                "$schema=https://json.schemastore.org/clang-format.json\n");
            m_filePath.writeFileContents(fileContent.value());
        }
        parseConfigurationFile(m_filePath, m_style);
        return;
    }

    // It is done separately from folder because filePath.copyFile doesn't copy if file exists
    // ToDo: when behavior of copyFile is changed, combine that block with folder creation block
    // create file
    std::fstream newStyleFile(m_filePath.path().toStdString(), std::fstream::out);
    if (newStyleFile.is_open())
        newStyleFile.close();

    if (preferences->displayName() == "GNU") { // For build-in GNU style
        m_style = clang::format::getGNUStyle();
        saveStyleToFile(m_style, m_filePath);
        return;
    }

    resetStyleToQtC(preferences);
}

Utils::FilePath ClangFormatFile::filePath()
{
    return m_filePath;
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

void ClangFormatFile::saveStyleToFile(clang::format::FormatStyle style, Utils::FilePath filePath)
{
    std::string styleStr = clang::format::configurationAsText(style);

    // workaround: configurationAsText() add comment "# " before BasedOnStyle line
    const int pos = styleStr.find("# BasedOnStyle");
    if (pos != int(std::string::npos))
        styleStr.erase(pos, 2);
    styleStr.append("\n");
    styleStr
        .insert(0,
                "# yaml-language-server: $schema=https://json.schemastore.org/clang-format.json\n");
    filePath.writeFileContents(QByteArray::fromStdString(styleStr));
}

void ClangFormatFile::removeClangFormatFileForStylePreferences(
    const TextEditor::ICodeStylePreferences *preferences)
{
    filePathToCurrentSettings(preferences).parentDir().removeRecursively();
}

void ClangFormatFile::copyClangFormatFileBasedOnStylePreferences(
    const TextEditor::ICodeStylePreferences *current,
    const TextEditor::ICodeStylePreferences *target)
{
    Utils::FilePath currentFP = filePathToCurrentSettings(current);
    if (!currentFP.exists())
        return;

    Utils::FilePath targetFP = filePathToCurrentSettings(target);
    targetFP.parentDir().createDir();
    currentFP.copyFile(targetFP);
}
