/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "clangformatfile.h"
#include <sstream>

using namespace ClangFormat;

ClangFormatFile::ClangFormatFile(Utils::FilePath filePath)
    : m_filePath(filePath)
{
    if (!m_filePath.exists()) {
        resetStyleToLLVM();
        return;
    }

    m_style.Language = clang::format::FormatStyle::LK_Cpp;
    const std::error_code error
        = clang::format::parseConfiguration(m_filePath.fileContents().toStdString(), &m_style);
    if (error.value() != static_cast<int>(clang::format::ParseError::Success)) {
        resetStyleToLLVM();
    }
}

clang::format::FormatStyle ClangFormatFile::format() {
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

void ClangFormatFile::resetStyleToLLVM()
{
    m_style = clang::format::getLLVMStyle();
    saveNewFormat();
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
    std::string style = clang::format::configurationAsText(m_style);

    // workaround: configurationAsText() add comment "# " before BasedOnStyle line
    const int pos = style.find("# BasedOnStyle");
    if (pos < style.size())
        style.erase(pos, 2);
    m_filePath.writeFileContents(QByteArray::fromStdString(style));
}

void ClangFormatFile::saveNewFormat(QByteArray style)
{
    m_filePath.writeFileContents(style);
}
