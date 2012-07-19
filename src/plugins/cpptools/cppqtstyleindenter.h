/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
