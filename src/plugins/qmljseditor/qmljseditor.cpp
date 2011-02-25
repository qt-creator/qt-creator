/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmljseditor.h"
#include "qmljseditoreditable.h"
#include "qmljseditorconstants.h"
#include "qmljshighlighter.h"
#include "qmljseditorplugin.h"
#include "qmljsquickfix.h"
#include "qmloutlinemodel.h"
#include "qmljsfindreferences.h"
#include "qmljssemantichighlighter.h"
#include "qmljsindenter.h"
#include "qmljsautocompleter.h"

#include <qmljs/qmljsbind.h>
#include <qmljs/qmljsevaluate.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsicontextpane.h>
#include <qmljs/qmljslookupcontext.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>

#include <qmljstools/qmljsqtstylecodeformatter.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/fontsettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/refactoroverlay.h>
#include <texteditor/tooltip/tooltip.h>
#include <qmldesigner/qmldesignerconstants.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/changeset.h>
#include <utils/uncommentselection.h>

#include <QtCore/QFileInfo>
#include <QtCore/QSignalMapper>
#include <QtCore/QTimer>

#include <QtGui/QMenu>
#include <QtGui/QComboBox>
#include <QtGui/QHeaderView>
#include <QtGui/QInputDialog>
#include <QtGui/QMainWindow>
#include <QtGui/QToolBar>
#include <QtGui/QTreeView>

enum {
    UPDATE_DOCUMENT_DEFAULT_INTERVAL = 100,
    UPDATE_USES_DEFAULT_INTERVAL = 150,
    UPDATE_OUTLINE_INTERVAL = 500 // msecs after new semantic info has been arrived / cursor has moved
};

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;

namespace {

class FindIdDeclarations: protected Visitor
{
public:
    typedef QHash<QString, QList<AST::SourceLocation> > Result;

    Result operator()(Document::Ptr doc)
    {
        _ids.clear();
        _maybeIds.clear();
        if (doc && doc->qmlProgram())
            doc->qmlProgram()->accept(this);
        return _ids;
    }

protected:
    QString asString(AST::UiQualifiedId *id)
    {
        QString text;
        for (; id; id = id->next) {
            if (id->name)
                text += id->name->asString();
            else
                text += QLatin1Char('?');

            if (id->next)
                text += QLatin1Char('.');
        }

        return text;
    }

    void accept(AST::Node *node)
    { AST::Node::acceptChild(node, this); }

    using Visitor::visit;
    using Visitor::endVisit;

    virtual bool visit(AST::UiScriptBinding *node)
    {
        if (asString(node->qualifiedId) == QLatin1String("id")) {
            if (AST::ExpressionStatement *stmt = AST::cast<AST::ExpressionStatement*>(node->statement)) {
                if (AST::IdentifierExpression *idExpr = AST::cast<AST::IdentifierExpression *>(stmt->expression)) {
                    if (idExpr->name) {
                        const QString id = idExpr->name->asString();
                        QList<AST::SourceLocation> *locs = &_ids[id];
                        locs->append(idExpr->firstSourceLocation());
                        locs->append(_maybeIds.value(id));
                        _maybeIds.remove(id);
                        return false;
                    }
                }
            }
        }

        accept(node->statement);

        return false;
    }

    virtual bool visit(AST::IdentifierExpression *node)
    {
        if (node->name) {
            const QString name = node->name->asString();

            if (_ids.contains(name))
                _ids[name].append(node->identifierToken);
            else
                _maybeIds[name].append(node->identifierToken);
        }
        return false;
    }

private:
    Result _ids;
    Result _maybeIds;
};

class FindDeclarations: protected Visitor
{
    QList<Declaration> _declarations;
    int _depth;

public:
    QList<Declaration> operator()(AST::Node *node)
    {
        _depth = -1;
        _declarations.clear();
        accept(node);
        return _declarations;
    }

protected:
    using Visitor::visit;
    using Visitor::endVisit;

    QString asString(AST::UiQualifiedId *id)
    {
        QString text;
        for (; id; id = id->next) {
            if (id->name)
                text += id->name->asString();
            else
                text += QLatin1Char('?');

            if (id->next)
                text += QLatin1Char('.');
        }

        return text;
    }

    void accept(AST::Node *node)
    { AST::Node::acceptChild(node, this); }

    void init(Declaration *decl, AST::UiObjectMember *member)
    {
        const SourceLocation first = member->firstSourceLocation();
        const SourceLocation last = member->lastSourceLocation();
        decl->startLine = first.startLine;
        decl->startColumn = first.startColumn;
        decl->endLine = last.startLine;
        decl->endColumn = last.startColumn + last.length;
    }

    void init(Declaration *decl, AST::ExpressionNode *expressionNode)
    {
        const SourceLocation first = expressionNode->firstSourceLocation();
        const SourceLocation last = expressionNode->lastSourceLocation();
        decl->startLine = first.startLine;
        decl->startColumn = first.startColumn;
        decl->endLine = last.startLine;
        decl->endColumn = last.startColumn + last.length;
    }

    virtual bool visit(AST::UiObjectDefinition *node)
    {
        ++_depth;

        Declaration decl;
        init(&decl, node);

        decl.text.fill(QLatin1Char(' '), _depth);
        if (node->qualifiedTypeNameId)
            decl.text.append(asString(node->qualifiedTypeNameId));
        else
            decl.text.append(QLatin1Char('?'));

        _declarations.append(decl);

        return true; // search for more bindings
    }

    virtual void endVisit(AST::UiObjectDefinition *)
    {
        --_depth;
    }

    virtual bool visit(AST::UiObjectBinding *node)
    {
        ++_depth;

        Declaration decl;
        init(&decl, node);

        decl.text.fill(QLatin1Char(' '), _depth);

        decl.text.append(asString(node->qualifiedId));
        decl.text.append(QLatin1String(": "));

        if (node->qualifiedTypeNameId)
            decl.text.append(asString(node->qualifiedTypeNameId));
        else
            decl.text.append(QLatin1Char('?'));

        _declarations.append(decl);

        return true; // search for more bindings
    }

    virtual void endVisit(AST::UiObjectBinding *)
    {
        --_depth;
    }

    virtual bool visit(AST::UiScriptBinding *)
    {
        ++_depth;

#if 0 // ### ignore script bindings for now.
        Declaration decl;
        init(&decl, node);

        decl.text.fill(QLatin1Char(' '), _depth);
        decl.text.append(asString(node->qualifiedId));

        _declarations.append(decl);
#endif

        return false; // more more bindings in this subtree.
    }

    virtual void endVisit(AST::UiScriptBinding *)
    {
        --_depth;
    }

    virtual bool visit(AST::FunctionExpression *)
    {
        return false;
    }

    virtual bool visit(AST::FunctionDeclaration *ast)
    {
        if (! ast->name)
            return false;

        Declaration decl;
        init(&decl, ast);

        decl.text.fill(QLatin1Char(' '), _depth);
        decl.text += ast->name->asString();

        decl.text += QLatin1Char('(');
        for (FormalParameterList *it = ast->formals; it; it = it->next) {
            if (it->name)
                decl.text += it->name->asString();

            if (it->next)
                decl.text += QLatin1String(", ");
        }

        decl.text += QLatin1Char(')');

        _declarations.append(decl);

        return false;
    }

    virtual bool visit(AST::VariableDeclaration *ast)
    {
        if (! ast->name)
            return false;

        Declaration decl;
        decl.text.fill(QLatin1Char(' '), _depth);
        decl.text += ast->name->asString();

        const SourceLocation first = ast->identifierToken;
        decl.startLine = first.startLine;
        decl.startColumn = first.startColumn;
        decl.endLine = first.startLine;
        decl.endColumn = first.startColumn + first.length;

        _declarations.append(decl);

        return false;
    }
};

class CreateRanges: protected AST::Visitor
{
    QTextDocument *_textDocument;
    QList<Range> _ranges;

public:
    QList<Range> operator()(QTextDocument *textDocument, Document::Ptr doc)
    {
        _textDocument = textDocument;
        _ranges.clear();
        if (doc && doc->ast() != 0)
            doc->ast()->accept(this);
        return _ranges;
    }

protected:
    using AST::Visitor::visit;

    virtual bool visit(AST::UiObjectBinding *ast)
    {
        if (ast->initializer && ast->initializer->lbraceToken.length)
            _ranges.append(createRange(ast, ast->initializer));
        return true;
    }

    virtual bool visit(AST::UiObjectDefinition *ast)
    {
        if (ast->initializer && ast->initializer->lbraceToken.length)
            _ranges.append(createRange(ast, ast->initializer));
        return true;
    }

    virtual bool visit(AST::FunctionExpression *ast)
    {
        _ranges.append(createRange(ast));
        return true;
    }

    virtual bool visit(AST::FunctionDeclaration *ast)
    {
        _ranges.append(createRange(ast));
        return true;
    }

    Range createRange(AST::UiObjectMember *member, AST::UiObjectInitializer *ast)
    {
        Range range;

        range.ast = member;

        range.begin = QTextCursor(_textDocument);
        range.begin.setPosition(member->firstSourceLocation().begin());

        range.end = QTextCursor(_textDocument);
        range.end.setPosition(ast->rbraceToken.end());
        return range;
    }

    Range createRange(AST::FunctionExpression *ast)
    {
        Range range;

        range.ast = ast;

        range.begin = QTextCursor(_textDocument);
        range.begin.setPosition(ast->lbraceToken.begin());

        range.end = QTextCursor(_textDocument);
        range.end.setPosition(ast->rbraceToken.end());

        return range;
    }

};


class CollectASTNodes: protected AST::Visitor
{
public:
    QList<AST::UiQualifiedId *> qualifiedIds;
    QList<AST::IdentifierExpression *> identifiers;
    QList<AST::FieldMemberExpression *> fieldMembers;

    void accept(AST::Node *node)
    {
        if (node)
            node->accept(this);
    }

protected:
    using AST::Visitor::visit;

    virtual bool visit(AST::UiQualifiedId *ast)
    {
        qualifiedIds.append(ast);
        return false;
    }

    virtual bool visit(AST::IdentifierExpression *ast)
    {
        identifiers.append(ast);
        return false;
    }

    virtual bool visit(AST::FieldMemberExpression *ast)
    {
        fieldMembers.append(ast);
        return true;
    }
};

} // end of anonymous namespace


AST::Node *SemanticInfo::declaringMember(int cursorPosition) const
{
    AST::Node *declaringMember = 0;

    for (int i = ranges.size() - 1; i != -1; --i) {
        const Range &range = ranges.at(i);

        if (range.begin.isNull() || range.end.isNull()) {
            continue;
        } else if (cursorPosition >= range.begin.position() && cursorPosition <= range.end.position()) {
            declaringMember = range.ast;
            break;
        }
    }

    return declaringMember;
}

QmlJS::AST::Node *SemanticInfo::declaringMemberNoProperties(int cursorPosition) const
{
   AST::Node *node = declaringMember(cursorPosition);

   if (UiObjectDefinition *objectDefinition = cast<UiObjectDefinition*>(node)) {
       QString name = objectDefinition->qualifiedTypeNameId->name->asString();
       if (!name.isNull() && name.at(0).isLower()) {
           QList<AST::Node *> path = astPath(cursorPosition);
           if (path.size() > 1)
               return path.at(path.size() - 2);
       } else if (name.contains("GradientStop")) {
           QList<AST::Node *> path = astPath(cursorPosition);
           if (path.size() > 2)
               return path.at(path.size() - 3);
       }
   } else if (UiObjectBinding *objectBinding = cast<UiObjectBinding*>(node)) {
       QString name = objectBinding->qualifiedTypeNameId->name->asString();
       if (name.contains("Gradient")) {
           QList<AST::Node *> path = astPath(cursorPosition);
           if (path.size() > 1)
               return path.at(path.size() - 2);
       }
   }

   return node;
}

QList<AST::Node *> SemanticInfo::astPath(int cursorPosition) const
{
    QList<AST::Node *> path;

    foreach (const Range &range, ranges) {
        if (range.begin.isNull() || range.end.isNull()) {
            continue;
        } else if (cursorPosition >= range.begin.position() && cursorPosition <= range.end.position()) {
            path += range.ast;
        }
    }

    return path;
}

LookupContext::Ptr SemanticInfo::lookupContext(const QList<QmlJS::AST::Node *> &path) const
{
    Q_ASSERT(! m_context.isNull());

    if (m_context.isNull())
        return LookupContext::create(document, snapshot, path);

    return LookupContext::create(document, snapshot, *m_context, path);
}

static bool importContainsCursor(UiImport *importAst, unsigned cursorPosition)
{
    return cursorPosition >= importAst->firstSourceLocation().begin()
           && cursorPosition <= importAst->lastSourceLocation().end();
}

AST::Node *SemanticInfo::nodeUnderCursor(int pos) const
{
    if (! document)
        return 0;

    const unsigned cursorPosition = pos;

    foreach (const Interpreter::ImportInfo &import, document->bind()->imports()) {
        if (importContainsCursor(import.ast(), cursorPosition))
            return import.ast();
    }

    CollectASTNodes nodes;
    nodes.accept(document->ast());

    foreach (AST::UiQualifiedId *q, nodes.qualifiedIds) {
        if (cursorPosition >= q->identifierToken.begin()) {
            for (AST::UiQualifiedId *tail = q; tail; tail = tail->next) {
                if (! tail->next && cursorPosition <= tail->identifierToken.end())
                    return q;
            }
        }
    }

    foreach (AST::IdentifierExpression *id, nodes.identifiers) {
        if (cursorPosition >= id->identifierToken.begin() && cursorPosition <= id->identifierToken.end())
            return id;
    }

    foreach (AST::FieldMemberExpression *mem, nodes.fieldMembers) {
        if (mem->name && cursorPosition >= mem->identifierToken.begin() && cursorPosition <= mem->identifierToken.end())
            return mem;
    }

    return 0;
}

bool SemanticInfo::isValid() const
{
    if (document && m_context)
        return true;

    return false;
}

int SemanticInfo::revision() const
{
    if (document)
        return document->editorRevision();

    return 0;
}

QmlJSTextEditorWidget::QmlJSTextEditorWidget(QWidget *parent) :
    TextEditor::BaseTextEditorWidget(parent),
    m_outlineCombo(0),
    m_outlineModel(new QmlOutlineModel(this)),
    m_modelManager(0),
    m_contextPane(0),
    m_updateSelectedElements(false),
    m_findReferences(new FindReferences(this))
{
    qRegisterMetaType<QmlJSEditor::SemanticInfo>("QmlJSEditor::SemanticInfo");

    m_semanticHighlighter = new SemanticHighlighter(this);
    m_semanticHighlighter->start();

    setParenthesesMatchingEnabled(true);
    setMarksVisible(true);
    setCodeFoldingSupported(true);
    setIndenter(new Indenter);
    setAutoCompleter(new AutoCompleter);

    m_updateDocumentTimer = new QTimer(this);
    m_updateDocumentTimer->setInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
    m_updateDocumentTimer->setSingleShot(true);
    connect(m_updateDocumentTimer, SIGNAL(timeout()), this, SLOT(updateDocumentNow()));

    m_updateUsesTimer = new QTimer(this);
    m_updateUsesTimer->setInterval(UPDATE_USES_DEFAULT_INTERVAL);
    m_updateUsesTimer->setSingleShot(true);
    connect(m_updateUsesTimer, SIGNAL(timeout()), this, SLOT(updateUsesNow()));

    m_semanticRehighlightTimer = new QTimer(this);
    m_semanticRehighlightTimer->setInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
    m_semanticRehighlightTimer->setSingleShot(true);
    connect(m_semanticRehighlightTimer, SIGNAL(timeout()), this, SLOT(forceSemanticRehighlight()));

    connect(this, SIGNAL(textChanged()), this, SLOT(updateDocument()));

    connect(this, SIGNAL(textChanged()), this, SLOT(updateUses()));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(updateUses()));

    m_updateOutlineTimer = new QTimer(this);
    m_updateOutlineTimer->setInterval(UPDATE_OUTLINE_INTERVAL);
    m_updateOutlineTimer->setSingleShot(true);
    connect(m_updateOutlineTimer, SIGNAL(timeout()), this, SLOT(updateOutlineNow()));

    m_updateOutlineIndexTimer = new QTimer(this);
    m_updateOutlineIndexTimer->setInterval(UPDATE_OUTLINE_INTERVAL);
    m_updateOutlineIndexTimer->setSingleShot(true);
    connect(m_updateOutlineIndexTimer, SIGNAL(timeout()), this, SLOT(updateOutlineIndexNow()));

    m_cursorPositionTimer  = new QTimer(this);
    m_cursorPositionTimer->setInterval(UPDATE_OUTLINE_INTERVAL);
    m_cursorPositionTimer->setSingleShot(true);
    connect(m_cursorPositionTimer, SIGNAL(timeout()), this, SLOT(updateCursorPositionNow()));

    baseTextDocument()->setSyntaxHighlighter(new Highlighter(document()));
    baseTextDocument()->setCodec(QTextCodec::codecForName("UTF-8")); // qml files are defined to be utf-8

    m_modelManager = ExtensionSystem::PluginManager::instance()->getObject<ModelManagerInterface>();
    m_contextPane = ExtensionSystem::PluginManager::instance()->getObject<QmlJS::IContextPane>();


    if (m_contextPane) {
        connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(onCursorPositionChanged()));
        connect(m_contextPane, SIGNAL(closed()), this, SLOT(showTextMarker()));
    }
    m_oldCursorPosition = -1;

    if (m_modelManager) {
        m_semanticHighlighter->setModelManager(m_modelManager);
        connect(m_modelManager, SIGNAL(documentUpdated(QmlJS::Document::Ptr)),
                this, SLOT(onDocumentUpdated(QmlJS::Document::Ptr)));
        connect(m_modelManager, SIGNAL(libraryInfoUpdated(QString,QmlJS::LibraryInfo)),
                this, SLOT(forceSemanticRehighlightIfCurrentEditor()));
        connect(this->document(), SIGNAL(modificationChanged(bool)), this, SLOT(modificationChanged(bool)));
    }

    connect(m_semanticHighlighter, SIGNAL(changed(QmlJSEditor::SemanticInfo)),
            this, SLOT(updateSemanticInfo(QmlJSEditor::SemanticInfo)));

    connect(this, SIGNAL(refactorMarkerClicked(TextEditor::RefactorMarker)),
            SLOT(onRefactorMarkerClicked(TextEditor::RefactorMarker)));

    setRequestMarkEnabled(true);
}

QmlJSTextEditorWidget::~QmlJSTextEditorWidget()
{
    m_semanticHighlighter->abort();
    m_semanticHighlighter->wait();
}

SemanticInfo QmlJSTextEditorWidget::semanticInfo() const
{
    return m_semanticInfo;
}

int QmlJSTextEditorWidget::editorRevision() const
{
    return document()->revision();
}

bool QmlJSTextEditorWidget::isOutdated() const
{
    if (m_semanticInfo.revision() != editorRevision())
        return true;

    return false;
}

QmlOutlineModel *QmlJSTextEditorWidget::outlineModel() const
{
    return m_outlineModel;
}

QModelIndex QmlJSTextEditorWidget::outlineModelIndex()
{
    if (!m_outlineModelIndex.isValid()) {
        m_outlineModelIndex = indexForPosition(position());
        emit outlineModelIndexChanged(m_outlineModelIndex);
    }
    return m_outlineModelIndex;
}

Core::IEditor *QmlJSEditorEditable::duplicate(QWidget *parent)
{
    QmlJSTextEditorWidget *newEditor = new QmlJSTextEditorWidget(parent);
    newEditor->duplicateFrom(editorWidget());
    QmlJSEditorPlugin::instance()->initializeEditor(newEditor);
    return newEditor->editor();
}

QString QmlJSEditorEditable::id() const
{
    return QLatin1String(QmlJSEditor::Constants::C_QMLJSEDITOR_ID);
}

bool QmlJSEditorEditable::open(const QString &fileName)
{
    bool b = TextEditor::BaseTextEditor::open(fileName);
    editorWidget()->setMimeType(Core::ICore::instance()->mimeDatabase()->findByFile(QFileInfo(fileName)).type());
    return b;
}

Core::Context QmlJSEditorEditable::context() const
{
    return m_context;
}

void QmlJSTextEditorWidget::updateDocument()
{
    m_updateDocumentTimer->start(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
}

void QmlJSTextEditorWidget::updateDocumentNow()
{
    // ### move in the parser thread.

    m_updateDocumentTimer->stop();

    const QString fileName = file()->fileName();

    m_modelManager->updateSourceFiles(QStringList() << fileName, false);
}

static void appendExtraSelectionsForMessages(
        QList<QTextEdit::ExtraSelection> *selections,
        const QList<DiagnosticMessage> &messages,
        const QTextDocument *document)
{
    foreach (const DiagnosticMessage &d, messages) {
        const int line = d.loc.startLine;
        const int column = qMax(1U, d.loc.startColumn);

        QTextEdit::ExtraSelection sel;
        QTextCursor c(document->findBlockByNumber(line - 1));
        sel.cursor = c;

        sel.cursor.setPosition(c.position() + column - 1);

        if (d.loc.length == 0) {
            if (sel.cursor.atBlockEnd())
                sel.cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
            else
                sel.cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        } else {
            sel.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, d.loc.length);
        }

        if (d.isWarning())
            sel.format.setUnderlineColor(Qt::darkYellow);
        else
            sel.format.setUnderlineColor(Qt::red);

        sel.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        sel.format.setToolTip(d.message);

        selections->append(sel);
    }
}

void QmlJSTextEditorWidget::onDocumentUpdated(QmlJS::Document::Ptr doc)
{
    if (file()->fileName() != doc->fileName()
            || doc->editorRevision() != document()->revision()) {
        return;
    }

    if (doc->ast()) {
        // got a correctly parsed (or recovered) file.

        const SemanticHighlighterSource source = currentSource(/*force = */ true);
        m_semanticHighlighter->rehighlight(source);
    } else {
        // show parsing errors
        QList<QTextEdit::ExtraSelection> selections;
        appendExtraSelectionsForMessages(&selections, doc->diagnosticMessages(), document());
        setExtraSelections(CodeWarningsSelection, selections);
    }
}

void QmlJSTextEditorWidget::modificationChanged(bool changed)
{
    if (!changed && m_modelManager)
        m_modelManager->fileChangedOnDisk(file()->fileName());
}

void QmlJSTextEditorWidget::jumpToOutlineElement(int /*index*/)
{
    QModelIndex index = m_outlineCombo->view()->currentIndex();
    AST::SourceLocation location = m_outlineModel->sourceLocation(index);

    if (!location.isValid())
        return;

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    editorManager->cutForwardNavigationHistory();
    editorManager->addCurrentPositionToNavigationHistory();

    QTextCursor cursor = textCursor();
    cursor.setPosition(location.offset);
    setTextCursor(cursor);

    setFocus();
}

void QmlJSTextEditorWidget::updateOutlineNow()
{
    if (!m_semanticInfo.document)
        return;

    if (m_semanticInfo.document->editorRevision() != editorRevision()) {
        m_updateOutlineTimer->start();
        return;
    }

    m_outlineModel->update(m_semanticInfo);

    QTreeView *treeView = static_cast<QTreeView*>(m_outlineCombo->view());
    treeView->expandAll();

    updateOutlineIndexNow();
}

void QmlJSTextEditorWidget::updateOutlineIndexNow()
{
    if (m_updateOutlineTimer->isActive())
        return; // updateOutlineNow will call this function soon anyway

    if (!m_outlineModel->document())
        return;

    if (m_outlineModel->document()->editorRevision() != editorRevision()) {
        m_updateOutlineIndexTimer->start();
        return;
    }

    m_outlineModelIndex = QModelIndex(); // invalidate
    QModelIndex comboIndex = outlineModelIndex();

    if (comboIndex.isValid()) {
        bool blocked = m_outlineCombo->blockSignals(true);

        // There is no direct way to select a non-root item
        m_outlineCombo->setRootModelIndex(comboIndex.parent());
        m_outlineCombo->setCurrentIndex(comboIndex.row());
        m_outlineCombo->setRootModelIndex(QModelIndex());

        m_outlineCombo->blockSignals(blocked);
    }
}

static UiQualifiedId *qualifiedTypeNameId(Node *m)
{
    if (UiObjectDefinition *def = cast<UiObjectDefinition *>(m))
        return def->qualifiedTypeNameId;
    else if (UiObjectBinding *binding = cast<UiObjectBinding *>(m))
        return binding->qualifiedTypeNameId;
    return 0;
}

void QmlJSTextEditorWidget::updateCursorPositionNow()
{
    if (m_contextPane && document() && semanticInfo().isValid()
            && document()->revision() == semanticInfo().document->editorRevision())
    {
        Node *oldNode = m_semanticInfo.declaringMemberNoProperties(m_oldCursorPosition);
        Node *newNode = m_semanticInfo.declaringMemberNoProperties(position());
        if (oldNode != newNode && m_oldCursorPosition != -1)
            m_contextPane->apply(editor(), semanticInfo().document, LookupContext::Ptr(),newNode, false);
        if (m_contextPane->isAvailable(editor(), semanticInfo().document, newNode) &&
            !m_contextPane->widget()->isVisible()) {
            QList<TextEditor::RefactorMarker> markers;
            if (UiObjectMember *m = newNode->uiObjectMemberCast()) {
                const int start = qualifiedTypeNameId(m)->identifierToken.begin();
                for (UiQualifiedId *q = qualifiedTypeNameId(m); q; q = q->next) {
                    if (! q->next) {
                        const int end = q->identifierToken.end();
                        if (position() >= start && position() <= end) {
                            TextEditor::RefactorMarker marker;
                            QTextCursor tc(document());
                            tc.setPosition(end);
                            marker.cursor = tc;
                            marker.tooltip = tr("Show Qt Quick ToolBar");
                            markers.append(marker);
                        } else {
                             QList<TextEditor::RefactorMarker> markers;
                             setRefactorMarkers(markers);
                        }
                    }
                }
            }
            setRefactorMarkers(markers);
        } else if (oldNode != newNode) {
            QList<TextEditor::RefactorMarker> markers;
            setRefactorMarkers(markers);
        }
        m_oldCursorPosition = position();

        setSelectedElements();
    }
}

void QmlJSTextEditorWidget::showTextMarker()
{
    m_oldCursorPosition = -1;
    updateCursorPositionNow();
}

void QmlJSTextEditorWidget::updateUses()
{
    m_updateUsesTimer->start();
}

bool QmlJSTextEditorWidget::updateSelectedElements() const
{
    return m_updateSelectedElements;
}

void QmlJSTextEditorWidget::setUpdateSelectedElements(bool value)
{
    m_updateSelectedElements = value;
}

void QmlJSTextEditorWidget::renameId(const QString &oldId, const QString &newId)
{
    Utils::ChangeSet changeSet;

    foreach (const AST::SourceLocation &loc, m_semanticInfo.idLocations.value(oldId)) {
        changeSet.replace(loc.begin(), loc.end(), newId);
    }

    QTextCursor tc = textCursor();
    changeSet.apply(&tc);
}

void QmlJSTextEditorWidget::updateUsesNow()
{
    if (document()->revision() != m_semanticInfo.revision()) {
        updateUses();
        return;
    }

    m_updateUsesTimer->stop();

    QList<QTextEdit::ExtraSelection> selections;
    foreach (const AST::SourceLocation &loc, m_semanticInfo.idLocations.value(wordUnderCursor())) {
        if (! loc.isValid())
            continue;

        QTextEdit::ExtraSelection sel;
        sel.format = m_occurrencesFormat;
        sel.cursor = textCursor();
        sel.cursor.setPosition(loc.begin());
        sel.cursor.setPosition(loc.end(), QTextCursor::KeepAnchor);
        selections.append(sel);
    }

    setExtraSelections(CodeSemanticsSelection, selections);
}

class SelectedElement: protected Visitor
{
    unsigned m_cursorPositionStart;
    unsigned m_cursorPositionEnd;
    QList<UiObjectMember *> m_selectedMembers;
    LookupContext::Ptr m_lookupContext;

public:
    SelectedElement()
        : m_cursorPositionStart(0), m_cursorPositionEnd(0) {}

    QList<UiObjectMember *> operator()(LookupContext::Ptr lookupContext, unsigned startPosition, unsigned endPosition)
    {
        m_lookupContext = lookupContext;
        m_cursorPositionStart = startPosition;
        m_cursorPositionEnd = endPosition;
        m_selectedMembers.clear();
        Node::accept(lookupContext->document()->qmlProgram(), this);
        return m_selectedMembers;
    }

protected:

    bool isSelectable(UiObjectMember *member) const
    {
        UiQualifiedId *id = 0;
        if (UiObjectDefinition *def = cast<UiObjectDefinition *>(member))
            id = def->qualifiedTypeNameId;
        else if (UiObjectBinding *binding = cast<UiObjectBinding *>(member))
            id = binding->qualifiedTypeNameId;

        if (id) {
            QString name = id->name->asString();
            if (!name.isEmpty() && name.at(0).isUpper()) {
                return true;
            }
        }

        return false;
    }

    inline UiObjectInitializer *initializer(UiObjectMember *member) const
    {
        if (UiObjectDefinition *def = cast<UiObjectDefinition *>(member))
            return def->initializer;
        else if (UiObjectBinding *binding = cast<UiObjectBinding *>(member))
            return binding->initializer;
        return 0;
    }

    inline bool isIdBinding(UiObjectMember *member) const
    {
        if (UiScriptBinding *script = cast<UiScriptBinding *>(member)) {
            if (! script->qualifiedId)
                return false;
            else if (! script->qualifiedId->name)
                return false;
            else if (script->qualifiedId->next)
                return false;

            const QString propertyName = script->qualifiedId->name->asString();

            if (propertyName == QLatin1String("id"))
                return true;
        }

        return false;
    }

    inline bool containsCursor(unsigned begin, unsigned end)
    {
        return m_cursorPositionStart >= begin && m_cursorPositionEnd <= end;
    }

    inline bool intersectsCursor(unsigned begin, unsigned end)
    {
        return (m_cursorPositionEnd >= begin && m_cursorPositionStart <= end);
    }

    inline bool isRangeSelected() const
    {
        return (m_cursorPositionStart != m_cursorPositionEnd);
    }

    void postVisit(Node *ast)
    {
        if (!isRangeSelected() && !m_selectedMembers.isEmpty())
            return; // nothing to do, we already have the results.

        if (UiObjectMember *member = ast->uiObjectMemberCast()) {
            unsigned begin = member->firstSourceLocation().begin();
            unsigned end = member->lastSourceLocation().end();

            if ((isRangeSelected() && intersectsCursor(begin, end))
            || (!isRangeSelected() && containsCursor(begin, end)))
            {
                if (initializer(member) && isSelectable(member)) {
                    m_selectedMembers << member;
                    // move start towards end; this facilitates multiselection so that root is usually ignored.
                    m_cursorPositionStart = qMin(end, m_cursorPositionEnd);
                }
            }
        }
    }
};

void QmlJSTextEditorWidget::setSelectedElements()
{
    if (!m_updateSelectedElements)
        return;

    QTextCursor tc = textCursor();
    QString wordAtCursor;
    QList<int> offsets;

    unsigned startPos;
    unsigned endPos;

    if (tc.hasSelection()) {
        startPos = tc.selectionStart();
        endPos = tc.selectionEnd();
    } else {
        tc.movePosition(QTextCursor::StartOfWord);
        tc.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);

        startPos = textCursor().position();
        endPos = textCursor().position();
    }

    if (m_semanticInfo.isValid()) {
        SelectedElement selectedMembers;
        QList<UiObjectMember *> members = selectedMembers(m_semanticInfo.lookupContext(),
                                                          startPos, endPos);
        if (!members.isEmpty()) {
            foreach(UiObjectMember *m, members) {
                offsets << m->firstSourceLocation().begin();
            }
        }
    }
    wordAtCursor = tc.selectedText();

    emit selectedElementsChanged(offsets, wordAtCursor);
}

void QmlJSTextEditorWidget::updateFileName()
{
}

void QmlJSTextEditorWidget::renameIdUnderCursor()
{
    const QString id = wordUnderCursor();
    bool ok = false;
    const QString newId = QInputDialog::getText(Core::ICore::instance()->mainWindow(),
                                                tr("Rename..."),
                                                tr("New id:"),
                                                QLineEdit::Normal,
                                                id, &ok);
    if (ok) {
        renameId(id, newId);
    }
}

void QmlJSTextEditorWidget::setFontSettings(const TextEditor::FontSettings &fs)
{
    TextEditor::BaseTextEditorWidget::setFontSettings(fs);
    Highlighter *highlighter = qobject_cast<Highlighter*>(baseTextDocument()->syntaxHighlighter());
    if (!highlighter)
        return;

    highlighter->setFormats(fs.toTextCharFormats(highlighterFormatCategories()));
    highlighter->rehighlight();

    m_occurrencesFormat = fs.toTextCharFormat(QLatin1String(TextEditor::Constants::C_OCCURRENCES));
    m_occurrencesUnusedFormat = fs.toTextCharFormat(QLatin1String(TextEditor::Constants::C_OCCURRENCES_UNUSED));
    m_occurrencesUnusedFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    m_occurrencesUnusedFormat.setUnderlineColor(m_occurrencesUnusedFormat.foreground().color());
    m_occurrencesUnusedFormat.clearForeground();
    m_occurrencesUnusedFormat.setToolTip(tr("Unused variable"));
    m_occurrenceRenameFormat = fs.toTextCharFormat(QLatin1String(TextEditor::Constants::C_OCCURRENCES_RENAME));

    // only set the background, we do not want to modify foreground properties set by the syntax highlighter or the link
    m_occurrencesFormat.clearForeground();
    m_occurrenceRenameFormat.clearForeground();
}


QString QmlJSTextEditorWidget::wordUnderCursor() const
{
    QTextCursor tc = textCursor();
    const QChar ch = characterAt(tc.position() - 1);
    // make sure that we're not at the start of the next word.
    if (ch.isLetterOrNumber() || ch == QLatin1Char('_'))
        tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::StartOfWord);
    tc.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    const QString word = tc.selectedText();
    return word;
}

bool QmlJSTextEditorWidget::isClosingBrace(const QList<Token> &tokens) const
{

    if (tokens.size() == 1) {
        const Token firstToken = tokens.first();

        return firstToken.is(Token::RightBrace) || firstToken.is(Token::RightBracket);
    }

    return false;
}

TextEditor::BaseTextEditor *QmlJSTextEditorWidget::createEditor()
{
    QmlJSEditorEditable *editable = new QmlJSEditorEditable(this);
    createToolBar(editable);
    return editable;
}

void QmlJSTextEditorWidget::createToolBar(QmlJSEditorEditable *editor)
{
    m_outlineCombo = new QComboBox;
    m_outlineCombo->setMinimumContentsLength(22);
    m_outlineCombo->setModel(m_outlineModel);

    QTreeView *treeView = new QTreeView;
    treeView->header()->hide();
    treeView->setItemsExpandable(false);
    treeView->setRootIsDecorated(false);
    m_outlineCombo->setView(treeView);
    treeView->expandAll();

    //m_outlineCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    // Make the combo box prefer to expand
    QSizePolicy policy = m_outlineCombo->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_outlineCombo->setSizePolicy(policy);

    connect(m_outlineCombo, SIGNAL(activated(int)), this, SLOT(jumpToOutlineElement(int)));
    connect(this, SIGNAL(cursorPositionChanged()), m_updateOutlineIndexTimer, SLOT(start()));

    connect(file(), SIGNAL(changed()), this, SLOT(updateFileName()));

    editor->insertExtraToolBarWidget(TextEditor::BaseTextEditor::Left, m_outlineCombo);
}

TextEditor::BaseTextEditorWidget::Link QmlJSTextEditorWidget::findLinkAt(const QTextCursor &cursor, bool /*resolveTarget*/)
{
    const SemanticInfo semanticInfo = m_semanticInfo;
    if (! semanticInfo.isValid())
        return Link();

    const unsigned cursorPosition = cursor.position();

    AST::Node *node = semanticInfo.nodeUnderCursor(cursorPosition);

    if (AST::UiImport *importAst = cast<AST::UiImport *>(node)) {
        // if it's a file import, link to the file
        foreach (const Interpreter::ImportInfo &import, semanticInfo.document->bind()->imports()) {
            if (import.ast() == importAst && import.type() == Interpreter::ImportInfo::FileImport) {
                BaseTextEditorWidget::Link link(import.name());
                link.begin = importAst->firstSourceLocation().begin();
                link.end = importAst->lastSourceLocation().end();
                return link;
            }
        }
        return Link();
    }

    LookupContext::Ptr lookupContext = semanticInfo.lookupContext(semanticInfo.astPath(cursorPosition));
    Evaluate evaluator(lookupContext->context());
    const Interpreter::Value *value = evaluator.reference(node);

    QString fileName;
    int line = 0, column = 0;

    if (! (value && value->getSourceLocation(&fileName, &line, &column)))
        return Link();

    BaseTextEditorWidget::Link link;
    link.fileName = fileName;
    link.line = line;
    link.column = column - 1; // adjust the column

    if (AST::UiQualifiedId *q = AST::cast<AST::UiQualifiedId *>(node)) {
        for (AST::UiQualifiedId *tail = q; tail; tail = tail->next) {
            if (! tail->next && cursorPosition <= tail->identifierToken.end()) {
                link.begin = tail->identifierToken.begin();
                link.end = tail->identifierToken.end();
                return link;
            }
        }

    } else if (AST::IdentifierExpression *id = AST::cast<AST::IdentifierExpression *>(node)) {
        link.begin = id->firstSourceLocation().begin();
        link.end = id->lastSourceLocation().end();
        return link;

    } else if (AST::FieldMemberExpression *mem = AST::cast<AST::FieldMemberExpression *>(node)) {
        link.begin = mem->lastSourceLocation().begin();
        link.end = mem->lastSourceLocation().end();
        return link;
    }

    return Link();
}

void QmlJSTextEditorWidget::followSymbolUnderCursor()
{
    openLink(findLinkAt(textCursor()));
}

void QmlJSTextEditorWidget::findUsages()
{
    m_findReferences->findUsages(file()->fileName(), textCursor().position());
}

void QmlJSTextEditorWidget::showContextPane()
{
    if (m_contextPane && m_semanticInfo.isValid()) {
        Node *newNode = m_semanticInfo.declaringMemberNoProperties(position());
        m_contextPane->apply(editor(), m_semanticInfo.document, m_semanticInfo.lookupContext(), newNode, false, true);
        m_oldCursorPosition = position();
        QList<TextEditor::RefactorMarker> markers;
        setRefactorMarkers(markers);
    }
}

void QmlJSTextEditorWidget::performQuickFix(int index)
{
    TextEditor::QuickFixOperation::Ptr op = m_quickFixes.at(index);
    op->perform();
}

void QmlJSTextEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu *menu = new QMenu();

    QMenu *refactoringMenu = new QMenu(tr("Refactoring"), menu);

    // Conditionally add the rename-id action:
    const QString id = wordUnderCursor();
    const QList<AST::SourceLocation> &locations = m_semanticInfo.idLocations.value(id);
    if (! locations.isEmpty()) {
        QAction *a = refactoringMenu->addAction(tr("Rename id '%1'...").arg(id));
        connect(a, SIGNAL(triggered()), this, SLOT(renameIdUnderCursor()));
    }

    // Add other refactoring actions:
    QmlJSQuickFixCollector *quickFixCollector = QmlJSEditorPlugin::instance()->quickFixCollector();
    QSignalMapper mapper;
    connect(&mapper, SIGNAL(mapped(int)), this, SLOT(performQuickFix(int)));

    if (! isOutdated()) {
        if (quickFixCollector->startCompletion(editor()) != -1) {
            m_quickFixes = quickFixCollector->quickFixes();

            for (int index = 0; index < m_quickFixes.size(); ++index) {
                TextEditor::QuickFixOperation::Ptr op = m_quickFixes.at(index);
                QAction *action = refactoringMenu->addAction(op->description());
                mapper.setMapping(action, index);
                connect(action, SIGNAL(triggered()), &mapper, SLOT(map()));
            }
        }
    }

    refactoringMenu->setEnabled(!refactoringMenu->isEmpty());

    if (Core::ActionContainer *mcontext = Core::ICore::instance()->actionManager()->actionContainer(QmlJSEditor::Constants::M_CONTEXT)) {
        QMenu *contextMenu = mcontext->menu();
        foreach (QAction *action, contextMenu->actions()) {
            menu->addAction(action);
            if (action->objectName() == QmlJSEditor::Constants::M_REFACTORING_MENU_INSERTION_POINT)
                menu->addMenu(refactoringMenu);
        }
    }

    appendStandardContextMenuActions(menu);

    menu->exec(e->globalPos());
    menu->deleteLater();
    quickFixCollector->cleanup();
    m_quickFixes.clear();
}

bool QmlJSTextEditorWidget::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::ShortcutOverride:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape && m_contextPane) {
            if (hideContextPane()) {
                e->accept();
                return true;
            }
        }
        break;
    default:
        break;
    }

    return BaseTextEditorWidget::event(e);
}


void QmlJSTextEditorWidget::wheelEvent(QWheelEvent *event)
{
    bool visible = false;
    if (m_contextPane && m_contextPane->widget()->isVisible())
        visible = true;

    BaseTextEditorWidget::wheelEvent(event);

    if (visible) {
        LookupContext::Ptr lookupContext;
        if (m_semanticInfo.isValid())
            lookupContext = m_semanticInfo.lookupContext();
        m_contextPane->apply(editor(), semanticInfo().document, QmlJS::LookupContext::Ptr(), m_semanticInfo.declaringMemberNoProperties(m_oldCursorPosition), false, true);
    }
}

void QmlJSTextEditorWidget::resizeEvent(QResizeEvent *event)
{
    BaseTextEditorWidget::resizeEvent(event);
    hideContextPane();
}

 void QmlJSTextEditorWidget::scrollContentsBy(int dx, int dy)
 {
     BaseTextEditorWidget::scrollContentsBy(dx, dy);
     hideContextPane();
 }

void QmlJSTextEditorWidget::unCommentSelection()
{
    Utils::unCommentSelection(this);
}

void QmlJSTextEditorWidget::forceSemanticRehighlight()
{
    m_semanticHighlighter->rehighlight(currentSource(/* force = */ true));
}

void QmlJSEditor::QmlJSTextEditorWidget::forceSemanticRehighlightIfCurrentEditor()
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    if (editorManager->currentEditor() == editor())
        forceSemanticRehighlight();
}

void QmlJSTextEditorWidget::semanticRehighlight()
{
    m_semanticHighlighter->rehighlight(currentSource());
}

void QmlJSTextEditorWidget::updateSemanticInfo(const SemanticInfo &semanticInfo)
{
    if (semanticInfo.revision() != document()->revision()) {
        // got outdated semantic info
        semanticRehighlight();
        return;
    }

    m_semanticInfo = semanticInfo;
    Document::Ptr doc = semanticInfo.document;

    // create the ranges
    CreateRanges createRanges;
    m_semanticInfo.ranges = createRanges(document(), doc);

    // Refresh the ids
    FindIdDeclarations updateIds;
    m_semanticInfo.idLocations = updateIds(doc);

    FindDeclarations findDeclarations;
    m_semanticInfo.declarations = findDeclarations(doc->ast());

    if (m_contextPane) {
        Node *newNode = m_semanticInfo.declaringMemberNoProperties(position());
        if (newNode) {
            m_contextPane->apply(editor(), semanticInfo.document, LookupContext::Ptr(), newNode, true);
            m_cursorPositionTimer->start(); //update text marker
        }
    }

    // update outline
    m_updateOutlineTimer->start();

    // update warning/error extra selections
    QList<QTextEdit::ExtraSelection> selections;
    appendExtraSelectionsForMessages(&selections, doc->diagnosticMessages(), document());
    appendExtraSelectionsForMessages(&selections, m_semanticInfo.semanticMessages, document());
    setExtraSelections(CodeWarningsSelection, selections);
}

void QmlJSTextEditorWidget::onRefactorMarkerClicked(const TextEditor::RefactorMarker &)
{
    showContextPane();
}

void QmlJSTextEditorWidget::onCursorPositionChanged()
{
    m_cursorPositionTimer->start();
}

QModelIndex QmlJSTextEditorWidget::indexForPosition(unsigned cursorPosition, const QModelIndex &rootIndex) const
{
    QModelIndex lastIndex = rootIndex;


    const int rowCount = m_outlineModel->rowCount(rootIndex);
    for (int i = 0; i < rowCount; ++i) {
        QModelIndex childIndex = m_outlineModel->index(i, 0, rootIndex);
        AST::SourceLocation location = m_outlineModel->sourceLocation(childIndex);

        if ((cursorPosition >= location.offset)
              && (cursorPosition <= location.offset + location.length)) {
            lastIndex = childIndex;
            break;
        }
    }

    if (lastIndex != rootIndex) {
        // recurse
        lastIndex = indexForPosition(cursorPosition, lastIndex);
    }
    return lastIndex;
}

bool QmlJSTextEditorWidget::hideContextPane()
{
    bool b = (m_contextPane) && m_contextPane->widget()->isVisible();
    if (b) {
        m_contextPane->apply(editor(), semanticInfo().document, LookupContext::Ptr(), 0, false);
    }
    return b;
}

QVector<QString> QmlJSTextEditorWidget::highlighterFormatCategories()
{
    /*
        NumberFormat,
        StringFormat,
        TypeFormat,
        KeywordFormat,
        LabelFormat,
        CommentFormat,
        VisualWhitespace,
     */
    static QVector<QString> categories;
    if (categories.isEmpty()) {
        categories << QLatin1String(TextEditor::Constants::C_NUMBER)
                << QLatin1String(TextEditor::Constants::C_STRING)
                << QLatin1String(TextEditor::Constants::C_TYPE)
                << QLatin1String(TextEditor::Constants::C_KEYWORD)
                << QLatin1String(TextEditor::Constants::C_FIELD)
                << QLatin1String(TextEditor::Constants::C_COMMENT)
                << QLatin1String(TextEditor::Constants::C_VISUAL_WHITESPACE);
    }
    return categories;
}

SemanticHighlighterSource QmlJSTextEditorWidget::currentSource(bool force)
{
    int line = 0, column = 0;
    convertPosition(position(), &line, &column);

    const Snapshot snapshot = m_modelManager->snapshot();
    const QString fileName = file()->fileName();

    QString code;
    if (force || m_semanticInfo.revision() != document()->revision())
        code = toPlainText(); // get the source code only when needed.

    const unsigned revision = document()->revision();
    SemanticHighlighterSource source(snapshot, fileName, code,
                                       line, column, revision);
    source.force = force;
    return source;
}
