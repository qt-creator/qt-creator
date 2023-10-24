// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljseditordocument.h"

#include "qmljseditordocument_p.h"
#include "qmljseditorplugin.h"
#include "qmljseditortr.h"
#include "qmljshighlighter.h"
#include "qmljsquickfixassist.h"
#include "qmljssemantichighlighter.h"
#include "qmljssemanticinfoupdater.h"
#include "qmljstextmark.h"
#include "qmllsclient.h"
#include "qmllssettings.h"
#include "qmloutlinemodel.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>

#include <qmljstools/qmljsindenter.h>
#include <qmljstools/qmljsmodelmanager.h>
#include <qmljstools/qmljsqtstylecodeformatter.h>

#include <utils/fileutils.h>
#include <utils/infobar.h>

#include <languageclient/languageclientmanager.h>

#include <QDebug>
#include <QLoggingCategory>
#include <QTextCodec>

const char QML_UI_FILE_WARNING[] = "QmlJSEditor.QmlUiFileWarning";

using namespace QmlJSEditor;
using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJSTools;

namespace {

Q_LOGGING_CATEGORY(qmllsLog, "qtc.qmlls.editor", QtWarningMsg);

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
                text += id->name.toString();
            else
                text += QLatin1Char('?');

            if (id->next)
                text += QLatin1Char('.');
        }

        return text;
    }

    void accept(AST::Node *node) { AST::Node::accept(node, this); }

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
                text += id->name.toString();
            else
                text += QLatin1Char('?');

            if (id->next)
                text += QLatin1Char('.');
        }

        return text;
    }

    void accept(AST::Node *node) { AST::Node::accept(node, this); }

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

    bool visit(AST::TemplateLiteral *ast) override
    {
        // avoid? finds function declarations in templates
        AST::Node::accept(ast->expression, this);
        return true;
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
        decl.text += ast->name.toString();

        decl.text += QLatin1Char('(');
        for (FormalParameterList *it = ast->formals; it; it = it->next) {
            if (!it->element->bindingIdentifier.isEmpty())
                decl.text += it->element->bindingIdentifier.toString();

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
        decl.text += ast->bindingIdentifier.toString();

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
            decl.text += field->name.toString();

            decl.text += QLatin1Char('(');
            for (FormalParameterList *it = funcExpr->formals; it; it = it->next) {
                if (!it->element->bindingIdentifier.isEmpty())
                    decl.text += it->element->bindingIdentifier.toString();

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

    bool visit(AST::TemplateLiteral *ast) override
    {
        AST::Node::accept(ast->expression, this);
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
    connect(QmllsSettingsManager::instance(),
            &QmllsSettingsManager::settingsChanged,
            this,
            &Internal::QmlJSEditorDocumentPrivate::settingsChanged);

    // semantic info
    m_semanticInfoUpdater = new SemanticInfoUpdater();
    connect(m_semanticInfoUpdater, &SemanticInfoUpdater::finished,
            m_semanticInfoUpdater, &QObject::deleteLater);
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

    modelManager->updateSourceFiles(Utils::FilePaths({parent->filePath()}), false);
}

QmlJSEditorDocumentPrivate::~QmlJSEditorDocumentPrivate()
{
    m_semanticInfoUpdater->abort();
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
    ModelManagerInterface::instance()->updateSourceFiles(Utils::FilePaths({q->filePath()}), false);
}

void QmlJSEditorDocumentPrivate::onDocumentUpdated(Document::Ptr doc)
{
    if (q->filePath() != doc->fileName())
        return;

    // text document has changed, simply wait for the next onDocumentUpdated
    if (doc->editorRevision() != q->document()->revision())
        return;

    cleanDiagnosticMarks();
    if (doc->ast()) {
        // got a correctly parsed (or recovered) file.
        m_semanticInfoDocRevision = doc->editorRevision();
        m_semanticInfoUpdater->update(doc, ModelManagerInterface::instance()->snapshot());
    } else if (doc->language().isFullySupportedLanguage()
               && m_qmllsStatus.semanticWarningsSource == QmllsStatus::Source::EmbeddedCodeModel) {
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

    if (m_qmllsStatus.semanticWarningsSource == QmllsStatus::Source::EmbeddedCodeModel)
        createTextMarks(m_semanticInfo);
    emit q->semanticInfoUpdated(m_semanticInfo); // calls triggerPendingUpdates as necessary
}

void QmlJSEditorDocumentPrivate::updateOutlineModel()
{
    if (isSemanticInfoOutdated())
        return; // outline update will be retriggered when semantic info is updated

    m_outlineModel->update(m_semanticInfo);
}

bool QmlJSEditorDocumentPrivate::isSemanticInfoOutdated() const
{
    return m_semanticInfo.revision() != q->document()->revision();
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
    if (m_qmllsStatus.semanticWarningsSource != QmllsStatus::Source::EmbeddedCodeModel)
        return;
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
    for (const DiagnosticMessage &diagnostic : std::as_const(info.semanticMessages)) {
        auto mark = new QmlJSTextMark(q->filePath(),
                                      diagnostic, onMarkRemoved);
        m_semanticMarks.append(mark);
        q->addMark(mark);
    }
    for (const QmlJS::StaticAnalysis::Message &message : std::as_const(info.staticAnalysisMessages)) {
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

void QmlJSEditorDocumentPrivate::setSemanticWarningSource(QmllsStatus::Source newSource)
{
    if (m_qmllsStatus.semanticWarningsSource == newSource)
        return;
    m_qmllsStatus.semanticWarningsSource = newSource;
    QTC_ASSERT(q->thread() == QThread::currentThread(), return );
    switch (m_qmllsStatus.semanticWarningsSource) {
    case QmllsStatus::Source::Qmlls:
        m_semanticHighlighter->setEnableWarnings(false);
        cleanDiagnosticMarks();
        cleanSemanticMarks();
        if (m_semanticInfo.isValid() && !isSemanticInfoOutdated()) {
            // clean up underlines for warning messages
            m_semanticHighlightingNecessary = false;
            m_semanticHighlighter->rerun(m_semanticInfo);
        }
        break;
    case QmllsStatus::Source::EmbeddedCodeModel:
        m_semanticHighlighter->setEnableWarnings(true);
        reparseDocument();
        break;
    }
}

void QmlJSEditorDocumentPrivate::setSemanticHighlightSource(QmllsStatus::Source newSource)
{
    if (m_qmllsStatus.semanticHighlightSource == newSource)
        return;
    m_qmllsStatus.semanticHighlightSource = newSource;
    QTC_ASSERT(q->thread() == QThread::currentThread(), return );
    switch (m_qmllsStatus.semanticHighlightSource) {
    case QmllsStatus::Source::Qmlls:
        m_semanticHighlighter->setEnableHighlighting(false);
        cleanSemanticMarks();
        break;
    case QmllsStatus::Source::EmbeddedCodeModel:
        m_semanticHighlighter->setEnableHighlighting(true);
        if (m_semanticInfo.isValid() && !isSemanticInfoOutdated()) {
            m_semanticHighlightingNecessary = false;
            m_semanticHighlighter->rerun(m_semanticInfo);
        }
        break;
    }
}

void QmlJSEditorDocumentPrivate::setCompletionSource(QmllsStatus::Source newSource)
{
    if (m_qmllsStatus.completionSource == newSource)
        return;
    m_qmllsStatus.completionSource = newSource;
    switch (m_qmllsStatus.completionSource) {
    case QmllsStatus::Source::Qmlls:
        // activation of the document already takes care of setting it
        break;
    case QmllsStatus::Source::EmbeddedCodeModel:
        // deactivation of the document takes care of restoring it
        break;
    }
}

void QmlJSEditorDocumentPrivate::setSourcesWithCapabilities(
    const LanguageServerProtocol::ServerCapabilities &cap)
{
    if (cap.completionProvider())
        setCompletionSource(QmllsStatus::Source::Qmlls);
    else
        setCompletionSource(QmllsStatus::Source::EmbeddedCodeModel);
    if (cap.codeActionProvider())
        setSemanticWarningSource(QmllsStatus::Source::Qmlls);
    else
        setSemanticWarningSource(QmllsStatus::Source::EmbeddedCodeModel);
    if (cap.semanticTokensProvider())
        setSemanticHighlightSource(QmllsStatus::Source::Qmlls);
    else
        setSemanticHighlightSource(QmllsStatus::Source::EmbeddedCodeModel);
}

static Utils::FilePath qmllsForFile(const Utils::FilePath &file,
                                    QmlJS::ModelManagerInterface *modelManager)
{
    QmllsSettingsManager *settingsManager = QmllsSettingsManager::instance();
    QmllsSettings settings = settingsManager->lastSettings();
    bool enabled = settings.useQmlls;
    if (!enabled)
        return {};
    if (settings.useLatestQmlls)
        return settingsManager->latestQmlls();
    QmlJS::ModelManagerInterface::ProjectInfo pInfo = modelManager->projectInfoForPath(file);
    return pInfo.qmllsPath;
}

void QmlJSEditorDocumentPrivate::settingsChanged()
{
    Utils::FilePath newQmlls = qmllsForFile(q->filePath(), ModelManagerInterface::instance());
    if (m_qmllsStatus.qmllsPath == newQmlls)
        return;
    m_qmllsStatus.qmllsPath = newQmlls;
    auto lspClientManager = LanguageClient::LanguageClientManager::instance();
    if (newQmlls.isEmpty()) {
        qCDebug(qmllsLog) << "disabling qmlls for" << q->filePath();
        if (LanguageClient::Client *client = lspClientManager->clientForDocument(q)) {
            qCDebug(qmllsLog) << "deactivating " << q->filePath() << "in qmlls" << newQmlls;
            client->deactivateDocument(q);
        } else
            qCWarning(qmllsLog) << "Could not find client to disable for document " << q->filePath()
                                << " in LanguageClient::LanguageClientManager";
        setCompletionSource(QmllsStatus::Source::EmbeddedCodeModel);
        setSemanticWarningSource(QmllsStatus::Source::EmbeddedCodeModel);
        setSemanticHighlightSource(QmllsStatus::Source::EmbeddedCodeModel);
    } else if (QmllsClient *client = QmllsClient::clientForQmlls(newQmlls)) {
        bool shouldActivate = false;
        if (auto oldClient = lspClientManager->clientForDocument(q)) {
            // check if it was disabled
            if (client == oldClient)
                shouldActivate = true;
        }
        switch (client->state()) {
        case LanguageClient::Client::State::Uninitialized:
        case LanguageClient::Client::State::InitializeRequested:
            connect(client,
                    &LanguageClient::Client::initialized,
                    this,
                    &QmlJSEditorDocumentPrivate::setSourcesWithCapabilities);
            break;
        case LanguageClient::Client::State::Initialized:
            setSourcesWithCapabilities(client->capabilities());
            break;
        case LanguageClient::Client::State::FailedToInitialize:
        case LanguageClient::Client::State::Error:
            qCWarning(qmllsLog) << "qmlls" << newQmlls << "requested for document" << q->filePath()
                                << "had errors, skipping setSourcesWithCababilities";
            break;
        case LanguageClient::Client::State::Shutdown:
            qCWarning(qmllsLog) << "qmlls" << newQmlls << "requested for document" << q->filePath()
                                << "did stop, skipping setSourcesWithCababilities";
            break;
        case LanguageClient::Client::State::ShutdownRequested:
            qCWarning(qmllsLog) << "qmlls" << newQmlls << "requested for document" << q->filePath()
                                << "is stopping, skipping setSourcesWithCababilities";
            break;
        }
        if (shouldActivate) {
            qCDebug(qmllsLog) << "reactivating " << q->filePath() << "in qmlls" << newQmlls;
            client->activateDocument(q);
        } else {
            qCDebug(qmllsLog) << "opening " << q->filePath() << "in qmlls" << newQmlls;
            lspClientManager->openDocumentWithClient(q, client);
        }
    } else {
        qCWarning(qmllsLog) << "could not start qmlls " << newQmlls << "for" << q->filePath();
    }
}

} // Internal

QmlJSEditorDocument::QmlJSEditorDocument(Utils::Id id)
    : d(new Internal::QmlJSEditorDocumentPrivate(this))
{
    setId(id);
    connect(this, &TextEditor::TextDocument::tabSettingsChanged,
            d, &Internal::QmlJSEditorDocumentPrivate::invalidateFormatterCache);
    connect(this, &TextEditor::TextDocument::openFinishedSuccessfully,
            d, &Internal::QmlJSEditorDocumentPrivate::settingsChanged);
    setSyntaxHighlighter(new QmlJSHighlighter(document()));
    setCodec(QTextCodec::codecForName("UTF-8")); // qml files are defined to be utf-8
    setIndenter(new Internal::Indenter(document()));
}

bool QmlJSEditorDocument::supportsCodec(const QTextCodec *codec) const
{
    return codec == QTextCodec::codecForName("UTF-8");
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
    return d->isSemanticInfoOutdated();
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
    if (const auto baseProvider = TextDocument::quickFixAssistProvider())
        return baseProvider;
    return Internal::QmlJSEditorPlugin::quickFixAssistProvider();
}

void QmlJSEditorDocument::setIsDesignModePreferred(bool value)
{
    d->m_isDesignModePreferred = value;
    if (value) {
        if (infoBar()->canInfoBeAdded(QML_UI_FILE_WARNING)) {
            Utils::InfoBarEntry info(QML_UI_FILE_WARNING,
                                     Tr::tr("This file should only be edited in <b>Design</b> mode."));
            info.addCustomButton(Tr::tr("Switch Mode"), []() {
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
