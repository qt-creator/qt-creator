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

#pragma once

#include "utils/filepath.h"
#include <clang/Format/Format.h>

namespace CppEditor { class CppCodeStyleSettings; }
namespace ProjectExplorer { class Project; }
namespace TextEditor { class TabSettings; }

namespace ClangFormat {

class ClangFormatFile
{
public:
    explicit ClangFormatFile(Utils::FilePath file, bool isReadOnly);
    clang::format::FormatStyle format();

    Utils::FilePath filePath();
    void resetStyleToQtC();
    void setBasedOnStyle(QString styleName);
    void setStyle(clang::format::FormatStyle style);
    QString setStyle(QString style);
    void clearBasedOnStyle();

    using Field = std::pair<QString, QString>;
    QString changeFields(QList<Field> fields);
    QString changeField(Field field);
    CppEditor::CppCodeStyleSettings toCppCodeStyleSettings(ProjectExplorer::Project *project) const;
    TextEditor::TabSettings toTabSettings(ProjectExplorer::Project *project) const;
    void fromCppCodeStyleSettings(const CppEditor::CppCodeStyleSettings &settings);
    void fromTabSettings(const TextEditor::TabSettings &settings);
    bool isReadOnly() const;

private:
    void saveNewFormat();
    void saveNewFormat(QByteArray style);

private:
    Utils::FilePath m_filePath;
    clang::format::FormatStyle m_style;
    const bool m_isReadOnly;
};

} // namespace ClangFormat
