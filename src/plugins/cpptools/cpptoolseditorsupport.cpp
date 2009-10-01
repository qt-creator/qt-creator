/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "cpptoolseditorsupport.h"
#include "cppmodelmanager.h"

#include <coreplugin/ifile.h>

#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>

#include <AST.h>
#include <ASTVisitor.h>
#include <TranslationUnit.h>

#include <QtCore/QTimer>

using namespace CppTools::Internal;
using namespace CPlusPlus;

namespace {

enum {
    DEFAULT_QUICKFIX_INTERVAL = 500
};

class QuickFixMark: public TextEditor::ITextMark
{
    QIcon _icon;

public:
    QuickFixMark(QObject *parent)
        : TextEditor::ITextMark(parent),
          _icon(QLatin1String(":/core/images/redo.png")) // ### FIXME
    { }

    virtual ~QuickFixMark()
    { }

    virtual QIcon icon() const
    { return _icon; }

    virtual void updateLineNumber(int)
    { }

    virtual void updateBlock(const QTextBlock &)
    { }

    virtual void removedFromEditor()
    { }

    virtual void documentClosing()
    { }
};

class ReplaceCast: public QuickFixOperation
{
    CastExpressionAST *_castExpression;

public:
    ReplaceCast(CastExpressionAST *node, Document::Ptr doc, const Snapshot &snapshot)
        : QuickFixOperation(doc, snapshot),
          _castExpression(node)
    { }

    virtual QString description() const
    { return QLatin1String("Rewrite old C-style cast"); }

    virtual void apply(QTextCursor tc)
    {
        setTextCursor(tc);

        tc.beginEditBlock();

        QTextCursor beginOfCast = cursor(_castExpression->lparen_token);
        QTextCursor endOfCast = cursor(_castExpression->rparen_token);
        QTextCursor beginOfExpr = moveAtStartOfToken(_castExpression->expression->firstToken());
        QTextCursor endOfExpr = moveAtEndOfToken(_castExpression->expression->lastToken() - 1);

        beginOfCast.insertText(QLatin1String("reinterpret_cast<"));
        endOfCast.insertText(QLatin1String(">"));

        beginOfExpr.insertText(QLatin1String("("));
        endOfExpr.insertText(QLatin1String(")"));

        tc.endEditBlock();
    }
};

class RewriteConditional: public QuickFixOperation
{
    QString _source;
    BinaryExpressionAST *_binaryExpression;

public:
    RewriteConditional(const QString &source, BinaryExpressionAST *node,
                       Document::Ptr doc, const Snapshot &snapshot)
        : QuickFixOperation(doc, snapshot),
          _source(source),
          _binaryExpression(node)
    { }

    virtual QString description() const
    { return QString::fromUtf8("Rewrite conditional (%1)").arg(_source.simplified()); }

    virtual void apply(QTextCursor tc)
    {
        setTextCursor(tc);

        tc.beginEditBlock();

        UnaryExpressionAST *left_unary_expr = _binaryExpression->left_expression->asUnaryExpression();
        UnaryExpressionAST *right_unary_expr = _binaryExpression->right_expression->asUnaryExpression();

        QTextCursor left_not_op = cursor(left_unary_expr->unary_op_token);
        QTextCursor right_not_op = cursor(right_unary_expr->unary_op_token);
        QTextCursor log_and_op = cursor(_binaryExpression->binary_op_token);

        QTextCursor begin_of_expr = moveAtStartOfToken(_binaryExpression->firstToken());
        QTextCursor end_of_expr = moveAtEndOfToken(_binaryExpression->lastToken() - 1);

        left_not_op.removeSelectedText();
        right_not_op.removeSelectedText();
        log_and_op.insertText(QLatin1String("||"));
        begin_of_expr.insertText(QLatin1String("!("));
        end_of_expr.insertText(QLatin1String(")"));

        tc.endEditBlock();
    }
};
class CheckDocument: protected ASTVisitor
{
    QTextCursor _textCursor;
    Document::Ptr _doc;
    Snapshot _snapshot;
    unsigned _line;
    unsigned _column;
    QList<QuickFixOperationPtr> _quickFixes;

public:
    CheckDocument(Document::Ptr doc, Snapshot snapshot)
        : ASTVisitor(doc->control()), _doc(doc), _snapshot(snapshot)
    { }

    QList<QuickFixOperationPtr> operator()(QTextCursor tc)
    {
        _quickFixes.clear();
        _textCursor = tc;
        _line = tc.blockNumber() + 1;
        _column = tc.columnNumber() + 1;
        accept(_doc->translationUnit()->ast());
        return _quickFixes;
    }

protected:
    using ASTVisitor::visit;

    bool checkPosition(AST *ast) const
    {
        unsigned startLine, startColumn;
        unsigned endLine, endColumn;

        getTokenStartPosition(ast->firstToken(), &startLine, &startColumn);
        getTokenEndPosition(ast->lastToken() - 1, &endLine, &endColumn);

        if (_line < startLine || (_line == startLine && _column < startColumn))
            return false;
        else if (_line > endLine || (_line == endLine && _column >= endColumn))
            return false;

        return true;
    }

    QTextCursor moveAtStartOfToken(unsigned index) const
    {
        unsigned line, col;
        getTokenStartPosition(index, &line, &col);
        QTextCursor tc = _textCursor;
        tc.setPosition(tc.document()->findBlockByNumber(line - 1).position() + col - 1);
        return tc;
    }

    QTextCursor moveAtEndOfToken(unsigned index) const
    {
        const Token &tk = tokenAt(index);

        unsigned line, col;
        getTokenStartPosition(index, &line, &col);
        QTextCursor tc = _textCursor;
        tc.setPosition(tc.document()->findBlockByNumber(line - 1).position() + col + tk.f.length - 1);
        return tc;
    }

    virtual bool visit(BinaryExpressionAST *ast)
    {
        if (ast->left_expression && ast->right_expression && tokenKind(ast->binary_op_token) == T_AMPER_AMPER &&
                checkPosition(ast)) {
            UnaryExpressionAST *left_unary_expr = ast->left_expression->asUnaryExpression();
            UnaryExpressionAST *right_unary_expr = ast->right_expression->asUnaryExpression();
            if (left_unary_expr && left_unary_expr->expression && tokenKind(left_unary_expr->unary_op_token) == T_NOT &&
                right_unary_expr && right_unary_expr->expression && tokenKind(right_unary_expr->unary_op_token) == T_NOT) {
                // replace !a && !b with !(a || b)
                QTextCursor beg = moveAtStartOfToken(ast->firstToken());
                QTextCursor end = moveAtEndOfToken(ast->lastToken() - 1);
                beg.setPosition(end.position(), QTextCursor::KeepAnchor);
                QString source = beg.selectedText();

                QuickFixOperationPtr op(new RewriteConditional(source, ast, _doc, _snapshot));
                _quickFixes.append(op);
                return true;
            }
        }

        return true;
    }

    virtual bool visit(CastExpressionAST *ast)
    {
        if (! checkPosition(ast))
            return true;

        if (ast->type_id && ast->lparen_token && ast->rparen_token && ast->expression) {
            QuickFixOperationPtr op(new ReplaceCast(ast, _doc, _snapshot));
            _quickFixes.append(op);
        }

        return true;
    }
};

} // end of anonymous namespace

QuickFixOperation::QuickFixOperation(CPlusPlus::Document::Ptr doc, const CPlusPlus::Snapshot &snapshot)
    : _doc(doc), _snapshot(snapshot)
{ }

QuickFixOperation::~QuickFixOperation()
{ }

QTextCursor QuickFixOperation::textCursor() const
{ return _textCursor; }

void QuickFixOperation::setTextCursor(const QTextCursor &tc)
{ _textCursor = tc; }

const CPlusPlus::Token &QuickFixOperation::tokenAt(unsigned index) const
{ return _doc->translationUnit()->tokenAt(index); }

void QuickFixOperation::getTokenStartPosition(unsigned index, unsigned *line, unsigned *column) const
{ _doc->translationUnit()->getPosition(tokenAt(index).begin(), line, column); }

void QuickFixOperation::getTokenEndPosition(unsigned index, unsigned *line, unsigned *column) const
{ _doc->translationUnit()->getPosition(tokenAt(index).end(), line, column); }

QTextCursor QuickFixOperation::cursor(unsigned index) const
{
    const Token &tk = tokenAt(index);

    unsigned line, col;
    getTokenStartPosition(index, &line, &col);
    QTextCursor tc = _textCursor;
    tc.setPosition(tc.document()->findBlockByNumber(line - 1).position() + col - 1);
    tc.setPosition(tc.position() + tk.f.length, QTextCursor::KeepAnchor);
    return tc;
}

QTextCursor QuickFixOperation::moveAtStartOfToken(unsigned index) const
{
    unsigned line, col;
    getTokenStartPosition(index, &line, &col);
    QTextCursor tc = _textCursor;
    tc.setPosition(tc.document()->findBlockByNumber(line - 1).position() + col - 1);
    return tc;
}

QTextCursor QuickFixOperation::moveAtEndOfToken(unsigned index) const
{
    const Token &tk = tokenAt(index);

    unsigned line, col;
    getTokenStartPosition(index, &line, &col);
    QTextCursor tc = _textCursor;
    tc.setPosition(tc.document()->findBlockByNumber(line - 1).position() + col + tk.f.length - 1);
    return tc;
}

CppEditorSupport::CppEditorSupport(CppModelManager *modelManager)
    : QObject(modelManager),
      _modelManager(modelManager),
      _updateDocumentInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL)
{
    _updateDocumentTimer = new QTimer(this);
    _updateDocumentTimer->setSingleShot(true);
    _updateDocumentTimer->setInterval(_updateDocumentInterval);
    connect(_updateDocumentTimer, SIGNAL(timeout()), this, SLOT(updateDocumentNow()));
    _quickFixMark = new QuickFixMark(this);

    _quickFixTimer = new QTimer(this);
    _quickFixTimer->setSingleShot(true);
    _quickFixTimer->setInterval(DEFAULT_QUICKFIX_INTERVAL);
#ifdef QTCREATOR_WITH_QUICKFIX
    connect(_quickFixTimer, SIGNAL(timeout()), this, SLOT(checkDocumentNow()));
#endif
}

CppEditorSupport::~CppEditorSupport()
{ }

TextEditor::ITextEditor *CppEditorSupport::textEditor() const
{ return _textEditor; }

void CppEditorSupport::setTextEditor(TextEditor::ITextEditor *textEditor)
{
    _textEditor = textEditor;

    if (! _textEditor)
        return;

    connect(_textEditor, SIGNAL(contentsChanged()), this, SIGNAL(contentsChanged()));

#ifdef QTCREATOR_WITH_QUICKFIX
    connect(qobject_cast<TextEditor::BaseTextEditor *>(_textEditor->widget()), SIGNAL(cursorPositionChanged()),
            this, SLOT(checkDocument()));
#endif

    connect(this, SIGNAL(contentsChanged()), this, SLOT(updateDocument()));

    updateDocument();
}

QString CppEditorSupport::contents()
{
    if (! _textEditor)
        return QString();
    else if (! _cachedContents.isEmpty())
        _cachedContents = _textEditor->contents();

    return _cachedContents;
}

int CppEditorSupport::updateDocumentInterval() const
{ return _updateDocumentInterval; }

void CppEditorSupport::setUpdateDocumentInterval(int updateDocumentInterval)
{ _updateDocumentInterval = updateDocumentInterval; }

void CppEditorSupport::updateDocument()
{
    if (TextEditor::BaseTextEditor *edit = qobject_cast<TextEditor::BaseTextEditor*>(_textEditor->widget())) {
        const QList<QTextEdit::ExtraSelection> selections =
                edit->extraSelections(TextEditor::BaseTextEditor::CodeWarningsSelection);

        _modelManager->stopEditorSelectionsUpdate();
    }

    _updateDocumentTimer->start(_updateDocumentInterval);
}

void CppEditorSupport::updateDocumentNow()
{
    if (_documentParser.isRunning()) {
        _updateDocumentTimer->start(_updateDocumentInterval);
    } else {
        _updateDocumentTimer->stop();

        QStringList sourceFiles(_textEditor->file()->fileName());
        _cachedContents = _textEditor->contents().toUtf8();
        _documentParser = _modelManager->refreshSourceFiles(sourceFiles);
    }
}

void CppEditorSupport::checkDocument()
{
    _quickFixTimer->start(DEFAULT_QUICKFIX_INTERVAL);
}

void CppEditorSupport::checkDocumentNow()
{
    _textEditor->markableInterface()->removeMark(_quickFixMark);
    _quickFixes.clear();

    TextEditor::BaseTextEditor *ed =
            qobject_cast<TextEditor::BaseTextEditor *>(_textEditor->widget());

    Snapshot snapshot = _modelManager->snapshot();
    const QString plainText = contents();
    const QString fileName = _textEditor->file()->fileName();
    const QByteArray preprocessedCode = snapshot.preprocessedCode(plainText, fileName);

    if (Document::Ptr doc = snapshot.documentFromSource(preprocessedCode, fileName)) {
        doc->parse();

        CheckDocument checkDocument(doc, snapshot);
        QList<QuickFixOperationPtr> quickFixes = checkDocument(ed->textCursor());
        if (! quickFixes.isEmpty()) {
            int line, col;
            ed->convertPosition(ed->position(), &line, &col);

            _textEditor->markableInterface()->addMark(_quickFixMark, line);
            _quickFixes = quickFixes;
        }
    }
}


