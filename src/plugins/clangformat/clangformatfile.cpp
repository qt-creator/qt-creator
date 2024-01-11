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

    if (!parseConfigurationFile(m_filePath, m_style))
        resetStyleToQtC(preferences);
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
