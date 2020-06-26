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

#include "qmljseditordocument.h"

#include "qmljseditorconstants.h"
#include "qmljseditordocument_p.h"
#include "qmljseditorplugin.h"
#include "qmljshighlighter.h"
#include "qmljsquickfixassist.h"
#include "qmljssemantichighlighter.h"
#include "qmljssemanticinfoupdater.h"
#include "qmljstextmark.h"
#include "qmloutlinemodel.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>

#include <qmljstools/qmljsindenter.h>
#include <qmljstools/qmljsmodelmanager.h>
#include <qmljstools/qmljsqtstylecodeformatter.h>

#include <utils/infobar.h>

#include <QDebug>

const char QML_UI_FILE_WARNING[] = "QmlJSEditor.QmlUiFileWarning";

using namespace QmlJSEditor;
using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJSTools;

namespace {

enum {
    UPDATE_DOCUMENT_DEFAULT_INTERVAL = 100,
    UPDATE_OUTLINE_INTERVAL = 500
};

struct Declaration
{
    QString text;
    int startLine = 0;
    int startColumn = 0;
    int endLine = 0;
    int endColumn = 0;
};

class FindIdDeclarations: protected Visitor
{
public:
    using Result = QHash<QString, QList<SourceLocation> >;

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

    bool visit(AST::UiScriptBinding *node) override
    {
        if (asString(node->qualifiedId) == QLatin1String("id")) {
            if (auto stmt = AST::cast<const AST::ExpressionStatement*>(node->statement)) {
                if (auto idExpr = AST::cast<const AST::IdentifierExpression *>(stmt->expression)) {
                    if (!idExpr->name.isEmpty()) {
                        const QString &id = idExpr->name.toString();
                        QList<SourceLocation> *locs = &_ids[id];
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

    bool visit(AST::IdentifierExpression *node) override
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

    void throwRecursionDepthError() override
    {
        qWarning("Warning: Hit maximum recursion depth while visiting AST in FindIdDeclarations");
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

    bool visit(AST::UiObjectDefinition *node) override
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

    void endVisit(AST::UiObjectDefinition *) override
    {
        --_depth;
    }

    bool visit(AST::UiObjectBinding *node) override
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

    void endVisit(AST::UiObjectBinding *) override
    {
        --_depth;
    }

    bool visit(AST::UiScriptBinding *) override
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

    void endVisit(AST::UiScriptBinding *) override
    {
        --_depth;
    }

    bool visit(AST::FunctionExpression *) override
    {
        return false;
    }

    bool visit(AST::FunctionDeclaration *ast) override
    {
        if (ast->name.isEmpty())
            return false;

        Declaration decl;
        init(&decl, ast);

        decl.text.fill(QLatin1Char(' '), _depth);
        decl.text += ast->name;

        decl.text += QLatin1Char('(');
        for (FormalParameterList *it = ast->formals; it; it = it->next) {
            if (!it->element->bindingIdentifier.isEmpty())
                decl.text += it->element->bindingIdentifier;

            if (it->next)
                decl.text += QLatin1String(", ");
        }

        decl.text += QLatin1Char(')');

        _declarations.append(decl);

        return false;
    }

    bool visit(AST::PatternElement *ast) override
    {
        if (!ast->isVariableDeclaration() || ast->bindingIdentifier.isEmpty())
            return false;

        Declaration decl;
        decl.text.fill(QLatin1Char(' '), _depth);
        decl.text += ast->bindingIdentifier;

        const SourceLocation first = ast->identifierToken;
        decl.startLine = first.startLine;
        decl.startColumn = first.startColumn;
        decl.endLine = first.startLine;
        decl.endColumn = first.startColumn + first.length;

        _declarations.append(decl);

        return false;
    }

    bool visit(AST::BinaryExpression *ast) override
    {
        auto field = AST::cast<const AST::FieldMemberExpression *>(ast->left);
        auto funcExpr = AST::cast<const AST::FunctionExpression *>(ast->right);

        if (field && funcExpr && funcExpr->body && (ast->op == QSOperator::Assign)) {
            Declaration decl;
            init(&decl, ast);

            decl.text.fill(QLatin1Char(' '), _depth);
            decl.text += field->name;

            decl.text += QLatin1Char('(');
            for (FormalParameterList *it = funcExpr->formals; it; it = it->next) {
                if (!it->element->bindingIdentifier.isEmpty())
                    decl.text += it->element->bindingIdentifier;

                if (it->next)
                    decl.text += QLatin1String(", ");
            }
            decl.text += QLatin1Char(')');

            _declarations.append(decl);
        }

        return true;
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
        if (doc && doc->ast() != nullptr)
            doc->ast()->accept(this);
        return _ranges;
    }

protected:
    using AST::Visitor::visit;

    bool visit(AST::UiObjectBinding *ast) override
    {
        if (ast->initializer && ast->initializer->lbraceToken.length)
            _ranges.append(createRange(ast, ast->initializer));
        return true;
    }

    bool visit(AST::UiObjectDefinition *ast) override
    {
        if (ast->initializer && ast->initializer->lbraceToken.length)
            _ranges.append(createRange(ast, ast->initializer));
        return true;
    }

    bool visit(AST::FunctionExpression *ast) override
    {
        _ranges.append(createRange(ast));
        return true;
    }

    bool visit(AST::FunctionDeclaration *ast) override
    {
        _ranges.append(createRange(ast));
        return true;
    }

    bool visit(AST::BinaryExpression *ast) override
    {
        auto field = AST::cast<AST::FieldMemberExpression *>(ast->left);
        auto funcExpr = AST::cast<AST::FunctionExpression *>(ast->right);

        if (field && funcExpr && funcExpr->body && (ast->op == QSOperator::Assign))
            _ranges.append(createRange(ast, ast->firstSourceLocation(), ast->lastSourceLocation()));
        return true;
    }

    bool visit(AST::UiScriptBinding *ast) override
    {
        if (auto block = AST::cast<AST::Block *>(ast->statement))
            _ranges.append(createRange(ast, block));
        return true;
    }

    void throwRecursionDepthError() override
    {
        qWarning("Warning: Hit maximum recursion depth while visiting AST in CreateRanges");
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

    Range createRange(AST::Node *ast, SourceLocation start, SourceLocation end)
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

}

namespace QmlJSEditor {
namespace Internal {

QmlJSEditorDocumentPrivate::QmlJSEditorDocumentPrivate(QmlJSEditorDocument *parent)
    : q(parent)
    , m_semanticHighlighter(new SemanticHighlighter(parent))
    , m_outlineModel(new QmlOutlineModel(parent))
{
    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    // code model
    m_updateDocumentTimer.setInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
    m_updateDocumentTimer.setSingleShot(true);
    connect(q->document(), &QTextDocument::contentsChanged,
            &m_updateDocumentTimer, QOverload<>::of(&QTimer::start));
    connect(&m_updateDocumentTimer, &QTimer::timeout,
            this, &QmlJSEditorDocumentPrivate::reparseDocument);
    connect(modelManager, &ModelManagerInterface::documentUpdated,
            this, &QmlJSEditorDocumentPrivate::onDocumentUpdated);

    // semantic info
    m_semanticInfoUpdater = new SemanticInfoUpdater(this);
    connect(m_semanticInfoUpdater, &SemanticInfoUpdater::updated,
            this, &QmlJSEditorDocumentPrivate::acceptNewSemanticInfo);
    m_semanticInfoUpdater->start();

    // library info changes
    m_reupdateSemanticInfoTimer.setInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
    m_reupdateSemanticInfoTimer.setSingleShot(true);
    connect(&m_reupdateSemanticInfoTimer, &QTimer::timeout,
            this, &QmlJSEditorDocumentPrivate::reupdateSemanticInfo);
    connect(modelManager, &ModelManagerInterface::libraryInfoUpdated,
            &m_reupdateSemanticInfoTimer, QOverload<>::of(&QTimer::start));

    // outline model
    m_updateOutlineModelTimer.setInterval(UPDATE_OUTLINE_INTERVAL);
    m_updateOutlineModelTimer.setSingleShot(true);
    connect(&m_updateOutlineModelTimer, &QTimer::timeout,
            this, &QmlJSEditorDocumentPrivate::updateOutlineModel);

    modelManager->updateSourceFiles(QStringList(parent->filePath().toString()), false);
}

QmlJSEditorDocumentPrivate::~QmlJSEditorDocumentPrivate()
{
    m_semanticInfoUpdater->abort();
    m_semanticInfoUpdater->wait();
    // clean up all marks, otherwise a callback could try to access deleted members.
    // see QTCREATORBUG-20199
    cleanDiagnosticMarks();
    cleanSemanticMarks();
}

void QmlJSEditorDocumentPrivate::invalidateFormatterCache()
{
    CreatorCodeFormatter formatter(q->tabSettings());
    formatter.invalidateCache(q->document());
}

void QmlJSEditorDocumentPrivate::reparseDocument()
{
    ModelManagerInterface::instance()->updateSourceFiles(QStringList(q->filePath().toString()),
                                                         false);
}

void QmlJSEditorDocumentPrivate::onDocumentUpdated(Document::Ptr doc)
{
    if (q->filePath().toString() != doc->fileName())
        return;

    // text document has changed, simply wait for the next onDocumentUpdated
    if (doc->editorRevision() != q->document()->revision())
        return;

    cleanDiagnosticMarks();
    if (doc->ast()) {
        // got a correctly parsed (or recovered) file.
        m_semanticInfoDocRevision = doc->editorRevision();
        m_semanticInfoUpdater->update(doc, ModelManagerInterface::instance()->snapshot());
    } else if (doc->language().isFullySupportedLanguage()) {
        createTextMarks(doc->diagnosticMessages());
    }
    emit q->updateCodeWarnings(doc);
}

void QmlJSEditorDocumentPrivate::reupdateSemanticInfo()
{
    // If the editor is newer than the semantic info (possibly with update in progress),
    // new semantic infos won't be accepted anyway. We'll get a onDocumentUpdated anyhow.
    if (q->document()->revision() != m_semanticInfoDocRevision)
        return;

    m_semanticInfoUpdater->reupdate(ModelManagerInterface::instance()->snapshot());
}

void QmlJSEditorDocumentPrivate::acceptNewSemanticInfo(const SemanticInfo &semanticInfo)
{
    if (semanticInfo.revision() != q->document()->revision()) {
        // ignore outdated semantic infos
        return;
    }

    m_semanticInfo = semanticInfo;
    Document::Ptr doc = semanticInfo.document;

    // create the ranges
    CreateRanges createRanges;
    m_semanticInfo.ranges = createRanges(q->document(), doc);

    // Refresh the ids
    FindIdDeclarations updateIds;
    m_semanticInfo.idLocations = updateIds(doc);

    m_outlineModelNeedsUpdate = true;
    m_semanticHighlightingNecessary = true;

    createTextMarks(m_semanticInfo);
    emit q->semanticInfoUpdated(m_semanticInfo); // calls triggerPendingUpdates as necessary
}

void QmlJSEditorDocumentPrivate::updateOutlineModel()
{
    if (q->isSemanticInfoOutdated())
        return; // outline update will be retriggered when semantic info is updated

    m_outlineModel->update(m_semanticInfo);
}

static void cleanMarks(QVector<TextEditor::TextMark *> *marks, TextEditor::TextDocument *doc)
{
    // if doc is null, this method is improperly called, so better do nothing that leave an
    // inconsistent state where marks are cleared but not removed from doc.
    if (!marks || !doc)
        return;
    for (TextEditor::TextMark *mark : *marks) {
        doc->removeMark(mark);
        delete mark;
    }
    marks->clear();
}

void QmlJSEditorDocumentPrivate::createTextMarks(const QList<DiagnosticMessage> &diagnostics)
{
    for (const DiagnosticMessage &diagnostic : diagnostics) {
        const auto onMarkRemoved = [this](QmlJSTextMark *mark) {
            m_diagnosticMarks.removeAll(mark);
            delete mark;
         };

        auto mark = new QmlJSTextMark(q->filePath(), diagnostic, onMarkRemoved);
        m_diagnosticMarks.append(mark);
        q->addMark(mark);
    }
}

void QmlJSEditorDocumentPrivate::cleanDiagnosticMarks()
{
    cleanMarks(&m_diagnosticMarks, q);
}

void QmlJSEditorDocumentPrivate::createTextMarks(const SemanticInfo &info)
{
    cleanSemanticMarks();
    const auto onMarkRemoved = [this](QmlJSTextMark *mark) {
        m_semanticMarks.removeAll(mark);
        delete mark;
    };
    for (const DiagnosticMessage &diagnostic : qAsConst(info.semanticMessages)) {
        auto mark = new QmlJSTextMark(q->filePath(),
                                      diagnostic, onMarkRemoved);
        m_semanticMarks.append(mark);
        q->addMark(mark);
    }
    for (const QmlJS::StaticAnalysis::Message &message : qAsConst(info.staticAnalysisMessages)) {
        auto mark = new QmlJSTextMark(q->filePath(),
                                      message, onMarkRemoved);
        m_semanticMarks.append(mark);
        q->addMark(mark);
    }
}

void QmlJSEditorDocumentPrivate::cleanSemanticMarks()
{
    cleanMarks(&m_semanticMarks, q);
}

} // Internal

QmlJSEditorDocument::QmlJSEditorDocument(Utils::Id id)
    : d(new Internal::QmlJSEditorDocumentPrivate(this))
{
    setId(id);
    connect(this, &TextEditor::TextDocument::tabSettingsChanged,
            d, &Internal::QmlJSEditorDocumentPrivate::invalidateFormatterCache);
    setSyntaxHighlighter(new QmlJSHighlighter(document()));
    setIndenter(new Internal::Indenter(document()));
}

QmlJSEditorDocument::~QmlJSEditorDocument()
{
    delete d;
}

const SemanticInfo &QmlJSEditorDocument::semanticInfo() const
{
    return d->m_semanticInfo;
}

bool QmlJSEditorDocument::isSemanticInfoOutdated() const
{
    return d->m_semanticInfo.revision() != document()->revision();
}

QVector<QTextLayout::FormatRange> QmlJSEditorDocument::diagnosticRanges() const
{
    return d->m_diagnosticRanges;
}

Internal::QmlOutlineModel *QmlJSEditorDocument::outlineModel() const
{
    return d->m_outlineModel;
}

TextEditor::IAssistProvider *QmlJSEditorDocument::quickFixAssistProvider() const
{
    return Internal::QmlJSEditorPlugin::quickFixAssistProvider();
}

void QmlJSEditorDocument::setIsDesignModePreferred(bool value)
{
    d->m_isDesignModePreferred = value;
    if (value) {
        if (infoBar()->canInfoBeAdded(QML_UI_FILE_WARNING)) {
            Utils::InfoBarEntry info(QML_UI_FILE_WARNING,
                                     tr("This file should only be edited in <b>Design</b> mode."));
            info.setCustomButtonInfo(tr("Switch Mode"), []() {
                Core::ModeManager::activateMode(Core::Constants::MODE_DESIGN);
            });
            infoBar()->addInfo(info);
        }
    } else if (infoBar()->containsInfo(QML_UI_FILE_WARNING)) {
        infoBar()->removeInfo(QML_UI_FILE_WARNING);
    }
}

bool QmlJSEditorDocument::isDesignModePreferred() const
{
    return d->m_isDesignModePreferred;
}

void QmlJSEditorDocument::setDiagnosticRanges(const QVector<QTextLayout::FormatRange> &ranges)
{
    d->m_diagnosticRanges = ranges;
}

void QmlJSEditorDocument::applyFontSettings()
{
    TextDocument::applyFontSettings();
    d->m_semanticHighlighter->updateFontSettings(fontSettings());
    if (!isSemanticInfoOutdated()) {
        d->m_semanticHighlightingNecessary = false;
        d->m_semanticHighlighter->rerun(d->m_semanticInfo);
    }
}

void QmlJSEditorDocument::triggerPendingUpdates()
{
    TextDocument::triggerPendingUpdates(); // calls applyFontSettings if necessary
    // might still need to rehighlight if font settings did not change
    if (d->m_semanticHighlightingNecessary && !isSemanticInfoOutdated()) {
        d->m_semanticHighlightingNecessary = false;
        d->m_semanticHighlighter->rerun(d->m_semanticInfo);
    }
    if (d->m_outlineModelNeedsUpdate && !isSemanticInfoOutdated()) {
        d->m_outlineModelNeedsUpdate = false;
        d->m_updateOutlineModelTimer.start();
    }
}

} // QmlJSEditor
