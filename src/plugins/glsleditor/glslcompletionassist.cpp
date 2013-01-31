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

#include "glslcompletionassist.h"
#include "glsleditorconstants.h"
#include "glsleditorplugin.h"
#include "reuse.h"

#include <glsl/glslengine.h>
#include <glsl/glslengine.h>
#include <glsl/glsllexer.h>
#include <glsl/glslparser.h>
#include <glsl/glslsemantic.h>
#include <glsl/glslsymbols.h>
#include <glsl/glslastdump.h>

#include <coreplugin/idocument.h>
#include <texteditor/completionsettings.h>
#include <texteditor/codeassist/basicproposalitem.h>
#include <texteditor/codeassist/basicproposalitemlistmodel.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/functionhintproposal.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <utils/faketooltip.h>

#include <QIcon>
#include <QPainter>
#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>
#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>

using namespace GLSLEditor;
using namespace Internal;
using namespace TextEditor;

namespace {

enum CompletionOrder {
    SpecialMemberOrder = -5
};


bool isActivationChar(const QChar &ch)
{
    return ch == QLatin1Char('(') || ch == QLatin1Char('.') || ch == QLatin1Char(',');
}

bool isIdentifierChar(QChar ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_');
}

bool isDelimiter(QChar ch)
{
    switch (ch.unicode()) {
    case '{':
    case '}':
    case '[':
    case ']':
    case ')':
    case '?':
    case '!':
    case ':':
    case ';':
    case ',':
    case '+':
    case '-':
    case '*':
    case '/':
        return true;

    default:
        return false;
    }
}

bool checkStartOfIdentifier(const QString &word)
{
    if (! word.isEmpty()) {
        const QChar ch = word.at(0);
        if (ch.isLetter() || ch == QLatin1Char('_'))
            return true;
    }

    return false;
}

} // Anonymous

// ----------------------------
// GLSLCompletionAssistProvider
// ----------------------------
bool GLSLCompletionAssistProvider::supportsEditor(const Core::Id &editorId) const
{
    return editorId == Constants::C_GLSLEDITOR_ID;
}

IAssistProcessor *GLSLCompletionAssistProvider::createProcessor() const
{
    return new GLSLCompletionAssistProcessor;
}

int GLSLCompletionAssistProvider::activationCharSequenceLength() const
{
    return 1;
}

bool GLSLCompletionAssistProvider::isActivationCharSequence(const QString &sequence) const
{
    return isActivationChar(sequence.at(0));
}

// -----------------------------
// GLSLFunctionHintProposalModel
// -----------------------------
class GLSLFunctionHintProposalModel : public TextEditor::IFunctionHintProposalModel
{
public:
    GLSLFunctionHintProposalModel(QVector<GLSL::Function *> functionSymbols)
        : m_items(functionSymbols)
        , m_currentArg(-1)
    {}

    virtual void reset() {}
    virtual int size() const { return m_items.size(); }
    virtual QString text(int index) const;
    virtual int activeArgument(const QString &prefix) const;

private:
    QVector<GLSL::Function *> m_items;
    mutable int m_currentArg;
};

QString GLSLFunctionHintProposalModel::text(int index) const
{
    return m_items.at(index)->prettyPrint(m_currentArg);
}

int GLSLFunctionHintProposalModel::activeArgument(const QString &prefix) const
{
    const QByteArray &str = prefix.toLatin1();
    int argnr = 0;
    int parcount = 0;
    GLSL::Lexer lexer(0, str.constData(), str.length());
    GLSL::Token tk;
    QList<GLSL::Token> tokens;
    do {
        lexer.yylex(&tk);
        tokens.append(tk);
    } while (tk.isNot(GLSL::Parser::EOF_SYMBOL));
    for (int i = 0; i < tokens.count(); ++i) {
        const GLSL::Token &tk = tokens.at(i);
        if (tk.is(GLSL::Parser::T_LEFT_PAREN))
            ++parcount;
        else if (tk.is(GLSL::Parser::T_RIGHT_PAREN))
            --parcount;
        else if (! parcount && tk.is(GLSL::Parser::T_COMMA))
            ++argnr;
    }

    if (parcount < 0)
        return -1;

    if (argnr != m_currentArg)
        m_currentArg = argnr;

    return argnr;
}

// -----------------------------
// GLSLCompletionAssistProcessor
// -----------------------------
GLSLCompletionAssistProcessor::GLSLCompletionAssistProcessor()
    : m_startPosition(0)
    , m_keywordIcon(QLatin1String(":/glsleditor/images/keyword.png"))
    , m_varIcon(QLatin1String(":/glsleditor/images/var.png"))
    , m_functionIcon(QLatin1String(":/glsleditor/images/func.png"))
    , m_typeIcon(QLatin1String(":/glsleditor/images/type.png"))
    , m_constIcon(QLatin1String(":/glsleditor/images/const.png"))
    , m_attributeIcon(QLatin1String(":/glsleditor/images/attribute.png"))
    , m_uniformIcon(QLatin1String(":/glsleditor/images/uniform.png"))
    , m_varyingIcon(QLatin1String(":/glsleditor/images/varying.png"))
    , m_otherIcon(QLatin1String(":/glsleditor/images/other.png"))
{}

GLSLCompletionAssistProcessor::~GLSLCompletionAssistProcessor()
{}

IAssistProposal *GLSLCompletionAssistProcessor::perform(const IAssistInterface *interface)
{
    m_interface.reset(static_cast<const GLSLCompletionAssistInterface *>(interface));

    if (interface->reason() == IdleEditor && !acceptsIdleEditor())
        return 0;

    int pos = m_interface->position() - 1;
    QChar ch = m_interface->characterAt(pos);
    while (ch.isLetterOrNumber() || ch == QLatin1Char('_'))
        ch = m_interface->characterAt(--pos);

    CPlusPlus::ExpressionUnderCursor expressionUnderCursor;
    //GLSLTextEditorWidget *edit = qobject_cast<GLSLTextEditorWidget *>(editor->widget());

    QList<GLSL::Symbol *> members;
    QStringList specialMembers;

    bool functionCall = (ch == QLatin1Char('(') && pos == m_interface->position() - 1);

    if (ch == QLatin1Char(',')) {
        QTextCursor tc(m_interface->textDocument());
        tc.setPosition(pos);
        const int start = expressionUnderCursor.startOfFunctionCall(tc);
        if (start == -1)
            return 0;

        if (m_interface->characterAt(start) == QLatin1Char('(')) {
            pos = start;
            ch = QLatin1Char('(');
            functionCall = true;
        }
    }

    if (ch == QLatin1Char('.') || functionCall) {
        const bool memberCompletion = ! functionCall;
        QTextCursor tc(m_interface->textDocument());
        tc.setPosition(pos);

        // get the expression under cursor
        const QByteArray code = expressionUnderCursor(tc).toLatin1();
        //qDebug() << endl << "expression:" << code;

        // parse the expression
        GLSL::Engine engine;
        GLSL::Parser parser(&engine, code, code.size(), languageVariant(m_interface->mimeType()));
        GLSL::ExpressionAST *expr = parser.parseExpression();

#if 0
        // dump it!
        QTextStream qout(stdout, QIODevice::WriteOnly);
        GLSL::ASTDump dump(qout);
        dump(expr);
#endif

        if (Document::Ptr doc = m_interface->glslDocument()) {
            GLSL::Scope *currentScope = doc->scopeAt(pos);

            GLSL::Semantic sem;
            GLSL::Semantic::ExprResult exprTy = sem.expression(expr, currentScope, doc->engine());
            if (exprTy.type) {
                if (memberCompletion) {
                    if (const GLSL::VectorType *vecTy = exprTy.type->asVectorType()) {
                        members = vecTy->members();

                        // Sort the most relevant swizzle orderings to the top.
                        specialMembers += QLatin1String("xy");
                        specialMembers += QLatin1String("xyz");
                        specialMembers += QLatin1String("xyzw");
                        specialMembers += QLatin1String("rgb");
                        specialMembers += QLatin1String("rgba");
                        specialMembers += QLatin1String("st");
                        specialMembers += QLatin1String("stp");
                        specialMembers += QLatin1String("stpq");

                    } else if (const GLSL::Struct *structTy = exprTy.type->asStructType()) {
                        members = structTy->members();

                    } else {
                        // some other type
                    }
                } else { // function completion
                    QVector<GLSL::Function *> signatures;
                    if (const GLSL::Function *funTy = exprTy.type->asFunctionType())
                        signatures.append(const_cast<GLSL::Function *>(funTy)); // ### get rid of the const_cast
                    else if (const GLSL::OverloadSet *overload = exprTy.type->asOverloadSetType())
                        signatures = overload->functions();

                    if (! signatures.isEmpty()) {
                        m_startPosition = pos + 1;
                        return createHintProposal(signatures);
                    }
                }
            } else {
                // undefined

            }

        } else {
            // sorry, there's no document
        }

    } else {
        // it's a global completion
        if (Document::Ptr doc = m_interface->glslDocument()) {
            GLSL::Scope *currentScope = doc->scopeAt(pos);
            bool isGlobal = !currentScope || !currentScope->scope();

            // add the members from the scope chain
            for (; currentScope; currentScope = currentScope->scope())
                members += currentScope->members();

            // if this is the global scope, then add some standard Qt attribute
            // and uniform names for autocompleting variable declarations
            // this isn't a complete list, just the most common
            if (isGlobal) {
                static const char * const attributeNames[] = {
                    "qt_Vertex",
                    "qt_Normal",
                    "qt_MultiTexCoord0",
                    "qt_MultiTexCoord1",
                    "qt_MultiTexCoord2",
                    0
                };
                static const char * const uniformNames[] = {
                    "qt_ModelViewProjectionMatrix",
                    "qt_ModelViewMatrix",
                    "qt_ProjectionMatrix",
                    "qt_NormalMatrix",
                    "qt_Texture0",
                    "qt_Texture1",
                    "qt_Texture2",
                    "qt_Color",
                    "qt_Opacity",
                    0
                };
                for (int index = 0; attributeNames[index]; ++index)
                    addCompletion(QString::fromLatin1(attributeNames[index]), m_attributeIcon);
                for (int index = 0; uniformNames[index]; ++index)
                    addCompletion(QString::fromLatin1(uniformNames[index]), m_uniformIcon);
            }
        }

 //       if (m_keywordVariant != languageVariant(m_interface->mimeType())) {
            QStringList keywords = GLSL::Lexer::keywords(languageVariant(m_interface->mimeType()));
//            m_keywordCompletions.clear();
            for (int index = 0; index < keywords.size(); ++index)
                addCompletion(keywords.at(index), m_keywordIcon);
//            m_keywordVariant = languageVariant(m_interface->mimeType());
//        }

  //      m_completions += m_keywordCompletions;
    }

    foreach (GLSL::Symbol *s, members) {
        QIcon icon;
        GLSL::Variable *var = s->asVariable();
        if (var) {
            int storageType = var->qualifiers() & GLSL::QualifiedTypeAST::StorageMask;
            if (storageType == GLSL::QualifiedTypeAST::Attribute)
                icon = m_attributeIcon;
            else if (storageType == GLSL::QualifiedTypeAST::Uniform)
                icon = m_uniformIcon;
            else if (storageType == GLSL::QualifiedTypeAST::Varying)
                icon = m_varyingIcon;
            else if (storageType == GLSL::QualifiedTypeAST::Const)
                icon = m_constIcon;
            else
                icon = m_varIcon;
        } else if (s->asArgument()) {
            icon = m_varIcon;
        } else if (s->asFunction() || s->asOverloadSet()) {
            icon = m_functionIcon;
        } else if (s->asStruct()) {
            icon = m_typeIcon;
        } else {
            icon = m_otherIcon;
        }
        if (specialMembers.contains(s->name()))
            addCompletion(s->name(), icon, SpecialMemberOrder);
        else
            addCompletion(s->name(), icon);
    }

    m_startPosition = pos + 1;
    return createContentProposal();
}

IAssistProposal *GLSLCompletionAssistProcessor::createContentProposal() const
{
    IGenericProposalModel *model = new BasicProposalItemListModel(m_completions);
    IAssistProposal *proposal = new GenericProposal(m_startPosition, model);
    return proposal;
}

IAssistProposal *GLSLCompletionAssistProcessor::createHintProposal(
    const QVector<GLSL::Function *> &symbols)
{
    IFunctionHintProposalModel *model = new GLSLFunctionHintProposalModel(symbols);
    IAssistProposal *proposal = new FunctionHintProposal(m_startPosition, model);
    return proposal;
}

bool GLSLCompletionAssistProcessor::acceptsIdleEditor() const
{
    const int cursorPosition = m_interface->position();
    const QChar ch = m_interface->characterAt(cursorPosition - 1);

    const QChar characterUnderCursor = m_interface->characterAt(cursorPosition);

    if (isIdentifierChar(ch) && (characterUnderCursor.isSpace() ||
                                 characterUnderCursor.isNull() ||
                                 isDelimiter(characterUnderCursor))) {
        int pos = m_interface->position() - 1;
        for (; pos != -1; --pos) {
            if (! isIdentifierChar(m_interface->characterAt(pos)))
                break;
        }
        ++pos;

        const QString word = m_interface->textAt(pos, cursorPosition - pos);
        if (word.length() > 2 && checkStartOfIdentifier(word)) {
            for (int i = 0; i < word.length(); ++i) {
                if (! isIdentifierChar(word.at(i)))
                    return false;
            }
            return true;
        }
    }

    return isActivationChar(ch);
}

void GLSLCompletionAssistProcessor::addCompletion(const QString &text,
                                                  const QIcon &icon,
                                                  int order)
{
    BasicProposalItem *item = new BasicProposalItem;
    item->setText(text);
    item->setIcon(icon);
    item->setOrder(order);
    m_completions.append(item);
}

// -----------------------------
// GLSLCompletionAssistInterface
// -----------------------------
GLSLCompletionAssistInterface::GLSLCompletionAssistInterface(QTextDocument *textDocument,
                                                             int position,
                                                             Core::IDocument *document,
                                                             TextEditor::AssistReason reason,
                                                             const QString &mimeType,
                                                             const Document::Ptr &glslDoc)
    : DefaultAssistInterface(textDocument, position, document, reason)
    , m_mimeType(mimeType)
    , m_glslDoc(glslDoc)
{
}
