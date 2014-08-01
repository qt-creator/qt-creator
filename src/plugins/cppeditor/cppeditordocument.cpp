/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppeditordocument.h"

#include "cppeditorconstants.h"
#include "cpphighlighter.h"

#include <cpptools/cppcodeformatter.h>
#include <cpptools/cppqtstyleindenter.h>
#include <cpptools/cpptoolsconstants.h>

#include <QTextDocument>

namespace CppEditor {
namespace Internal {

CPPEditorDocument::CPPEditorDocument()
{
    setId(CppEditor::Constants::CPPEDITOR_ID);
    connect(this, SIGNAL(tabSettingsChanged()),
            this, SLOT(invalidateFormatterCache()));
    connect(this, SIGNAL(mimeTypeChanged()),
            this, SLOT(onMimeTypeChanged()));
    setSyntaxHighlighter(new CppHighlighter);
    setIndenter(new CppTools::CppQtStyleIndenter);
    onMimeTypeChanged();
}

bool CPPEditorDocument::isObjCEnabled() const
{
    return m_isObjCEnabled;
}

void CPPEditorDocument::applyFontSettings()
{
    if (TextEditor::SyntaxHighlighter *highlighter = syntaxHighlighter()) {
        // Clear all additional formats since they may have changed
        QTextBlock b = document()->firstBlock();
        while (b.isValid()) {
            QList<QTextLayout::FormatRange> noFormats;
            highlighter->setExtraAdditionalFormats(b, noFormats);
            b = b.next();
        }
    }
    BaseTextDocument::applyFontSettings(); // rehighlights and updates additional formats
}

void CPPEditorDocument::invalidateFormatterCache()
{
    CppTools::QtStyleCodeFormatter formatter;
    formatter.invalidateCache(document());
}

void CPPEditorDocument::onMimeTypeChanged()
{
    const QString &mt = mimeType();
    m_isObjCEnabled = (mt == QLatin1String(CppTools::Constants::OBJECTIVE_C_SOURCE_MIMETYPE)
                   || mt == QLatin1String(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE));
}

} // namespace Internal
} // namespace CppEditor
