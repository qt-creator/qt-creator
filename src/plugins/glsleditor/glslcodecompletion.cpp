/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#include "glslcodecompletion.h"
#include "glsleditor.h"
#include "glsleditorplugin.h"
#include <glsl/glslengine.h>
#include <glsl/glslengine.h>
#include <glsl/glsllexer.h>
#include <glsl/glslparser.h>
#include <glsl/glslsemantic.h>
#include <glsl/glslsymbols.h>
#include <glsl/glslastdump.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <texteditor/completionsettings.h>
#include <utils/faketooltip.h>
#include <QtGui/QIcon>
#include <QtGui/QPainter>
#include <QtGui/QLabel>
#include <QtGui/QToolButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtCore/QDebug>

using namespace GLSLEditor;
using namespace GLSLEditor::Internal;

enum CompletionOrder {
    SpecialMemberOrder = -5
};

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

namespace GLSLEditor {
namespace Internal {
class FunctionArgumentWidget : public QLabel
{
    Q_OBJECT

public:
    FunctionArgumentWidget();
    void showFunctionHint(QVector<GLSL::Function *> functionSymbols,
                          int startPosition);

protected:
    bool eventFilter(QObject *obj, QEvent *e);

private slots:
    void nextPage();
    void previousPage();

private:
    void updateArgumentHighlight();
    void updateHintText();
    void placeInsideScreen();

    GLSL::Function *currentFunction() const
    { return m_items.at(m_current); }

    int m_startpos;
    int m_currentarg;
    int m_current;
    bool m_escapePressed;

    TextEditor::ITextEditor *m_editor;

    QWidget *m_pager;
    QLabel *m_numberLabel;
    Utils::FakeToolTip *m_popupFrame;
    QVector<GLSL::Function *> m_items;
};


FunctionArgumentWidget::FunctionArgumentWidget():
    m_startpos(-1),
    m_current(0),
    m_escapePressed(false)
{
    QObject *editorObject = Core::EditorManager::instance()->currentEditor();
    m_editor = qobject_cast<TextEditor::ITextEditor *>(editorObject);

    m_popupFrame = new Utils::FakeToolTip(m_editor->widget());

    QToolButton *downArrow = new QToolButton;
    downArrow->setArrowType(Qt::DownArrow);
    downArrow->setFixedSize(16, 16);
    downArrow->setAutoRaise(true);

    QToolButton *upArrow = new QToolButton;
    upArrow->setArrowType(Qt::UpArrow);
    upArrow->setFixedSize(16, 16);
    upArrow->setAutoRaise(true);

    setParent(m_popupFrame);
    setFocusPolicy(Qt::NoFocus);

    m_pager = new QWidget;
    QHBoxLayout *hbox = new QHBoxLayout(m_pager);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    hbox->addWidget(upArrow);
    m_numberLabel = new QLabel;
    hbox->addWidget(m_numberLabel);
    hbox->addWidget(downArrow);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_pager);
    layout->addWidget(this);
    m_popupFrame->setLayout(layout);

    connect(upArrow, SIGNAL(clicked()), SLOT(previousPage()));
    connect(downArrow, SIGNAL(clicked()), SLOT(nextPage()));

    setTextFormat(Qt::RichText);

    qApp->installEventFilter(this);
}

void FunctionArgumentWidget::showFunctionHint(QVector<GLSL::Function *> functionSymbols,
                                              int startPosition)
{
    Q_ASSERT(!functionSymbols.isEmpty());

    if (m_startpos == startPosition)
        return;

    m_pager->setVisible(functionSymbols.size() > 1);

    m_items = functionSymbols;
    m_startpos = startPosition;
    m_current = 0;
    m_escapePressed = false;

    // update the text
    m_currentarg = -1;
    updateArgumentHighlight();

    m_popupFrame->show();
}

void FunctionArgumentWidget::nextPage()
{
    m_current = (m_current + 1) % m_items.size();
    updateHintText();
}

void FunctionArgumentWidget::previousPage()
{
    if (m_current == 0)
        m_current = m_items.size() - 1;
    else
        --m_current;

    updateHintText();
}

void FunctionArgumentWidget::updateArgumentHighlight()
{
    int curpos = m_editor->position();
    if (curpos < m_startpos) {
        m_popupFrame->close();
        return;
    }

    const QByteArray str = m_editor->textAt(m_startpos, curpos - m_startpos).toLatin1();

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

    if (m_currentarg != argnr) {
        m_currentarg = argnr;
        updateHintText();
    }

    if (parcount < 0)
        m_popupFrame->close();
}

bool FunctionArgumentWidget::eventFilter(QObject *obj, QEvent *e)
{
    switch (e->type()) {
    case QEvent::ShortcutOverride:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
            m_escapePressed = true;
        }
        break;
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
            m_escapePressed = true;
        }
        if (m_items.size() > 1) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            if (ke->key() == Qt::Key_Up) {
                previousPage();
                return true;
            } else if (ke->key() == Qt::Key_Down) {
                nextPage();
                return true;
            }
            return false;
        }
        break;
    case QEvent::KeyRelease:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape && m_escapePressed) {
            m_popupFrame->close();
            return false;
        }
        updateArgumentHighlight();
        break;
    case QEvent::WindowDeactivate:
    case QEvent::FocusOut:
        if (obj != m_editor->widget())
            break;
        m_popupFrame->close();
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel: {
            QWidget *widget = qobject_cast<QWidget *>(obj);
            if (! (widget == this || m_popupFrame->isAncestorOf(widget))) {
                m_popupFrame->close();
            }
        }
        break;
    default:
        break;
    }
    return false;
}

void FunctionArgumentWidget::updateHintText()
{
    setText(currentFunction()->prettyPrint(m_currentarg));

    m_numberLabel->setText(tr("%1 of %2").arg(m_current + 1).arg(m_items.size()));

    placeInsideScreen();
}

void FunctionArgumentWidget::placeInsideScreen()
{
    const QDesktopWidget *desktop = QApplication::desktop();
#ifdef Q_WS_MAC
    const QRect screen = desktop->availableGeometry(desktop->screenNumber(m_editor->widget()));
#else
    const QRect screen = desktop->screenGeometry(desktop->screenNumber(m_editor->widget()));
#endif

    m_pager->setFixedWidth(m_pager->minimumSizeHint().width());

    setWordWrap(false);
    const int maxDesiredWidth = screen.width() - 10;
    const QSize minHint = m_popupFrame->minimumSizeHint();
    if (minHint.width() > maxDesiredWidth) {
        setWordWrap(true);
        m_popupFrame->setFixedWidth(maxDesiredWidth);
        const int extra =
            m_popupFrame->contentsMargins().bottom() + m_popupFrame->contentsMargins().top();
        m_popupFrame->setFixedHeight(heightForWidth(maxDesiredWidth - m_pager->width()) + extra);
    } else {
        m_popupFrame->setFixedSize(minHint);
    }

    const QSize sz = m_popupFrame->size();
    QPoint pos = m_editor->cursorRect(m_startpos).topLeft();
    pos.setY(pos.y() - sz.height() - 1);

    if (pos.x() + sz.width() > screen.right())
        pos.setX(screen.right() - sz.width());

    m_popupFrame->move(pos);
}

} // Internal
} // GLSLEditor



CodeCompletion::CodeCompletion(QObject *parent)
    : ICompletionCollector(parent),
      m_editor(0),
      m_startPosition(-1),
      m_restartCompletion(false),
      m_keywordVariant(-1),
      m_keywordIcon(":/glsleditor/images/keyword.png"),
      m_varIcon(":/glsleditor/images/var.png"),
      m_functionIcon(":/glsleditor/images/func.png"),
      m_typeIcon(":/glsleditor/images/type.png"),
      m_constIcon(":/glsleditor/images/const.png"),
      m_attributeIcon(":/glsleditor/images/attribute.png"),
      m_uniformIcon(":/glsleditor/images/uniform.png"),
      m_varyingIcon(":/glsleditor/images/varying.png"),
      m_otherIcon(":/glsleditor/images/other.png")
{
}

CodeCompletion::~CodeCompletion()
{
}

TextEditor::ITextEditable *CodeCompletion::editor() const
{
    return m_editor;
}

int CodeCompletion::startPosition() const
{
    return m_startPosition;
}

bool CodeCompletion::supportsEditor(TextEditor::ITextEditable *editor)
{
    if (qobject_cast<GLSLTextEditor *>(editor->widget()) != 0)
        return true;

    return false;
}

bool CodeCompletion::triggersCompletion(TextEditor::ITextEditable *editor)
{
    const int cursorPosition = editor->position();
    const QChar ch = editor->characterAt(cursorPosition - 1);

    if (completionSettings().m_completionTrigger == TextEditor::AutomaticCompletion) {
        const QChar characterUnderCursor = editor->characterAt(cursorPosition);

        if (isIdentifierChar(ch) && (characterUnderCursor.isSpace() ||
                                     characterUnderCursor.isNull() ||
                                     isDelimiter(characterUnderCursor))) {
            int pos = editor->position() - 1;
            for (; pos != -1; --pos) {
                if (! isIdentifierChar(editor->characterAt(pos)))
                    break;
            }
            ++pos;

            const QString word = editor->textAt(pos, cursorPosition - pos);
            if (word.length() > 2 && checkStartOfIdentifier(word)) {
                for (int i = 0; i < word.length(); ++i) {
                    if (! isIdentifierChar(word.at(i)))
                        return false;
                }
                return true;
            }
        }
    }

    if (ch == QLatin1Char('(') || ch == QLatin1Char('.') || ch == QLatin1Char(','))
        return true;

    return false;
}

int CodeCompletion::startCompletion(TextEditor::ITextEditable *editor)
{
    m_editor = editor;

    int pos = editor->position() - 1;
    QChar ch = editor->characterAt(pos);
    while (ch.isLetterOrNumber() || ch == QLatin1Char('_'))
        ch = editor->characterAt(--pos);

    CPlusPlus::ExpressionUnderCursor expressionUnderCursor;
    GLSLTextEditor *edit = qobject_cast<GLSLTextEditor *>(editor->widget());

    QList<GLSL::Symbol *> members;
    QStringList specialMembers;

    bool functionCall = (ch == QLatin1Char('(') && pos == editor->position() - 1);

    if (ch == QLatin1Char(',')) {
        QTextCursor tc(edit->document());
        tc.setPosition(pos);
        const int start = expressionUnderCursor.startOfFunctionCall(tc);
        if (start == -1)
            return -1;

        if (edit->characterAt(start) == QLatin1Char('(')) {
            pos = start;
            ch = QLatin1Char('(');
            functionCall = true;
        }
    }

    if (ch == QLatin1Char('.') || functionCall) {
        const bool memberCompletion = ! functionCall;
        QTextCursor tc(edit->document());
        tc.setPosition(pos);

        // get the expression under cursor
        const QByteArray code = expressionUnderCursor(tc).toLatin1();
        //qDebug() << endl << "expression:" << code;

        // parse the expression
        GLSL::Engine engine;
        GLSL::Parser parser(&engine, code, code.size(), edit->languageVariant());
        GLSL::ExpressionAST *expr = parser.parseExpression();

#if 0
        // dump it!
        QTextStream qout(stdout, QIODevice::WriteOnly);
        GLSL::ASTDump dump(qout);
        dump(expr);
#endif

        if (Document::Ptr doc = edit->glslDocument()) {
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
                        // Recreate if necessary
                        if (!m_functionArgumentWidget)
                            m_functionArgumentWidget = new FunctionArgumentWidget;

                        m_functionArgumentWidget->showFunctionHint(signatures, pos + 1);
                        return false;
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
        if (Document::Ptr doc = edit->glslDocument()) {
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
                for (int index = 0; attributeNames[index]; ++index) {
                    TextEditor::CompletionItem item(this);
                    item.text = QString::fromLatin1(attributeNames[index]);
                    item.icon = m_attributeIcon;
                    m_completions.append(item);
                }
                for (int index = 0; uniformNames[index]; ++index) {
                    TextEditor::CompletionItem item(this);
                    item.text = QString::fromLatin1(uniformNames[index]);
                    item.icon = m_uniformIcon;
                    m_completions.append(item);
                }
            }
        }

        if (m_keywordVariant != edit->languageVariant()) {
            QStringList keywords = GLSL::Lexer::keywords(edit->languageVariant());
            m_keywordCompletions.clear();
            for (int index = 0; index < keywords.size(); ++index) {
                TextEditor::CompletionItem item(this);
                item.text = keywords.at(index);
                item.icon = m_keywordIcon;
                m_keywordCompletions.append(item);
            }
            m_keywordVariant = edit->languageVariant();
        }

        m_completions += m_keywordCompletions;
    }

    foreach (GLSL::Symbol *s, members) {
        TextEditor::CompletionItem item(this);
        GLSL::Variable *var = s->asVariable();
        if (var) {
            int storageType = var->qualifiers() & GLSL::QualifiedTypeAST::StorageMask;
            if (storageType == GLSL::QualifiedTypeAST::Attribute)
                item.icon = m_attributeIcon;
            else if (storageType == GLSL::QualifiedTypeAST::Uniform)
                item.icon = m_uniformIcon;
            else if (storageType == GLSL::QualifiedTypeAST::Varying)
                item.icon = m_varyingIcon;
            else if (storageType == GLSL::QualifiedTypeAST::Const)
                item.icon = m_constIcon;
            else
                item.icon = m_varIcon;
        } else if (s->asArgument()) {
            item.icon = m_varIcon;
        } else if (s->asFunction() || s->asOverloadSet()) {
            item.icon = m_functionIcon;
        } else if (s->asStruct()) {
            item.icon = m_typeIcon;
        } else {
            item.icon = m_otherIcon;
        }
        item.text = s->name();
        if (specialMembers.contains(item.text))
            item.order = SpecialMemberOrder;
        m_completions.append(item);
    }

    m_startPosition = pos + 1;
    return m_startPosition;
}

void CodeCompletion::completions(QList<TextEditor::CompletionItem> *completions)
{
    const int length = m_editor->position() - m_startPosition;

    if (length == 0)
        *completions = m_completions;
    else if (length > 0) {
        const QString key = m_editor->textAt(m_startPosition, length);

        filter(m_completions, completions, key);

        if (completions->size() == 1) {
            if (key == completions->first().text)
                completions->clear();
        }
    }
}

bool CodeCompletion::typedCharCompletes(const TextEditor::CompletionItem &item, QChar typedChar)
{
    Q_UNUSED(item);
    Q_UNUSED(typedChar);
    return false;
}

void CodeCompletion::complete(const TextEditor::CompletionItem &item, QChar typedChar)
{
    Q_UNUSED(typedChar);

    QString toInsert = item.text;

    const int length = m_editor->position() - m_startPosition;
    m_editor->setCurPos(m_startPosition);
    m_editor->replace(length, toInsert);

    if (toInsert.endsWith(QLatin1Char('.')) || toInsert.endsWith(QLatin1Char('(')))
        m_restartCompletion = true;
}

bool CodeCompletion::partiallyComplete(const QList<TextEditor::CompletionItem> &completionItems)
{
    return ICompletionCollector::partiallyComplete(completionItems);
}

bool CodeCompletion::glslCompletionItemLessThan(const TextEditor::CompletionItem &l, const TextEditor::CompletionItem &r)
{
    if (l.order != r.order)
        return l.order < r.order;
    return completionItemLessThan(l, r);
}

QList<TextEditor::CompletionItem> CodeCompletion::getCompletions()
{
    QList<TextEditor::CompletionItem> completionItems;

    completions(&completionItems);

    qStableSort(completionItems.begin(), completionItems.end(), glslCompletionItemLessThan);

    // Remove duplicates
    QString lastKey;
    QVariant lastData;
    QList<TextEditor::CompletionItem> uniquelist;

    foreach (const TextEditor::CompletionItem &item, completionItems) {
        if (item.text != lastKey || item.data.type() != lastData.type()) {
            uniquelist.append(item);
            lastKey = item.text;
            lastData = item.data;
        }
    }

    return uniquelist;
}

bool CodeCompletion::shouldRestartCompletion()
{
    return m_restartCompletion;
}

void CodeCompletion::cleanup()
{
    m_editor = 0;
    m_completions.clear();
    m_restartCompletion = false;
    m_startPosition = -1;
}

#include "glslcodecompletion.moc"
