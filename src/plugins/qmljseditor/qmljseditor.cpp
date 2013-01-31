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

#include "qmljseditor.h"
#include "qmljseditoreditable.h"
#include "qmljseditorconstants.h"
#include "qmljshighlighter.h"
#include "qmljseditorplugin.h"
#include "qmloutlinemodel.h"
#include "qmljsfindreferences.h"
#include "qmljssemanticinfoupdater.h"
#include "qmljsautocompleter.h"
#include "qmljscompletionassist.h"
#include "qmljsquickfixassist.h"
#include "qmljssemantichighlighter.h"

#include <qmljs/qmljsbind.h>
#include <qmljs/qmljsevaluate.h>
#include <qmljs/qmljsicontextpane.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsutils.h>

#include <qmljstools/qmljsindenter.h>
#include <qmljstools/qmljsqtstylecodeformatter.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/id.h>
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
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/basicproposalitemlistmodel.h>
#include <qmldesigner/qmldesignerconstants.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/changeset.h>
#include <utils/uncommentselection.h>
#include <utils/qtcassert.h>
#include <utils/annotateditemdelegate.h>

#include <QFileInfo>
#include <QSignalMapper>
#include <QTimer>
#include <QScopedPointer>
#include <QTextCodec>

#include <QMenu>
#include <QComboBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QToolBar>
#include <QTreeView>

enum {
    UPDATE_DOCUMENT_DEFAULT_INTERVAL = 100,
    UPDATE_USES_DEFAULT_INTERVAL = 150,
    UPDATE_OUTLINE_INTERVAL = 500 // msecs after new semantic info has been arrived / cursor has moved
};

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJSTools;

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
            if (!id->name.isEmpty())
                text += id->name;
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
                    if (!idExpr->name.isEmpty()) {
                        const QString &id = idExpr->name.toString();
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
        if (!node->name.isEmpty()) {
            const QString &name = node->name.toString();

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
            if (!id->name.isEmpty())
                text += id->name;
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
        if (ast->name.isEmpty())
            return false;

        Declaration decl;
        init(&decl, ast);

        decl.text.fill(QLatin1Char(' '), _depth);
        decl.text += ast->name;

        decl.text += QLatin1Char('(');
        for (FormalParameterList *it = ast->formals; it; it = it->next) {
            if (!it->name.isEmpty())
                decl.text += it->name;

            if (it->next)
                decl.text += QLatin1String(", ");
        }

        decl.text += QLatin1Char(')');

        _declarations.append(decl);

        return false;
    }

    virtual bool visit(AST::VariableDeclaration *ast)
    {
        if (ast->name.isEmpty())
            return false;

        Declaration decl;
        decl.text.fill(QLatin1Char(' '), _depth);
        decl.text += ast->name;

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

    virtual bool visit(AST::UiScriptBinding *ast)
    {
        if (AST::Block *block = AST::cast<AST::Block *>(ast->statement))
            _ranges.append(createRange(ast, block));
        return true;
    }

    Range createRange(AST::UiObjectMember *member, AST::UiObjectInitializer *ast)
    {
        return createRange(member, member->firstSourceLocation(), ast->rbraceToken);
    }

    Range createRange(AST::FunctionExpression *ast)
    {
        return createRange(ast, ast->lbraceToken, ast->rbraceToken);
    }

    Range createRange(AST::UiScriptBinding *ast, AST::Block *block)
    {
        return createRange(ast, block->lbraceToken, block->rbraceToken);
    }

    Range createRange(AST::Node *ast, AST::SourceLocation start, AST::SourceLocation end)
    {
        Range range;

        range.ast = ast;

        range.begin = QTextCursor(_textDocument);
        range.begin.setPosition(start.begin());

        range.end = QTextCursor(_textDocument);
        range.end.setPosition(end.end());

        return range;
    }

};

} // end of anonymous namespace


QmlJSTextEditorWidget::QmlJSTextEditorWidget(QWidget *parent) :
    TextEditor::BaseTextEditorWidget(parent),
    m_outlineCombo(0),
    m_outlineModel(new QmlOutlineModel(this)),
    m_modelManager(0),
    m_futureSemanticInfoRevision(0),
    m_contextPane(0),
    m_findReferences(new FindReferences(this)),
    m_semanticHighlighter(new SemanticHighlighter(this))
{
    m_semanticInfoUpdater = new SemanticInfoUpdater(this);
    m_semanticInfoUpdater->start();

    setParenthesesMatchingEnabled(true);
    setMarksVisible(true);
    setCodeFoldingSupported(true);
    setIndenter(new Indenter);
    setAutoCompleter(new AutoCompleter);

    m_updateDocumentTimer = new QTimer(this);
    m_updateDocumentTimer->setInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
    m_updateDocumentTimer->setSingleShot(true);
    connect(m_updateDocumentTimer, SIGNAL(timeout()), this, SLOT(reparseDocumentNow()));

    m_updateUsesTimer = new QTimer(this);
    m_updateUsesTimer->setInterval(UPDATE_USES_DEFAULT_INTERVAL);
    m_updateUsesTimer->setSingleShot(true);
    connect(m_updateUsesTimer, SIGNAL(timeout()), this, SLOT(updateUsesNow()));

    m_updateSemanticInfoTimer = new QTimer(this);
    m_updateSemanticInfoTimer->setInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
    m_updateSemanticInfoTimer->setSingleShot(true);
    connect(m_updateSemanticInfoTimer, SIGNAL(timeout()), this, SLOT(updateSemanticInfoNow()));

    connect(this, SIGNAL(textChanged()), this, SLOT(reparseDocument()));

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

    m_modelManager = QmlJS::ModelManagerInterface::instance();
    m_contextPane = ExtensionSystem::PluginManager::getObject<QmlJS::IContextPane>();


    if (m_contextPane) {
        connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(onCursorPositionChanged()));
        connect(m_contextPane, SIGNAL(closed()), this, SLOT(showTextMarker()));
    }
    m_oldCursorPosition = -1;

    if (m_modelManager) {
        connect(m_modelManager, SIGNAL(documentUpdated(QmlJS::Document::Ptr)),
                this, SLOT(onDocumentUpdated(QmlJS::Document::Ptr)));
        connect(m_modelManager, SIGNAL(libraryInfoUpdated(QString,QmlJS::LibraryInfo)),
                this, SLOT(updateSemanticInfo()));
        connect(this->document(), SIGNAL(modificationChanged(bool)), this, SLOT(modificationChanged(bool)));
    }

    connect(m_semanticInfoUpdater, SIGNAL(updated(QmlJSTools::SemanticInfo)),
            this, SLOT(acceptNewSemanticInfo(QmlJSTools::SemanticInfo)));

    connect(this, SIGNAL(refactorMarkerClicked(TextEditor::RefactorMarker)),
            SLOT(onRefactorMarkerClicked(TextEditor::RefactorMarker)));

    setRequestMarkEnabled(true);
}

QmlJSTextEditorWidget::~QmlJSTextEditorWidget()
{
    hideContextPane();
    m_semanticInfoUpdater->abort();
    m_semanticInfoUpdater->wait();
}

SemanticInfo QmlJSTextEditorWidget::semanticInfo() const
{
    return m_semanticInfo;
}

int QmlJSTextEditorWidget::editorRevision() const
{
    return document()->revision();
}

bool QmlJSTextEditorWidget::isSemanticInfoOutdated() const
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

Core::Id QmlJSEditorEditable::id() const
{
    return Core::Id(QmlJSEditor::Constants::C_QMLJSEDITOR_ID);
}

bool QmlJSEditorEditable::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    bool b = TextEditor::BaseTextEditor::open(errorString, fileName, realFileName);
    editorWidget()->setMimeType(Core::ICore::mimeDatabase()->findByFile(QFileInfo(fileName)).type());
    return b;
}

void QmlJSTextEditorWidget::reparseDocument()
{
    m_updateDocumentTimer->start();
}

void QmlJSTextEditorWidget::reparseDocumentNow()
{
    m_updateDocumentTimer->stop();

    const QString fileName = editorDocument()->fileName();
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

static void appendExtraSelectionsForMessages(
        QList<QTextEdit::ExtraSelection> *selections,
        const QList<StaticAnalysis::Message> &messages,
        const QTextDocument *document)
{
    foreach (const StaticAnalysis::Message &d, messages) {
        const int line = d.location.startLine;
        const int column = qMax(1U, d.location.startColumn);

        QTextEdit::ExtraSelection sel;
        QTextCursor c(document->findBlockByNumber(line - 1));
        sel.cursor = c;

        sel.cursor.setPosition(c.position() + column - 1);

        if (d.location.length == 0) {
            if (sel.cursor.atBlockEnd())
                sel.cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
            else
                sel.cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        } else {
            sel.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, d.location.length);
        }

        if (d.severity == StaticAnalysis::Warning || d.severity == StaticAnalysis::MaybeWarning)
            sel.format.setUnderlineColor(Qt::darkYellow);
        else if (d.severity == StaticAnalysis::Error || d.severity == StaticAnalysis::MaybeError)
            sel.format.setUnderlineColor(Qt::red);
        else if (d.severity == StaticAnalysis::Hint)
            sel.format.setUnderlineColor(Qt::darkGreen);

        sel.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        sel.format.setToolTip(d.message);

        selections->append(sel);
    }
}

void QmlJSTextEditorWidget::onDocumentUpdated(QmlJS::Document::Ptr doc)
{
    if (editorDocument()->fileName() != doc->fileName())
        return;

    if (doc->editorRevision() != editorRevision()) {
        // Maybe a dependency changed and our semantic info is now outdated.
        // Ignore 0-revision documents though, we get them when a file is initially opened
        // in an editor.
        if (doc->editorRevision() != 0)
            updateSemanticInfo();
        return;
    }

    //qDebug() << doc->fileName() << "was reparsed";

    if (doc->ast()) {
        // got a correctly parsed (or recovered) file.
        m_futureSemanticInfoRevision = doc->editorRevision();
        m_semanticInfoUpdater->update(doc, m_modelManager->snapshot());
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
        m_modelManager->fileChangedOnDisk(editorDocument()->fileName());
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

class QtQuickToolbarMarker {};
Q_DECLARE_METATYPE(QtQuickToolbarMarker)

template <class T>
static QList<TextEditor::RefactorMarker> removeMarkersOfType(const QList<TextEditor::RefactorMarker> &markers)
{
    QList<TextEditor::RefactorMarker> result;
    foreach (const TextEditor::RefactorMarker &marker, markers) {
        if (!marker.data.canConvert<T>())
            result += marker;
    }
    return result;
}

void QmlJSTextEditorWidget::updateCursorPositionNow()
{
    if (m_contextPane && document() && semanticInfo().isValid()
            && document()->revision() == semanticInfo().document->editorRevision())
    {
        Node *oldNode = m_semanticInfo.declaringMemberNoProperties(m_oldCursorPosition);
        Node *newNode = m_semanticInfo.declaringMemberNoProperties(position());
        if (oldNode != newNode && m_oldCursorPosition != -1)
            m_contextPane->apply(editor(), semanticInfo().document, 0, newNode, false);

        if (m_contextPane->isAvailable(editor(), semanticInfo().document, newNode) &&
            !m_contextPane->widget()->isVisible()) {
            QList<TextEditor::RefactorMarker> markers = removeMarkersOfType<QtQuickToolbarMarker>(refactorMarkers());
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
                            marker.data = QVariant::fromValue(QtQuickToolbarMarker());
                            markers.append(marker);
                        }
                    }
                }
            }
            setRefactorMarkers(markers);
        } else if (oldNode != newNode) {
            setRefactorMarkers(removeMarkersOfType<QtQuickToolbarMarker>(refactorMarkers()));
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
    if (m_semanticHighlighter->startRevision() != editorRevision())
        m_semanticHighlighter->cancel();
    m_updateUsesTimer->start();
}


void QmlJSTextEditorWidget::updateUsesNow()
{
    if (isSemanticInfoOutdated()) {
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

public:
    SelectedElement()
        : m_cursorPositionStart(0), m_cursorPositionEnd(0) {}

    QList<UiObjectMember *> operator()(const Document::Ptr &doc, unsigned startPosition, unsigned endPosition)
    {
        m_cursorPositionStart = startPosition;
        m_cursorPositionEnd = endPosition;
        m_selectedMembers.clear();
        Node::accept(doc->qmlProgram(), this);
        return m_selectedMembers;
    }

protected:

    bool isSelectable(UiObjectMember *member) const
    {
        UiQualifiedId *id = qualifiedTypeNameId(member);
        if (id) {
            const QStringRef &name = id->name;
            if (!name.isEmpty() && name.at(0).isUpper())
                return true;
        }

        return false;
    }

    inline bool isIdBinding(UiObjectMember *member) const
    {
        if (UiScriptBinding *script = cast<UiScriptBinding *>(member)) {
            if (! script->qualifiedId)
                return false;
            else if (script->qualifiedId->name.isEmpty())
                return false;
            else if (script->qualifiedId->next)
                return false;

            const QStringRef &propertyName = script->qualifiedId->name;

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
                if (initializerOfObject(member) && isSelectable(member)) {
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
    if (!receivers(SIGNAL(selectedElementsChanged(QList<QmlJS::AST::UiObjectMember*>,QString))))
        return;

    QTextCursor tc = textCursor();
    QString wordAtCursor;
    QList<UiObjectMember *> offsets;

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
        QList<UiObjectMember *> members = selectedMembers(m_semanticInfo.document,
                                                          startPos, endPos);
        if (!members.isEmpty()) {
            foreach (UiObjectMember *m, members) {
                offsets << m;
            }
        }
    }
    wordAtCursor = tc.selectedText();

    emit selectedElementsChanged(offsets, wordAtCursor);
}

void QmlJSTextEditorWidget::updateFileName()
{
}

void QmlJSTextEditorWidget::setFontSettings(const TextEditor::FontSettings &fs)
{
    TextEditor::BaseTextEditorWidget::setFontSettings(fs);
    Highlighter *highlighter = qobject_cast<Highlighter*>(baseTextDocument()->syntaxHighlighter());
    if (!highlighter)
        return;

    highlighter->setFormats(fs.toTextCharFormats(highlighterFormatCategories()));
    highlighter->rehighlight();

    m_occurrencesFormat = fs.toTextCharFormat(TextEditor::C_OCCURRENCES);
    m_occurrencesUnusedFormat = fs.toTextCharFormat(TextEditor::C_OCCURRENCES_UNUSED);
    m_occurrencesUnusedFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    m_occurrencesUnusedFormat.setUnderlineColor(m_occurrencesUnusedFormat.foreground().color());
    m_occurrencesUnusedFormat.clearForeground();
    m_occurrencesUnusedFormat.setToolTip(tr("Unused variable"));
    m_occurrenceRenameFormat = fs.toTextCharFormat(TextEditor::C_OCCURRENCES_RENAME);

    // only set the background, we do not want to modify foreground properties set by the syntax highlighter or the link
    m_occurrencesFormat.clearForeground();
    m_occurrenceRenameFormat.clearForeground();

    m_semanticHighlighter->updateFontSettings(fs);
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

    Utils::AnnotatedItemDelegate *itemDelegate = new Utils::AnnotatedItemDelegate(this);
    itemDelegate->setDelimiter(QLatin1String(" "));
    itemDelegate->setAnnotationRole(QmlOutlineModel::AnnotationRole);
    treeView->setItemDelegateForColumn(0, itemDelegate);

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

    connect(editorDocument(), SIGNAL(changed()), this, SLOT(updateFileName()));

    editor->insertExtraToolBarWidget(TextEditor::BaseTextEditor::Left, m_outlineCombo);
}

TextEditor::BaseTextEditorWidget::Link QmlJSTextEditorWidget::findLinkAt(const QTextCursor &cursor, bool /*resolveTarget*/)
{
    const SemanticInfo semanticInfo = m_semanticInfo;
    if (! semanticInfo.isValid())
        return Link();

    const unsigned cursorPosition = cursor.position();

    AST::Node *node = semanticInfo.astNodeAt(cursorPosition);
    QTC_ASSERT(node, return Link());

    if (AST::UiImport *importAst = cast<AST::UiImport *>(node)) {
        // if it's a file import, link to the file
        foreach (const ImportInfo &import, semanticInfo.document->bind()->imports()) {
            if (import.ast() == importAst && import.type() == ImportInfo::FileImport) {
                BaseTextEditorWidget::Link link(import.path());
                link.begin = importAst->firstSourceLocation().begin();
                link.end = importAst->lastSourceLocation().end();
                return link;
            }
        }
        return Link();
    }

    // string literals that could refer to a file link to them
    if (StringLiteral *literal = cast<StringLiteral *>(node)) {
        const QString &text = literal->value.toString();
        BaseTextEditorWidget::Link link;
        link.begin = literal->literalToken.begin();
        link.end = literal->literalToken.end();
        if (semanticInfo.snapshot.document(text)) {
            link.fileName = text;
            return link;
        }
        const QString relative = QString::fromLatin1("%1/%2").arg(
                    semanticInfo.document->path(),
                    text);
        if (semanticInfo.snapshot.document(relative)) {
            link.fileName = relative;
            return link;
        }
    }

    const ScopeChain scopeChain = semanticInfo.scopeChain(semanticInfo.rangePath(cursorPosition));
    Evaluate evaluator(&scopeChain);
    const Value *value = evaluator.reference(node);

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

void QmlJSTextEditorWidget::findUsages()
{
    m_findReferences->findUsages(editorDocument()->fileName(), textCursor().position());
}

void QmlJSTextEditorWidget::renameUsages()
{
    m_findReferences->renameUsages(editorDocument()->fileName(), textCursor().position());
}

void QmlJSTextEditorWidget::showContextPane()
{
    if (m_contextPane && m_semanticInfo.isValid()) {
        Node *newNode = m_semanticInfo.declaringMemberNoProperties(position());
        ScopeChain scopeChain = m_semanticInfo.scopeChain(m_semanticInfo.rangePath(position()));
        m_contextPane->apply(editor(), m_semanticInfo.document,
                             &scopeChain,
                             newNode, false, true);
        m_oldCursorPosition = position();
        setRefactorMarkers(removeMarkersOfType<QtQuickToolbarMarker>(refactorMarkers()));
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

    QSignalMapper mapper;
    connect(&mapper, SIGNAL(mapped(int)), this, SLOT(performQuickFix(int)));
    if (! isSemanticInfoOutdated()) {
        TextEditor::IAssistInterface *interface =
                createAssistInterface(TextEditor::QuickFix, TextEditor::ExplicitlyInvoked);
        if (interface) {
            QScopedPointer<TextEditor::IAssistProcessor> processor(
                        QmlJSEditorPlugin::instance()->quickFixAssistProvider()->createProcessor());
            QScopedPointer<TextEditor::IAssistProposal> proposal(processor->perform(interface));
            if (!proposal.isNull()) {
                TextEditor::BasicProposalItemListModel *model =
                        static_cast<TextEditor::BasicProposalItemListModel *>(proposal->model());
                for (int index = 0; index < model->size(); ++index) {
                    TextEditor::BasicProposalItem *item =
                            static_cast<TextEditor::BasicProposalItem *>(model->proposalItem(index));
                    TextEditor::QuickFixOperation::Ptr op =
                            item->data().value<TextEditor::QuickFixOperation::Ptr>();
                    m_quickFixes.append(op);
                    QAction *action = refactoringMenu->addAction(op->description());
                    mapper.setMapping(action, index);
                    connect(action, SIGNAL(triggered()), &mapper, SLOT(map()));
                }
                delete model;
            }
        }
    }

    refactoringMenu->setEnabled(!refactoringMenu->isEmpty());

    if (Core::ActionContainer *mcontext = Core::ActionManager::actionContainer(QmlJSEditor::Constants::M_CONTEXT)) {
        QMenu *contextMenu = mcontext->menu();
        foreach (QAction *action, contextMenu->actions()) {
            menu->addAction(action);
            if (action->objectName() == QLatin1String(QmlJSEditor::Constants::M_REFACTORING_MENU_INSERTION_POINT))
                menu->addMenu(refactoringMenu);
            if (action->objectName() == QLatin1String(QmlJSEditor::Constants::SHOW_QT_QUICK_HELPER)) {
                bool enabled = m_contextPane->isAvailable(editor(), semanticInfo().document, m_semanticInfo.declaringMemberNoProperties(position()));
                action->setEnabled(enabled);
            }
        }
    }

    appendStandardContextMenuActions(menu);

    menu->exec(e->globalPos());
    menu->deleteLater();
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

    if (visible)
        m_contextPane->apply(editor(), semanticInfo().document, 0, m_semanticInfo.declaringMemberNoProperties(m_oldCursorPosition), false, true);
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

void QmlJSTextEditorWidget::setTabSettings(const TextEditor::TabSettings &ts)
{
    QmlJSTools::CreatorCodeFormatter formatter(ts);
    formatter.invalidateCache(document());

    TextEditor::BaseTextEditorWidget::setTabSettings(ts);
}

void QmlJSTextEditorWidget::updateSemanticInfo()
{
    // If the editor is newer than the future semantic info, new semantic infos
    // won't be accepted anyway. What we need is a reparse.
    if (editorRevision() != m_futureSemanticInfoRevision)
        return;

    // Save time by not doing it for non-active editors.
    if (Core::EditorManager::currentEditor() != editor())
        return;

    m_updateSemanticInfoTimer->start();
}

void QmlJSTextEditorWidget::updateSemanticInfoNow()
{
    // If the editor is newer than the future semantic info, new semantic infos
    // won't be accepted anyway. What we need is a reparse.
    if (editorRevision() != m_futureSemanticInfoRevision)
        return;

    m_updateSemanticInfoTimer->stop();

    m_semanticInfoUpdater->reupdate(m_modelManager->snapshot());
}

void QmlJSTextEditorWidget::acceptNewSemanticInfo(const SemanticInfo &semanticInfo)
{
    if (semanticInfo.revision() != editorRevision()) {
        // ignore outdated semantic infos
        return;
    }

    //qDebug() << file()->fileName() << "got new semantic info";

    m_semanticInfo = semanticInfo;
    Document::Ptr doc = semanticInfo.document;

    // create the ranges
    CreateRanges createRanges;
    m_semanticInfo.ranges = createRanges(document(), doc);

    // Refresh the ids
    FindIdDeclarations updateIds;
    m_semanticInfo.idLocations = updateIds(doc);

    if (m_contextPane) {
        Node *newNode = m_semanticInfo.declaringMemberNoProperties(position());
        if (newNode) {
            m_contextPane->apply(editor(), semanticInfo.document, 0, newNode, true);
            m_cursorPositionTimer->start(); //update text marker
        }
    }

    // update outline
    m_updateOutlineTimer->start();

    // update warning/error extra selections
    QList<QTextEdit::ExtraSelection> selections;
    appendExtraSelectionsForMessages(&selections, doc->diagnosticMessages(), document());
    appendExtraSelectionsForMessages(&selections, m_semanticInfo.semanticMessages, document());
    appendExtraSelectionsForMessages(&selections, m_semanticInfo.staticAnalysisMessages, document());
    setExtraSelections(CodeWarningsSelection, selections);

    if (Core::EditorManager::currentEditor() == editor())
        m_semanticHighlighter->rerun(m_semanticInfo.scopeChain());

    emit semanticInfoUpdated();
}

void QmlJSTextEditorWidget::onRefactorMarkerClicked(const TextEditor::RefactorMarker &marker)
{
    if (marker.data.canConvert<QtQuickToolbarMarker>())
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
    if (b)
        m_contextPane->apply(editor(), semanticInfo().document, 0, 0, false);
    return b;
}

QVector<TextEditor::TextStyle> QmlJSTextEditorWidget::highlighterFormatCategories()
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
    static QVector<TextEditor::TextStyle> categories;
    if (categories.isEmpty()) {
        categories << TextEditor::C_NUMBER
                << TextEditor::C_STRING
                << TextEditor::C_TYPE
                << TextEditor::C_KEYWORD
                << TextEditor::C_FIELD
                << TextEditor::C_COMMENT
                << TextEditor::C_VISUAL_WHITESPACE;
    }
    return categories;
}

TextEditor::IAssistInterface *QmlJSTextEditorWidget::createAssistInterface(
    TextEditor::AssistKind assistKind,
    TextEditor::AssistReason reason) const
{
    if (assistKind == TextEditor::Completion) {
        return new QmlJSCompletionAssistInterface(document(),
                                                  position(),
                                                  editor()->document(),
                                                  reason,
                                                  m_semanticInfo);
    } else if (assistKind == TextEditor::QuickFix) {
        return new QmlJSQuickFixAssistInterface(const_cast<QmlJSTextEditorWidget *>(this), reason);
    }
    return 0;
}

QString QmlJSTextEditorWidget::foldReplacementText(const QTextBlock &block) const
{
    const int curlyIndex = block.text().indexOf(QLatin1Char('{'));

    if (curlyIndex != -1 && m_semanticInfo.isValid()) {
        const int pos = block.position() + curlyIndex;
        Node *node = m_semanticInfo.rangeAt(pos);

        const QString objectId = idOfObject(node);
        if (!objectId.isEmpty())
            return QLatin1String("id: ") + objectId + QLatin1String("...");
    }

    return TextEditor::BaseTextEditorWidget::foldReplacementText(block);
}
