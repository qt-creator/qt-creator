// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "doxygengenerator.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/SimpleLexer.h>

#include <utils/textutils.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

#include <limits>

using namespace CPlusPlus;

namespace CppEditor::Internal {

DoxygenGenerator::DoxygenGenerator() = default;

QString DoxygenGenerator::generate(QTextCursor cursor,
                                   const CPlusPlus::Snapshot &snapshot,
                                   const Utils::FilePath &documentFilePath)
{
    const QTextCursor initialCursor = cursor;

    const QChar &c = cursor.document()->characterAt(cursor.position());
    if (!c.isLetter() && c != QLatin1Char('_') && c != QLatin1Char('['))
        return QString();

    // Try to find what would be the declaration we are interested in.
    SimpleLexer lexer;
    QTextBlock block = cursor.block();
    while (block.isValid()) {
        const QString &text = block.text();
        const Tokens &tks = lexer(text);
        for (const Token &tk : tks) {
            if (tk.is(T_SEMICOLON)) {
                // No need to continue beyond this, we might already have something meaningful.
                cursor.setPosition(block.position() + tk.utf16charsEnd(), QTextCursor::KeepAnchor);
                break;
            }
        }

        if (cursor.hasSelection())
            break;

        block = block.next();
    }

    // For the edge case of no semicolons at all, which can e.g. happen if the file
    // consists only of empty function definitions.
    if (!cursor.hasSelection())
        cursor.setPosition(cursor.document()->characterCount() - 1, QTextCursor::KeepAnchor);

    if (!cursor.hasSelection())
        return QString();

    QString declCandidate = cursor.selectedText();

    // remove attributes like [[nodiscard]] because
    // Document::Ptr::parse(Document::ParseDeclaration) fails on attributes
    static QRegularExpression attribute("\\[\\s*\\[.*\\]\\s*\\]");
    declCandidate.replace(attribute, "");

    declCandidate.replace("Q_INVOKABLE", "");
    declCandidate.remove(QRegularExpression(R"(\s*(public|protected|private)\s*:\s*)"));
    declCandidate.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));

    // Let's append a closing brace in the case we got content like 'class MyType {'
    if (declCandidate.endsWith(QLatin1Char('{')))
        declCandidate.append(QLatin1Char('}'));

    Document::Ptr doc = snapshot.preprocessedDocument(declCandidate.toUtf8(),
                                                      documentFilePath,
                                                      cursor.blockNumber());
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
    if (const TemplateDeclarationAST * const templDecl = decl->asTemplateDeclaration();
            templDecl && templDecl->declaration) {
        decl = templDecl->declaration;
    }

    SpecifierAST *spec = nullptr;
    DeclaratorAST *decltr = nullptr;
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
    writeNewLine(&comment);
    writeContinuation(&comment);

    if (decltr
            && decltr->core_declarator
            && decltr->core_declarator->asDeclaratorId()
            && decltr->core_declarator->asDeclaratorId()->name) {
        CoreDeclaratorAST *coreDecl = decltr->core_declarator;
        if (m_settings.generateBrief)
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
                    && !funcDecltr->symbol->returnType()->asVoidType()
                    && !funcDecltr->symbol->returnType()->isUndefinedType()) {
                writeContinuation(&comment);
                writeCommand(&comment, ReturnCommand);
            }
        }
    } else if (spec && m_settings.generateBrief) {
        bool briefWritten = false;
        if (ClassSpecifierAST *classSpec = spec->asClassSpecifier()) {
            if (classSpec->name) {
                QString aggregate;
                if (classSpec->symbol->asClass())
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

QChar DoxygenGenerator::styleMark() const
{
    switch (m_settings.commandPrefix) {
    case TextEditor::CommentsSettings::CommandPrefix::At: return '@';
    case TextEditor::CommentsSettings::CommandPrefix::Backslash: return '\\';
    case TextEditor::CommentsSettings::CommandPrefix::Auto: break;
    }

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
    else if (m_settings.leadingAsterisks)
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

} // namespace CppEditor::Internal
