/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "googletest.h"

#include <clangformat/clangformatbaseindenter.h>
#include <utils/fileutils.h>

#include <QTextDocument>

namespace TextEditor {
class TabSettings
{
};
}

namespace {

class ClangFormatIndenter : public ClangFormat::ClangFormatBaseIndenter
{
public:
    ClangFormatIndenter(QTextDocument *doc)
        : ClangFormat::ClangFormatBaseIndenter(doc)
    {}

    Utils::optional<TextEditor::TabSettings> tabSettings() const override
    {
        return Utils::optional<TextEditor::TabSettings>();
    }
};

class ClangFormat : public ::testing::Test
{
protected:
    void SetUp() final
    {
        indenter.setFileName(Utils::FileName::fromString(TESTDATA_DIR"/test.cpp"));
    }

    void insertLines(const std::vector<QString> &lines)
    {
        cursor.setPosition(0);
        for (size_t lineNumber = 1; lineNumber <= lines.size(); ++lineNumber) {
            if (lineNumber > 1)
                cursor.insertBlock();

            cursor.insertText(lines[lineNumber - 1]);
        }
        cursor.setPosition(0);
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    }

    std::vector<QString> documentLines()
    {
        std::vector<QString> result;
        const int lines = doc.blockCount();
        result.reserve(static_cast<size_t>(lines));
        for (int line = 0; line < lines; ++line)
            result.push_back(doc.findBlockByNumber(line).text());

        return result;
    }

    QTextDocument doc;
    ClangFormatIndenter indenter{&doc};
    QTextCursor cursor{&doc};
};

TEST_F(ClangFormat, SimpleIndent)
{
    insertLines({"int main()",
                 "{",
                 "int a;",
                 "}"});

    indenter.indent(cursor, QChar::Null, TextEditor::TabSettings());

    ASSERT_THAT(documentLines(), ElementsAre("int main()",
                                             "{",
                                             "    int a;",
                                             "}"));
}

}
