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

#include "qmljssemantichighlighter.h"

#include "qmljseditordocument.h"

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsscopechain.h>
#include <qmljs/qmljsscopebuilder.h>
#include <qmljs/qmljsevaluate.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljsutils.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsstaticanalysismessage.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QTextDocument>
#include <QThreadPool>

using namespace QmlJS;
using namespace QmlJS::AST;

namespace QmlJSEditor {

using namespace Internal;

namespace {

static bool isIdScope(const ObjectValue *scope, const QList<const QmlComponentChain *> &chain)
{
    foreach (const QmlComponentChain *c, chain) {
        if (c->idScope() == scope)
            return true;
        if (isIdScope(scope, c->instantiatingComponents()))
            return true;
    }
    return false;
}

class CollectStateNames : protected Visitor
{
    QStringList m_stateNames;
    bool m_inStateType;
    ScopeChain m_scopeChain;
    const CppComponentValue *m_statePrototype;

public:
    CollectStateNames(const ScopeChain &scopeChain)
        : m_scopeChain(scopeChain)
    {
        m_statePrototype = scopeChain.context()->valueOwner()->cppQmlTypes().objectByCppName(QLatin1String("QDeclarativeState"));
    }

    QStringList operator()(Node *ast)
    {
        m_stateNames.clear();
        if (!m_statePrototype)
            return m_stateNames;
        m_inStateType = false;
        accept(ast);
        return m_stateNames;
    }

protected:
    void accept(Node *ast)
    {
        if (ast)
            ast->accept(this);
    }

    bool preVisit(Node *ast)
    {
        return ast->uiObjectMemberCast()
                || cast<UiProgram *>(ast)
                || cast<UiObjectInitializer *>(ast)
                || cast<UiObjectMemberList *>(ast)
                || cast<UiArrayMemberList *>(ast);
    }

    bool hasStatePrototype(Node *ast)
    {
        Bind *bind = m_scopeChain.document()->bind();
        const ObjectValue *v = bind->findQmlObject(ast);
        if (!v)
            return false;
        PrototypeIterator it(v, m_scopeChain.context());
        while (it.hasNext()) {
            const ObjectValue *proto = it.next();
            const CppComponentValue *qmlProto = value_cast<CppComponentValue>(proto);
            if (!qmlProto)
                continue;
            if (qmlProto->metaObject() == m_statePrototype->metaObject())
                return true;
        }
        return false;
    }

    bool visit(UiObjectDefinition *ast)
    {
        const bool old = m_inStateType;
        m_inStateType = hasStatePrototype(ast);
        accept(ast->initializer);
        m_inStateType = old;
        return false;
    }

    bool visit(UiObjectBinding *ast)
    {
        const bool old = m_inStateType;
        m_inStateType = hasStatePrototype(ast);
        accept(ast->initializer);
        m_inStateType = old;
        return false;
    }

    bool visit(UiScriptBinding *ast)
    {
        if (!m_inStateType)
            return false;
        if (!ast->qualifiedId || ast->qualifiedId->name.isEmpty() || ast->qualifiedId->next)
            return false;
        if (ast->qualifiedId->name != QLatin1String("name"))
            return false;

        ExpressionStatement *expStmt = cast<ExpressionStatement *>(ast->statement);
        if (!expStmt)
            return false;
        StringLiteral *strLit = cast<StringLiteral *>(expStmt->expression);
        if (!strLit || strLit->value.isEmpty())
            return false;

        m_stateNames += strLit->value.toString();

        return false;
    }
};

class CollectionTask : protected Visitor
{
public:
    CollectionTask(QFutureInterface<SemanticHighlighter::Use> futureInterface,
                   const QmlJSTools::SemanticInfo &semanticInfo)
        : m_futureInterface(futureInterface)
        , m_semanticInfo(semanticInfo)
        , m_scopeChain(semanticInfo.scopeChain())
        , m_scopeBuilder(&m_scopeChain)
        , m_lineOfLastUse(0)
        , m_nextExtraFormat(SemanticHighlighter::Max)
        , m_currentDelayedUse(0)
    {
        int nMessages = 0;
        if (m_scopeChain.document()->language().isFullySupportedLanguage()) {
            nMessages = m_scopeChain.document()->diagnosticMessages().size()
                    + m_semanticInfo.semanticMessages.size()
                    + m_semanticInfo.staticAnalysisMessages.size();
            m_delayedUses.reserve(nMessages);
            m_diagnosticRanges.reserve(nMessages);
            m_extraFormats.reserve(nMessages);
            addMessages(m_scopeChain.document()->diagnosticMessages(), m_scopeChain.document());
            addMessages(m_semanticInfo.semanticMessages, m_semanticInfo.document);
            addMessages(m_semanticInfo.staticAnalysisMessages, m_semanticInfo.document);

            Utils::sort(m_delayedUses, sortByLinePredicate);
        }
        m_currentDelayedUse = 0;
    }

    QVector<QTextLayout::FormatRange> diagnosticRanges()
    {
        return m_diagnosticRanges;
    }

    QHash<int, QTextCharFormat> extraFormats()
    {
        return m_extraFormats;
    }

    void run()
    {
        Node *root = m_scopeChain.document()->ast();
        m_stateNames = CollectStateNames(m_scopeChain)(root);
        accept(root);
        while (m_currentDelayedUse < m_delayedUses.size())
            m_uses.append(m_delayedUses.value(m_currentDelayedUse++));
        flush();
    }

protected:
    void accept(Node *ast)
    {
        if (ast)
            ast->accept(this);
    }

    void scopedAccept(Node *ast, Node *child)
    {
        m_scopeBuilder.push(ast);
        accept(child);
        m_scopeBuilder.pop();
    }

    void processName(const QStringRef &name, SourceLocation location)
    {
        if (name.isEmpty())
            return;

        const QString &nameStr = name.toString();
        const ObjectValue *scope = 0;
        const Value *value = m_scopeChain.lookup(nameStr, &scope);
        if (!value || !scope)
            return;

        SemanticHighlighter::UseType type = SemanticHighlighter::UnknownType;
        if (m_scopeChain.qmlTypes() == scope) {
            type = SemanticHighlighter::QmlTypeType;
        } else if (m_scopeChain.qmlScopeObjects().contains(scope)) {
            type = SemanticHighlighter::ScopeObjectPropertyType;
        } else if (m_scopeChain.jsScopes().contains(scope)) {
            type = SemanticHighlighter::JsScopeType;
        } else if (m_scopeChain.jsImports() == scope) {
            type = SemanticHighlighter::JsImportType;
        } else if (m_scopeChain.globalScope() == scope) {
            type = SemanticHighlighter::JsGlobalType;
        } else if (QSharedPointer<const QmlComponentChain> chain = m_scopeChain.qmlComponentChain()) {
            if (scope == chain->idScope()) {
                type = SemanticHighlighter::LocalIdType;
            } else if (isIdScope(scope, chain->instantiatingComponents())) {
                type = SemanticHighlighter::ExternalIdType;
            } else if (scope == chain->rootObjectScope()) {
                type = SemanticHighlighter::RootObjectPropertyType;
            } else  { // check for this?
                type = SemanticHighlighter::ExternalObjectPropertyType;
            }
        }

        if (type != SemanticHighlighter::UnknownType)
            addUse(location, type);
    }

    void processTypeId(UiQualifiedId *typeId)
    {
        if (!typeId)
            return;
        if (m_scopeChain.context()->lookupType(m_scopeChain.document().data(), typeId))
            addUse(fullLocationForQualifiedId(typeId), SemanticHighlighter::QmlTypeType);
    }

    void processBindingName(UiQualifiedId *localId)
    {
        if (!localId)
            return;
        addUse(fullLocationForQualifiedId(localId), SemanticHighlighter::BindingNameType);
    }

    bool visit(UiImport *ast)
    {
        processName(ast->importId, ast->importIdToken);
        return true;
    }

    bool visit(UiObjectDefinition *ast)
    {
        if (m_scopeChain.document()->bind()->isGroupedPropertyBinding(ast))
            processBindingName(ast->qualifiedTypeNameId);
        else
            processTypeId(ast->qualifiedTypeNameId);
        scopedAccept(ast, ast->initializer);
        return false;
    }

    bool visit(UiObjectBinding *ast)
    {
        processTypeId(ast->qualifiedTypeNameId);
        processBindingName(ast->qualifiedId);
        scopedAccept(ast, ast->initializer);
        return false;
    }

    bool visit(UiScriptBinding *ast)
    {
        processBindingName(ast->qualifiedId);
        scopedAccept(ast, ast->statement);
        return false;
    }

    bool visit(UiArrayBinding *ast)
    {
        processBindingName(ast->qualifiedId);
        return true;
    }

    bool visit(UiPublicMember *ast)
    {
        if (ast->typeToken.isValid() && ast->isValid()) {
            if (m_scopeChain.context()->lookupType(m_scopeChain.document().data(), QStringList(ast->memberTypeName().toString())))
                addUse(ast->typeToken, SemanticHighlighter::QmlTypeType);
        }
        if (ast->identifierToken.isValid())
            addUse(ast->identifierToken, SemanticHighlighter::BindingNameType);
        if (ast->statement)
            scopedAccept(ast, ast->statement);
        if (ast->binding)
            // this is not strictly correct for Components, as their context depends from where they
            // are instantiated, but normally not too bad as approximation
            scopedAccept(ast, ast->binding);
        return false;
    }

    bool visit(FunctionExpression *ast)
    {
        processName(ast->name, ast->identifierToken);
        scopedAccept(ast, ast->body);
        return false;
    }

    bool visit(FunctionDeclaration *ast)
    {
        return visit(static_cast<FunctionExpression *>(ast));
    }

    bool visit(VariableDeclaration *ast)
    {
        processName(ast->name, ast->identifierToken);
        return true;
    }

    bool visit(IdentifierExpression *ast)
    {
        processName(ast->name, ast->identifierToken);
        return false;
    }

    bool visit(StringLiteral *ast)
    {
        if (ast->value.isEmpty())
            return false;

        const QString &value = ast->value.toString();
        if (m_stateNames.contains(value))
            addUse(ast->literalToken, SemanticHighlighter::LocalStateNameType);

        return false;
    }

    void addMessages(QList<DiagnosticMessage> messages,
            const Document::Ptr &doc)
    {
        foreach (const DiagnosticMessage &d, messages) {
            int line = d.loc.startLine;
            int column = qMax(1U, d.loc.startColumn);
            int length = d.loc.length;
            int begin = d.loc.begin();

            if (d.loc.length == 0) {
                QString source(doc->source());
                int end = begin;
                if (begin == source.size() || source.at(begin) == QLatin1Char('\n')
                        || source.at(begin) == QLatin1Char('\r')) {
                    while (begin > end - column && !source.at(--begin).isSpace()) { }
                } else {
                    while (end < source.size() && source.at(++end).isLetterOrNumber()) { }
                }
                column += begin - d.loc.begin();
                length = end-begin;
            }

            const TextEditor::FontSettings &fontSettings = TextEditor::TextEditorSettings::instance()->fontSettings();

            QTextCharFormat format;
            if (d.isWarning())
                format = fontSettings.toTextCharFormat(TextEditor::C_WARNING);
            else
                format = fontSettings.toTextCharFormat(TextEditor::C_ERROR);

            format.setToolTip(d.message);

            collectRanges(begin, length, format);
            addDelayedUse(SemanticHighlighter::Use(line, column, length, addFormat(format)));
        }
    }

    void addMessages(const QList<StaticAnalysis::Message> &messages,
                     const Document::Ptr &doc)
    {
        foreach (const StaticAnalysis::Message &d, messages) {
            int line = d.location.startLine;
            int column = qMax(1U, d.location.startColumn);
            int length = d.location.length;
            int begin = d.location.begin();

            if (d.location.length == 0) {
                QString source(doc->source());
                int end = begin;
                if (begin == source.size() || source.at(begin) == QLatin1Char('\n')
                        || source.at(begin) == QLatin1Char('\r')) {
                    while (begin > end - column && !source.at(--begin).isSpace()) { }
                } else {
                    while (end < source.size() && source.at(++end).isLetterOrNumber()) { }
                }
                column += begin - d.location.begin();
                length = end-begin;
            }

            const TextEditor::FontSettings &fontSettings = TextEditor::TextEditorSettings::instance()->fontSettings();

            QTextCharFormat format;
            if (d.severity == Severity::Warning
                    || d.severity == Severity::MaybeWarning
                    || d.severity == Severity::ReadingTypeInfoWarning) {
                format = fontSettings.toTextCharFormat(TextEditor::C_WARNING);
            } else if (d.severity == Severity::Error || d.severity == Severity::MaybeError) {
                format = fontSettings.toTextCharFormat(TextEditor::C_ERROR);
            } else if (d.severity == Severity::Hint) {
                format = fontSettings.toTextCharFormat(TextEditor::C_WARNING);
                format.setUnderlineColor(Qt::darkGreen);
            }

            format.setToolTip(d.message);

            collectRanges(begin, length, format);
            addDelayedUse(SemanticHighlighter::Use(line, column, length, addFormat(format)));
        }
    }

private:
    void addUse(const SourceLocation &location, SemanticHighlighter::UseType type)
    {
        addUse(SemanticHighlighter::Use(location.startLine, location.startColumn, location.length, type));
    }

    static const int chunkSize = 50;

    void addUse(const SemanticHighlighter::Use &use)
    {
        while (m_currentDelayedUse < m_delayedUses.size()
               && m_delayedUses.value(m_currentDelayedUse).line < use.line)
            m_uses.append(m_delayedUses.value(m_currentDelayedUse++));

        if (m_uses.size() >= chunkSize) {
            if (use.line > m_lineOfLastUse)
                flush();
        }

        m_lineOfLastUse = qMax(m_lineOfLastUse, use.line);
        m_uses.append(use);
    }

    void addDelayedUse(const SemanticHighlighter::Use &use)
    {
        m_delayedUses.append(use);
    }

    int addFormat(const QTextCharFormat &format)
    {
        int res = m_nextExtraFormat++;
        m_extraFormats.insert(res, format);
        return res;
    }

    void collectRanges(int start, int length, const QTextCharFormat &format) {
        QTextLayout::FormatRange range;
        range.start = start;
        range.length = length;
        range.format = format;
        m_diagnosticRanges.append(range);
    }

    static bool sortByLinePredicate(const SemanticHighlighter::Use &lhs, const SemanticHighlighter::Use &rhs)
    {
        return lhs.line < rhs.line;
    }

    void flush()
    {
        m_lineOfLastUse = 0;

        if (m_uses.isEmpty())
            return;

        Utils::sort(m_uses, sortByLinePredicate);
        m_futureInterface.reportResults(m_uses);
        m_uses.clear();
        m_uses.reserve(chunkSize);
    }

    QFutureInterface<SemanticHighlighter::Use> m_futureInterface;
    const QmlJSTools::SemanticInfo &m_semanticInfo;
    ScopeChain m_scopeChain;
    ScopeBuilder m_scopeBuilder;
    QStringList m_stateNames;
    QVector<SemanticHighlighter::Use> m_uses;
    unsigned m_lineOfLastUse;
    QVector<SemanticHighlighter::Use> m_delayedUses;
    int m_nextExtraFormat;
    int m_currentDelayedUse;
    QHash<int, QTextCharFormat> m_extraFormats;
    QVector<QTextLayout::FormatRange> m_diagnosticRanges;
};

} // anonymous namespace

SemanticHighlighter::SemanticHighlighter(QmlJSEditorDocument *document)
    : QObject(document)
    , m_document(document)
    , m_startRevision(0)
{
    connect(&m_watcher, &QFutureWatcherBase::resultsReadyAt,
            this, &SemanticHighlighter::applyResults);
    connect(&m_watcher, &QFutureWatcherBase::finished,
            this, &SemanticHighlighter::finished);
}

void SemanticHighlighter::rerun(const QmlJSTools::SemanticInfo &semanticInfo)
{
    m_watcher.cancel();

    m_startRevision = m_document->document()->revision();
    m_watcher.setFuture(Utils::runAsync(QThread::LowestPriority,
                                        &SemanticHighlighter::run, this, semanticInfo));
}

void SemanticHighlighter::cancel()
{
    m_watcher.cancel();
}

void SemanticHighlighter::applyResults(int from, int to)
{
    if (m_watcher.isCanceled())
        return;
    if (m_startRevision != m_document->document()->revision())
        return;

    TextEditor::SemanticHighlighter::incrementalApplyExtraAdditionalFormats(
                m_document->syntaxHighlighter(), m_watcher.future(), from, to, m_extraFormats);
}

void SemanticHighlighter::finished()
{
    if (m_watcher.isCanceled())
        return;
    if (m_startRevision != m_document->document()->revision())
        return;

    m_document->setDiagnosticRanges(m_diagnosticRanges);

    TextEditor::SemanticHighlighter::clearExtraAdditionalFormatsUntilEnd(
                m_document->syntaxHighlighter(), m_watcher.future());
}

void SemanticHighlighter::run(QFutureInterface<SemanticHighlighter::Use> &futureInterface, const QmlJSTools::SemanticInfo &semanticInfo)
{
    CollectionTask task(futureInterface, semanticInfo);
    reportMessagesInfo(task.diagnosticRanges(), task.extraFormats());
    task.run();
}

void SemanticHighlighter::updateFontSettings(const TextEditor::FontSettings &fontSettings)
{
    m_formats[LocalIdType] = fontSettings.toTextCharFormat(TextEditor::C_QML_LOCAL_ID);
    m_formats[ExternalIdType] = fontSettings.toTextCharFormat(TextEditor::C_QML_EXTERNAL_ID);
    m_formats[QmlTypeType] = fontSettings.toTextCharFormat(TextEditor::C_QML_TYPE_ID);
    m_formats[RootObjectPropertyType] = fontSettings.toTextCharFormat(TextEditor::C_QML_ROOT_OBJECT_PROPERTY);
    m_formats[ScopeObjectPropertyType] = fontSettings.toTextCharFormat(TextEditor::C_QML_SCOPE_OBJECT_PROPERTY);
    m_formats[ExternalObjectPropertyType] = fontSettings.toTextCharFormat(TextEditor::C_QML_EXTERNAL_OBJECT_PROPERTY);
    m_formats[JsScopeType] = fontSettings.toTextCharFormat(TextEditor::C_JS_SCOPE_VAR);
    m_formats[JsImportType] = fontSettings.toTextCharFormat(TextEditor::C_JS_IMPORT_VAR);
    m_formats[JsGlobalType] = fontSettings.toTextCharFormat(TextEditor::C_JS_GLOBAL_VAR);
    m_formats[LocalStateNameType] = fontSettings.toTextCharFormat(TextEditor::C_QML_STATE_NAME);
    m_formats[BindingNameType] = fontSettings.toTextCharFormat(TextEditor::C_BINDING);
    m_formats[FieldType] = fontSettings.toTextCharFormat(TextEditor::C_FIELD);
}

void SemanticHighlighter::reportMessagesInfo(const QVector<QTextLayout::FormatRange> &diagnosticRanges,
                                             const QHash<int,QTextCharFormat> &formats)

{
    // tricky usage of m_extraFormats and diagnosticMessages we call this in another thread...
    // but will use them only after a signal sent by that same thread, maybe we should transfer
    // them more explicitly
    m_extraFormats = formats;
    m_extraFormats.unite(m_formats);
    m_diagnosticRanges = diagnosticRanges;
}

int SemanticHighlighter::startRevision() const
{
    return m_startRevision;
}

} // namespace QmlJSEditor
