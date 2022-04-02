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

#include "basetexteditmodifier.h"

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljstools/qmljsindenter.h>
#include <qmljstools/qmljscodestylepreferences.h>
#include <qmljseditor/qmljseditordocument.h>
#include <qmljseditor/qmljscomponentfromobjectdef.h>
#include <qmljseditor/qmljscompletionassist.h>
#include <qmljstools/qmljstoolssettings.h>
#include <texteditor/tabsettings.h>
#include <utils/changeset.h>

#include <typeinfo>

using namespace QmlDesigner;

BaseTextEditModifier::BaseTextEditModifier(TextEditor::TextEditorWidget *textEdit)
    : PlainTextEditModifier(textEdit)
    , m_textEdit{textEdit}
{
}

void BaseTextEditModifier::indentLines(int startLine, int endLine)
{
    if (startLine < 0)
        return;

    if (!m_textEdit)
        return;

    TextEditor::TextDocument *baseTextEditorDocument = m_textEdit->textDocument();
    TextEditor::TabSettings tabSettings = baseTextEditorDocument->tabSettings();
    QTextCursor tc(textDocument());

    tc.beginEditBlock();
    for (int i = startLine; i <= endLine; i++) {
        QTextBlock start = textDocument()->findBlockByNumber(i);

        if (start.isValid()) {
            QmlJSEditor::Internal::Indenter indenter(textDocument());
            indenter.indentBlock(start, QChar::Null, tabSettings);
        }
    }
    tc.endEditBlock();
}

void BaseTextEditModifier::indent(int offset, int length)
{
    if (length == 0 || offset < 0 || offset + length >= text().length())
        return;

    int startLine = getLineInDocument(textDocument(), offset);
    int endLine = getLineInDocument(textDocument(), offset + length);

    if (startLine > -1 && endLine > -1)
        indentLines(startLine, endLine);
}

TextEditor::TabSettings BaseTextEditModifier::tabSettings() const
{
    if (m_textEdit)
        return m_textEdit->textDocument()->tabSettings();
    return QmlJSTools::QmlJSToolsSettings::globalCodeStyle()->tabSettings();
}

bool BaseTextEditModifier::renameId(const QString &oldId, const QString &newId)
{
    if (m_textEdit) {
        if (auto document = qobject_cast<QmlJSEditor::QmlJSEditorDocument *>(
                m_textEdit->textDocument())) {
            Utils::ChangeSet changeSet;
            foreach (const QmlJS::SourceLocation &loc,
                    document->semanticInfo().idLocations.value(oldId)) {
                changeSet.replace(loc.begin(), loc.end(), newId);
            }
            QTextCursor tc = textCursor();
            changeSet.apply(&tc);
            return true;
        }
    }
    return false;
}

static QmlJS::AST::UiObjectDefinition *getObjectDefinition(const QList<QmlJS::AST::Node *> &path, QmlJS::AST::UiQualifiedId *qualifiedId)
{
    QmlJS::AST::UiObjectDefinition *object = nullptr;
    for (int i = path.size() - 1; i >= 0; --i) {
        auto node = path.at(i);
        if (auto objDef =  QmlJS::AST::cast<QmlJS::AST::UiObjectDefinition *>(node)) {
            if (objDef->qualifiedTypeNameId == qualifiedId)
                object = objDef;
        }
    }
    return object;
}

bool BaseTextEditModifier::moveToComponent(int nodeOffset)
{
    if (m_textEdit) {
        if (auto document = qobject_cast<QmlJSEditor::QmlJSEditorDocument *>(
                m_textEdit->textDocument())) {
            auto qualifiedId = QmlJS::AST::cast<QmlJS::AST::UiQualifiedId *>(document->semanticInfo().astNodeAt(nodeOffset));
            QList<QmlJS::AST::Node *> path = document->semanticInfo().rangePath(nodeOffset);
            QmlJS::AST::UiObjectDefinition *object = getObjectDefinition(path, qualifiedId);

            if (!object)
                return false;

            QmlJSEditor::performComponentFromObjectDef(document->filePath().toString(), object);
            return true;
        }
    }
    return false;
}

QStringList BaseTextEditModifier::autoComplete(QTextDocument *textDocument, int position, bool explicitComplete)
{
    if (m_textEdit)
        if (auto document = qobject_cast<QmlJSEditor::QmlJSEditorDocument *>(
                m_textEdit->textDocument()))
            return QmlJSEditor::qmlJSAutoComplete(textDocument,
                                                  position,
                                                  document->filePath(),
                                                  explicitComplete ? TextEditor::ExplicitlyInvoked : TextEditor::ActivationCharacter,
                                                  document->semanticInfo());
    return QStringList();
}
