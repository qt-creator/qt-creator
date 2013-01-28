/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "doxygengenerator.h"

#include <cplusplus/SimpleLexer.h>
#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/Token.h>
#include <cplusplus/TranslationUnit.h>
#include <cplusplus/AST.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/Scope.h>
#include <cplusplus/LookupContext.h>

#include <utils/qtcassert.h>

#include <QStringBuilder>
#include <QTextDocument>
#include <QDebug>

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

QString DoxygenGenerator::generate(QTextCursor cursor)
{
    const QChar &c = cursor.document()->characterAt(cursor.position());
    if (!c.isLetter() && c != QLatin1Char('_'))
        return QString();

    // Try to find what would be the declaration we are interested in.
    SimpleLexer lexer;
    QTextBlock block = cursor.block();
    while (block.isValid()) {
        const QString &text = block.text();
        const QList<Token> &tks = lexer(text);
        foreach (const Token &tk, tks) {
            if (tk.is(T_SEMICOLON) || tk.is(T_LBRACE)) {
                // No need to continue beyond this, we might already have something meaningful.
                cursor.setPosition(block.position() + tk.end(), QTextCursor::KeepAnchor);
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

    Document::Ptr doc = Document::create(QLatin1String("<doxygen>"));
    doc->setUtf8Source(declCandidate.toUtf8());
    doc->parse(Document::ParseDeclaration);
    doc->check(Document::FastCheck);

    if (!doc->translationUnit()
            || !doc->translationUnit()->ast()
            || !doc->translationUnit()->ast()->asDeclaration()) {
        return QString();
    }

    return generate(cursor, doc->translationUnit()->ast()->asDeclaration());
}

QString DoxygenGenerator::generate(QTextCursor cursor, CPlusPlus::DeclarationAST *decl)
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
    if (m_style == QtStyle)
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
    comment->append(offsetString() % QLatin1String("/*") % startMark());
}

void DoxygenGenerator::writeEnd(QString *comment) const
{
    comment->append(offsetString() % QLatin1String(" */"));
}

void DoxygenGenerator::writeContinuation(QString *comment) const
{
    if (m_addLeadingAsterisks)
        comment->append(offsetString() % QLatin1String(" *"));
    else
        comment->append(offsetString() % QLatin1String("  "));
}

void DoxygenGenerator::writeNewLine(QString *comment) const
{
    comment->append(QLatin1Char('\n'));
}

void DoxygenGenerator::writeCommand(QString *comment,
                                    Command command,
                                    const QString &commandContent) const
{
    comment->append(QLatin1Char(' ')
                    % styleMark()
                    % commandSpelling(command)
                    % commandContent
                    % QLatin1Char('\n'));
}

void DoxygenGenerator::writeBrief(QString *comment,
                                  const QString &brief,
                                  const QString &prefix,
                                  const QString &suffix)
{
    QString content = prefix % QLatin1Char(' ') % brief % QLatin1Char(' ') % suffix;
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
