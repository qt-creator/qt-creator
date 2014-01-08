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

#include "highlighterutils.h"
#include "generichighlighter/highlighter.h"
#include "generichighlighter/highlightdefinition.h"
#include "generichighlighter/manager.h"
#include <coreplugin/icore.h>

using namespace TextEditor;
using namespace Internal;

QString TextEditor::findDefinitionId(const Core::MimeType &mimeType,
                         bool considerParents)
{
    QString definitionId = Manager::instance()->definitionIdByAnyMimeType(mimeType.aliases());
    if (definitionId.isEmpty() && considerParents) {
        definitionId = Manager::instance()->definitionIdByAnyMimeType(mimeType.subClassesOf());
        if (definitionId.isEmpty()) {
            foreach (const QString &parent, mimeType.subClassesOf()) {
                const Core::MimeType &parentMimeType = Core::MimeDatabase::findByType(parent);
                definitionId = findDefinitionId(parentMimeType, considerParents);
            }
        }
    }
    return definitionId;
}

void TextEditor::setMimeTypeForHighlighter(Highlighter *highlighter, const Core::MimeType &mimeType)
{
    const QString type = mimeType.type();
    QString definitionId = Manager::instance()->definitionIdByMimeType(type);
    if (definitionId.isEmpty())
        definitionId = findDefinitionId(mimeType, true);

    if (!definitionId.isEmpty()) {
        const QSharedPointer<HighlightDefinition> &definition =
            Manager::instance()->definition(definitionId);
        if (!definition.isNull() && definition->isValid())
            highlighter->setDefaultContext(definition->initialContext());
    }
}

SyntaxHighlighter *TextEditor::createGenericSyntaxHighlighter(const Core::MimeType &mimeType)
{
    TextEditor::Highlighter *highlighter = new TextEditor::Highlighter();
    setMimeTypeForHighlighter(highlighter, mimeType);
    return highlighter;
}
