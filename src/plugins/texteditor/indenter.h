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

#pragma once

#include <utils/fileutils.h>
#include <utils/optional.h>
#include <utils/textutils.h>

#include <QMap>
#include <QTextBlock>
#include <vector>

namespace Utils {
class FilePath;
}

namespace TextEditor {

class ICodeStylePreferences;
class TabSettings;

using IndentationForBlock = QMap<int, int>;

class RangeInLines
{
public:
    int startLine;
    int endLine;
};

using RangesInLines = std::vector<RangeInLines>;

class Indenter
{
public:
    explicit Indenter(QTextDocument *doc)
        : m_doc(doc)
    {}

    void setFileName(const Utils::FilePath &fileName) { m_fileName = fileName; }

    virtual ~Indenter() = default;

    // Returns true if key triggers an indent.
    virtual bool isElectricCharacter(const QChar & /*ch*/) const { return false; }

    virtual void setCodeStylePreferences(ICodeStylePreferences * /*preferences*/) {}

    virtual void invalidateCache() {}

    virtual int indentFor(const QTextBlock & /*block*/,
                          const TabSettings & /*tabSettings*/,
                          int /*cursorPositionInEditor*/ = -1)
    {
        return -1;
    }

    virtual void autoIndent(const QTextCursor &cursor,
                            const TabSettings &tabSettings,
                            int cursorPositionInEditor = -1)
    {
        indent(cursor, QChar::Null, tabSettings, cursorPositionInEditor);
    }

    virtual Utils::Text::Replacements format(const RangesInLines & /*rangesInLines*/)
    {
        return Utils::Text::Replacements();
    }

    virtual bool formatOnSave() const { return false; }

    // Expects a list of blocks in order of occurrence in the document.
    virtual IndentationForBlock indentationForBlocks(const QVector<QTextBlock> &blocks,
                                                     const TabSettings &tabSettings,
                                                     int cursorPositionInEditor = -1)
        = 0;
    virtual Utils::optional<TabSettings> tabSettings() const = 0;

    // Indent a text block based on previous line. Default does nothing
    virtual void indentBlock(const QTextBlock &block,
                             const QChar &typedChar,
                             const TabSettings &tabSettings,
                             int cursorPositionInEditor = -1)
        = 0;

    // Indent at cursor. Calls indentBlock for selection or current line.
    virtual void indent(const QTextCursor &cursor,
                        const QChar &typedChar,
                        const TabSettings &tabSettings,
                        int cursorPositionInEditor = -1)
        = 0;

    // Reindent at cursor. Selection will be adjusted according to the indentation
    // change of the first block.
    virtual void reindent(const QTextCursor &cursor,
                          const TabSettings &tabSettings,
                          int cursorPositionInEditor = -1)
        = 0;

protected:
    QTextDocument *m_doc;
    Utils::FilePath m_fileName;
};

} // namespace TextEditor
