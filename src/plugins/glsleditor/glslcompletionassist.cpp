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

#include "glslcompletionassist.h"
#include "glsleditorconstants.h"
#include "glsleditorplugin.h"

#include <glsl/glslengine.h>
#include <glsl/glsllexer.h>
#include <glsl/glslparser.h>
#include <glsl/glslsemantic.h>
#include <glsl/glslsymbols.h>
#include <glsl/glslastdump.h>

#include <coreplugin/idocument.h>
#include <texteditor/completionsettings.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/functionhintproposal.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/Icons.h>

#include <utils/icon.h>
#include <utils/faketooltip.h>

#include <QIcon>
#include <QPainter>
#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>
#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>

using namespace TextEditor;

namespace GlslEditor {
namespace Internal {

Document::Document()
    : _engine(0)
    , _ast(0)
    , _globalScope(0)
{
}

Document::~Document()
{
    delete _globalScope;
    delete _engine;
}

GLSL::Scope *Document::scopeAt(int position) const
{
    foreach (const Range &c, _cursors) {
        if (position >= c.cursor.selectionStart() && position <= c.cursor.selectionEnd())
            return c.scope;
    }
    return _globalScope;
}

void Document::addRange(const QTextCursor &cursor, GLSL::Scope *scope)
{
    Range c;
    c.cursor = cursor;
    c.scope = scope;
    _cursors.append(c);
}


enum CompletionOrder {
    SpecialMemberOrder = -5
};

static bool isActivationChar(const QChar &ch)
{
    return ch == QLatin1Char('(') || ch == QLatin1Char('.') || ch == QLatin1Char(',');
}

static bool isIdentifierChar(QChar ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_');
}

static bool isDelimiter(QChar ch)
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

static bool checkStartOfIdentifier(const QString &word)
{
    if (! word.isEmpty()) {
        const QChar ch = word.at(0);
        if (ch.isLetter() || ch == QLatin1Char('_'))
            return true;
    }

    return false;
}

enum IconTypes {
    IconTypeAttribute,
    IconTypeUniform,
    IconTypeKeyword,
    IconTypeVarying,
    IconTypeConst,
    IconTypeVariable,
    IconTypeType,
    IconTypeFunction,
    IconTypeOther
};

static QIcon glslIcon(IconTypes iconType)
{
    using namespace CPlusPlus;
    using namespace Utils;

    const QString member = QLatin1String(":/codemodel/images/member.png");

    switch (iconType) {
    case IconTypeType:
        return Icons::iconForType(Icons::ClassIconType);
    case IconTypeConst:
        return Icons::iconForType(Icons::EnumeratorIconType);
    case IconTypeKeyword:
        return Icons::iconForType(Icons::KeywordIconType);
    case IconTypeFunction:
        return Icons::iconForType(Icons::FuncPublicIconType);
    case IconTypeVariable:
        return Icons::iconForType(Icons::VarPublicIconType);
    case IconTypeAttribute: {
        static const QIcon icon =
                Icon({{member, Theme::IconsCodeModelAttributeColor}}, Icon::Tint).icon();
        return icon;
    }
    case IconTypeUniform: {
        static const QIcon icon =
                Icon({{member, Theme::IconsCodeModelUniformColor}}, Icon::Tint).icon();
        return icon;
    }
    case IconTypeVarying: {
        static const QIcon icon =
                Icon({{member, Theme::IconsCodeModelVaryingColor}}, Icon::Tint).icon();
        return icon;
    }
    case IconTypeOther:
    default:
        return Icons::iconForType(Icons::NamespaceIconType);
    }
}

// ----------------------------
// GlslCompletionAssistProvider
// ----------------------------
bool GlslCompletionAssistProvider::supportsEditor(Core::Id editorId) const
{
    return editorId == Constants::C_GLSLEDITOR_ID;
}

IAssistProcessor *GlslCompletionAssistProvider::createProcessor() const
{
    return new GlslCompletionAssistProcessor;
}

int GlslCompletionAssistProvider::activationCharSequenceLength() const
{
    return 1;
}

bool GlslCompletionAssistProvider::isActivationCharSequence(const QString &sequence) const
{
    return isActivationChar(sequence.at(0));
}

// -----------------------------
// GlslFunctionHintProposalModel
// -----------------------------
class GlslFunctionHintProposalModel : public IFunctionHintProposalModel
{
public:
    GlslFunctionHintProposalModel(QVector<GLSL::Function *> functionSymbols)
        : m_items(functionSymbols)
        , m_currentArg(-1)
    {}

    void reset() override {}
    int size() const override { return m_items.size(); }
    QString text(int index) const override;
    int activeArgument(const QString &prefix) const override;

private:
    QVector<GLSL::Function *> m_items;
    mutable int m_currentArg;
};

QString GlslFunctionHintProposalModel::text(int index) const
{
    return m_items.at(index)->prettyPrint(m_currentArg);
}

int GlslFunctionHintProposalModel::activeArgument(const QString &prefix) const
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
GlslCompletionAssistProcessor::GlslCompletionAssistProcessor()
    : m_startPosition(0)
{}

GlslCompletionAssistProcessor::~GlslCompletionAssistProcessor()
{}

static AssistProposalItem *createCompletionItem(const QString &text, const QIcon &icon, int order = 0)
{
    AssistProposalItem *item = new AssistProposalItem;
    item->setText(text);
    item->setIcon(icon);
    item->setOrder(order);
    return item;
}

IAssistProposal *GlslCompletionAssistProcessor::perform(const AssistInterface *interface)
{
    m_interface.reset(static_cast<const GlslCompletionAssistInterface *>(interface));

    if (interface->reason() == IdleEditor && !acceptsIdleEditor())
        return 0;

    int pos = m_interface->position() - 1;
    QChar ch = m_interface->characterAt(pos);
    while (ch.isLetterOrNumber() || ch == QLatin1Char('_'))
        ch = m_interface->characterAt(--pos);

    CPlusPlus::ExpressionUnderCursor expressionUnderCursor(
                CPlusPlus::LanguageFeatures::defaultFeatures());
    //GLSLTextEditorWidget *edit = qobject_cast<GLSLTextEditorWidget *>(editor->widget());

    QList<GLSL::Symbol *> members;
    QStringList specialMembers;
    QList<AssistProposalItemInterface *> m_completions;

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
                    m_completions << createCompletionItem(QString::fromLatin1(attributeNames[index]), glslIcon(IconTypeAttribute));
                for (int index = 0; uniformNames[index]; ++index)
                    m_completions << createCompletionItem(QString::fromLatin1(uniformNames[index]), glslIcon(IconTypeUniform));
            }
        }

 //       if (m_keywordVariant != languageVariant(m_interface->mimeType())) {
            QStringList keywords = GLSL::Lexer::keywords(languageVariant(m_interface->mimeType()));
//            m_keywordCompletions.clear();
            for (int index = 0; index < keywords.size(); ++index)
                m_completions << createCompletionItem(keywords.at(index), glslIcon(IconTypeKeyword));
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
                icon = glslIcon(IconTypeAttribute);
            else if (storageType == GLSL::QualifiedTypeAST::Uniform)
                icon = glslIcon(IconTypeUniform);
            else if (storageType == GLSL::QualifiedTypeAST::Varying)
                icon = glslIcon(IconTypeVarying);
            else if (storageType == GLSL::QualifiedTypeAST::Const)
                icon = glslIcon(IconTypeConst);
            else
                icon = glslIcon(IconTypeVariable);
        } else if (s->asArgument()) {
            icon = glslIcon(IconTypeVariable);
        } else if (s->asFunction() || s->asOverloadSet()) {
            icon = glslIcon(IconTypeFunction);
        } else if (s->asStruct()) {
            icon = glslIcon(IconTypeType);
        } else {
            icon = glslIcon(IconTypeOther);
        }
        if (specialMembers.contains(s->name()))
            m_completions << createCompletionItem(s->name(), icon, SpecialMemberOrder);
        else
            m_completions << createCompletionItem(s->name(), icon);
    }

    m_startPosition = pos + 1;

    return new GenericProposal(m_startPosition, m_completions);
}

IAssistProposal *GlslCompletionAssistProcessor::createHintProposal(
    const QVector<GLSL::Function *> &symbols)
{
    IFunctionHintProposalModel *model = new GlslFunctionHintProposalModel(symbols);
    IAssistProposal *proposal = new FunctionHintProposal(m_startPosition, model);
    return proposal;
}

bool GlslCompletionAssistProcessor::acceptsIdleEditor() const
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

// -----------------------------
// GlslCompletionAssistInterface
// -----------------------------
GlslCompletionAssistInterface::GlslCompletionAssistInterface(QTextDocument *textDocument,
                                                             int position,
                                                             const QString &fileName,
                                                             AssistReason reason,
                                                             const QString &mimeType,
                                                             const Document::Ptr &glslDoc)
    : AssistInterface(textDocument, position, fileName, reason)
    , m_mimeType(mimeType)
    , m_glslDoc(glslDoc)
{
}

} // namespace Internal
} // namespace GlslEditor
