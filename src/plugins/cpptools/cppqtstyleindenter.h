/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef CPPQTSTYLEINDENTER_H
#define CPPQTSTYLEINDENTER_H

#include "cpptools_global.h"

#include <texteditor/indenter.h>

namespace TextEditor
{
class ICodeStylePreferences;
}

namespace CppTools {
class CppCodeStyleSettings;
class CppCodeStylePreferences;

class CPPTOOLS_EXPORT CppQtStyleIndenter : public TextEditor::Indenter
{
public:
    CppQtStyleIndenter();
    virtual ~CppQtStyleIndenter();

    virtual bool isElectricCharacter(const QChar &ch) const;
    virtual void indentBlock(QTextDocument *doc,
                             const QTextBlock &block,
                             const QChar &typedChar,
                             const TextEditor::TabSettings &tabSettings);

    virtual void indent(QTextDocument *doc,
                        const QTextCursor &cursor,
                        const QChar &typedChar,
                        const TextEditor::TabSettings &tabSettings);

    virtual void setCodeStylePreferences(TextEditor::ICodeStylePreferences *preferences);
    virtual void invalidateCache(QTextDocument *doc);
private:
    CppCodeStyleSettings codeStyleSettings() const;
    CppCodeStylePreferences *m_cppCodeStylePreferences;
};

} // CppTools

#endif // CPPQTSTYLEINDENTER_H
