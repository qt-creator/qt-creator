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

#include "doxygengenerator.h"

#include <texteditor/convenience.h>

#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/CppDocument.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QTextDocument>
#include <QDebug>

#include <limits>

using namespace CppTools;
using namespace CPlusPlus;

DoxygenGenerator::DoxygenGenerator()
    : m_addLeadingAsterisks(true)
    , m_generateBrief(true)
    , m_startComment(true)
    , m_style(QtStyle)
{}

void DoxygenGenerator::setStyle(DocumentationStyle style)
{
    m_style = style;
}

void DoxygenGenerator::setStartComment(bool start)
{
    m_startComment = start;
}

void DoxygenGenerator::setGenerateBrief(bool get)
{
    m_generateBrief = get;
}

void DoxygenGenerator::setAddLeadingAsterisks(bool add)
{
    m_addLeadingAsterisks = add;
}

static int lineBeforeCursor(const QTextCursor &cursor)
{
    int line, column;
    const bool converted = TextEditor::Convenience::convertPosition(cursor.document(),
                                                                    cursor.position(),
                                                                    &line,
                                                                    &column);
    QTC_ASSERT(converted, return std::numeric_limits<int>::max());

    return line - 1;
}

QString DoxygenGenerator::generate(QTextCursor cursor,
                                   const CPlusPlus::Snapshot &snapshot,
                                   const Utils::FileName &documentFilePath)
{
    const QTextCursor initialCursor = cursor;

    const QChar &c = cursor.document()->characterAt(cursor.position());
    if (!c.isLetter() && c != QLatin1Char('_'))
        return QString();

    // Try to find what would be the declaration we are interested in.
    SimpleLexer lexer;
    QTextBlock block = cursor.block();
    while (block.isValid()) {
        const QString &text = block.text();
        const Tokens &tks = lexer(text);
        foreach (const Token &tk, tks) {
            if (tk.is(T_SEMICOLON) || tk.is(T_LBRACE)) {
                // No need to continue beyond this, we might already have something meaningful.
                cursor.setPosition(block.position() + tk.utf16charsEnd(), QTextCursor::KeepAnchor);
                break;
            }
        }

        if (cursor.hasSelection())
            break;

        block = block.next();
    }

    if (!cursor.hasSelection())
        return QString();

    QString declCandidate = cursor.selectedText();
    declCandidate.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));

    // Let's append a closing brace in the case we got content like 'class MyType {'
    if (declCandidate.endsWith(QLatin1Char('{')))
        declCandidate.append(QLatin1Char('}'));

    Document::Ptr doc = snapshot.preprocessedDocument(declCandidate.toUtf8(),
                                                      documentFilePath,
                                                      lineBeforeCursor(initialCursor));
    doc->parse(Document::ParseDeclaration);
    doc->check(Document::FastCheck);

    if (!doc->translationUnit()
            || !doc->translationUnit()->ast()
            || !doc->translationUnit()->ast()->asDeclaration()) {
        return QString();
    }

    return generate(cursor, doc->translationUnit()->ast()->asDeclaration());
}

QString DoxygenGenerator::generate(QTextCursor cursor, DeclarationAST *decl)
{
    SpecifierAST *spec = 0;
    DeclaratorAST *decltr = 0;
    if (SimpleDeclarationAST *simpleDecl = decl->asSimpleDeclaration()) {
        if (simpleDecl->declarator_list
                && simpleDecl->declarator_list->value) {
            decltr = simpleDecl->declarator_list->value;
        } else if (simpleDecl->decl_specifier_list
                   && simpleDecl->decl_specifier_list->value) {
            spec = simpleDecl->decl_specifier_list->value;
        }
    } else if (FunctionDefinitionAST * defDecl = decl->asFunctionDefinition()) {
        decltr = defDecl->declarator;
    }

    assignCommentOffset(cursor);

    QString comment;
    if (m_startComment)
        writeStart(&comment);
    writeNewLine(&comment);
    writeContinuation(&comment);

    if (decltr
            && decltr->core_declarator
            && decltr->core_declarator->asDeclaratorId()
            && decltr->core_declarator->asDeclaratorId()->name) {
        CoreDeclaratorAST *coreDecl = decltr->core_declarator;
        if (m_generateBrief)
            writeBrief(&comment, m_printer.prettyName(coreDecl->asDeclaratorId()->name->name));
        else
            writeNewLine(&comment);

        if (decltr->postfix_declarator_list
                && decltr->postfix_declarator_list->value
                && decltr->postfix_declarator_list->value->asFunctionDeclarator()) {
            FunctionDeclaratorAST *funcDecltr =
                    decltr->postfix_declarator_list->value->asFunctionDeclarator();
            if (funcDecltr->parameter_declaration_clause
                    && funcDecltr->parameter_declaration_clause->parameter_declaration_list) {
                for (ParameterDeclarationListAST *it =
                        funcDecltr->parameter_declaration_clause->parameter_declaration_list;
                     it;
                     it = it->next) {
                    ParameterDeclarationAST *paramDecl = it->value;
                    if (paramDecl->declarator
                            && paramDecl->declarator->core_declarator
                            && paramDecl->declarator->core_declarator->asDeclaratorId()
                            && paramDecl->declarator->core_declarator->asDeclaratorId()->name) {
                        DeclaratorIdAST *paramId =
                                paramDecl->declarator->core_declarator->asDeclaratorId();
                        writeContinuation(&comment);
                        writeCommand(&comment,
                                     ParamCommand,
                                     m_printer.prettyName(paramId->name->name));
                    }
                }
            }
            if (funcDecltr->symbol
                    && funcDecltr->symbol->returnType().type()
                    && !funcDecltr->symbol->returnType()->isVoidType()
                    && !funcDecltr->symbol->returnType()->isUndefinedType()) {
                writeContinuation(&comment);
                writeCommand(&comment, ReturnCommand);
            }
        }
    } else if (spec && m_generateBrief) {
        bool briefWritten = false;
        if (ClassSpecifierAST *classSpec = spec->asClassSpecifier()) {
            if (classSpec->name) {
                QString aggregate;
                if (classSpec->symbol->isClass())
                    aggregate = QLatin1String("class");
                else if (classSpec->symbol->isStruct())
                    aggregate = QLatin1String("struct");
                else
                    aggregate = QLatin1String("union");
                writeBrief(&comment,
                           m_printer.prettyName(classSpec->name->name),
                           QLatin1String("The"),
                           aggregate);
                briefWritten = true;
            }
        } else if (EnumSpecifierAST *enumSpec = spec->asEnumSpecifier()) {
            if (enumSpec->name) {
                writeBrief(&comment,
                           m_printer.prettyName(enumSpec->name->name),
                           QLatin1String("The"),
                           QLatin1String("enum"));
                briefWritten = true;
            }
        }
        if (!briefWritten)
            writeNewLine(&comment);
    } else {
        writeNewLine(&comment);
    }

    writeEnd(&comment);

    return comment;
}

QChar DoxygenGenerator::startMark() const
{
    if (m_style == QtStyle)
        return QLatin1Char('!');
    return QLatin1Char('*');
}

QChar DoxygenGenerator::styleMark() const
{
    if (m_style == QtStyle || m_style == CppStyleA || m_style == CppStyleB)
        return QLatin1Char('\\');
    return QLatin1Char('@');
}

QString DoxygenGenerator::commandSpelling(Command command)
{
    if (command == ParamCommand)
        return QLatin1String("param ");
    if (command == ReturnCommand)
        return QLatin1String("return ");

    QTC_ASSERT(command == BriefCommand, return QString());
    return QLatin1String("brief ");
}

void DoxygenGenerator::writeStart(QString *comment) const
{
    if (m_style == CppStyleA)
        comment->append(QLatin1String("///"));
    if (m_style == CppStyleB)
        comment->append(QLatin1String("//!"));
    else
        comment->append(offsetString() + "/*" + startMark());
}

void DoxygenGenerator::writeEnd(QString *comment) const
{
    if (m_style == CppStyleA)
        comment->append(QLatin1String("///"));
    else if (m_style == CppStyleB)
        comment->append(QLatin1String("//!"));
    else
        comment->append(offsetString() + " */");
}

void DoxygenGenerator::writeContinuation(QString *comment) const
{
    if (m_style == CppStyleA)
        comment->append(offsetString() + "///");
    else if (m_style == CppStyleB)
        comment->append(offsetString() + "//!");
    else if (m_addLeadingAsterisks)
        comment->append(offsetString() + " *");
    else
        comment->append(offsetString() + "  ");
}

void DoxygenGenerator::writeNewLine(QString *comment) const
{
    comment->append(QLatin1Char('\n'));
}

void DoxygenGenerator::writeCommand(QString *comment,
                                    Command command,
                                    const QString &commandContent) const
{
    comment->append(' ' + styleMark() + commandSpelling(command) + commandContent + '\n');
}

void DoxygenGenerator::writeBrief(QString *comment,
                                  const QString &brief,
                                  const QString &prefix,
                                  const QString &suffix)
{
    QString content = prefix + ' ' + brief + ' ' + suffix;
    writeCommand(comment, BriefCommand, content.trimmed());
}

void DoxygenGenerator::assignCommentOffset(QTextCursor cursor)
{
    if (cursor.hasSelection()) {
        if (cursor.anchor() < cursor.position())
            cursor.setPosition(cursor.anchor());
    }

    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    m_commentOffset = cursor.selectedText();
}

QString DoxygenGenerator::offsetString() const
{
    return m_commentOffset;
}
