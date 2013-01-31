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

#include "qmllivetextpreview.h"

#include "qmlinspectoradapter.h"
#include "qmlinspectoragent.h"

#include <coreplugin/infobar.h>
#include <qmldebug/basetoolsclient.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsdelta.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <utils/qtcassert.h>

using namespace QmlDebug;
using namespace QmlJS;
using namespace QmlJS::AST;

namespace Debugger {
namespace Internal {

const char INFO_OUT_OF_SYNC[] = "Debugger.Inspector.OutOfSyncWarning";

/*!
   Associates the UiObjectMember* to their QDeclarativeDebugObjectReference.
 */
class MapObjectWithDebugReference : public Visitor
{
public:
    typedef QList<int> DebugIdList;
    MapObjectWithDebugReference() : activated(0) {}
    virtual void endVisit(UiObjectDefinition *ast);
    virtual void endVisit(UiObjectBinding *ast);
    virtual bool visit(UiObjectDefinition *ast);
    virtual bool visit(UiObjectBinding *ast);

    QHash<QPair<int, int>, DebugIdList> ids;
    QString filename;
    QHash<UiObjectMember *, DebugIdList> result;
    QSet<UiObjectMember *> lookupObjects;

private:
    void process(UiObjectMember *ast);
    void process(UiObjectBinding *ast);
private:
    int activated;
};

class UpdateInspector : public Delta {
private:
    static inline QString stripQuotes(const QString &str)
    {
        if ((str.startsWith(QLatin1Char('"')) && str.endsWith(QLatin1Char('"')))
                || (str.startsWith(QLatin1Char('\''))
                    && str.endsWith(QLatin1Char('\''))))
            return str.mid(1, str.length() - 2);

        return str;
    }

    static inline QString deEscape(const QString &value)
    {
        QString result = value;

        result.replace(QLatin1String("\\\\"), QLatin1String("\\"));
        result.replace(QLatin1String("\\\""), QLatin1String("\""));
        result.replace(QLatin1String("\\\t"), QLatin1String("\t"));
        result.replace(QLatin1String("\\\r"), QLatin1String("\\\r"));
        result.replace(QLatin1String("\\\n"), QLatin1String("\n"));

        return result;
    }

    static QString cleanExpression(const QString &expression,
                                   UiScriptBinding *scriptBinding)
    {
        QString trimmedExpression = expression.trimmed();

        if (ExpressionStatement *expStatement
                = cast<ExpressionStatement*>(scriptBinding->statement)) {
            if (expStatement->semicolonToken.isValid())
                trimmedExpression.chop(1);
        }

        return deEscape(stripQuotes(trimmedExpression));
    }

    static bool isLiteralValue(ExpressionNode *expr)
    {
        if (cast<NumericLiteral*>(expr))
            return true;
        else if (cast<StringLiteral*>(expr))
            return true;
        else if (UnaryPlusExpression *plusExpr
                 = cast<UnaryPlusExpression*>(expr))
            return isLiteralValue(plusExpr->expression);
        else if (UnaryMinusExpression *minusExpr
                 = cast<UnaryMinusExpression*>(expr))
            return isLiteralValue(minusExpr->expression);
        else if (cast<TrueLiteral*>(expr))
            return true;
        else if (cast<FalseLiteral*>(expr))
            return true;
        else
            return false;
    }

    static inline bool isLiteralValue(UiScriptBinding *script)
    {
        if (!script || !script->statement)
            return false;

        ExpressionStatement *exprStmt
                = cast<ExpressionStatement *>(script->statement);
        if (exprStmt)
            return isLiteralValue(exprStmt->expression);
        else
            return false;
    }

    static QVariant castToLiteral(const QString &expression,
                                  UiScriptBinding *scriptBinding)
    {
        const QString cleanedValue = cleanExpression(expression, scriptBinding);
        QVariant castedExpression;

        ExpressionStatement *expStatement
                = cast<ExpressionStatement*>(scriptBinding->statement);

        switch (expStatement->expression->kind) {
        case Node::Kind_NumericLiteral:
        case Node::Kind_UnaryPlusExpression:
        case Node::Kind_UnaryMinusExpression:
            castedExpression = QVariant(cleanedValue).toReal();
            break;
        case Node::Kind_StringLiteral:
            castedExpression = QVariant(cleanedValue).toString();
            break;
        case Node::Kind_TrueLiteral:
        case Node::Kind_FalseLiteral:
            castedExpression = QVariant(cleanedValue).toBool();
            break;
        default:
            castedExpression = cleanedValue;
            break;
        }

        return castedExpression;
    }

protected:
    virtual void updateMethodBody(DebugId debugId,
                                  UiObjectMember *parentDefinition,
                                  UiScriptBinding *scriptBinding,
                                  const QString &methodName,
                                  const QString &methodBody)
    {
        Q_UNUSED(scriptBinding);
        Q_UNUSED(parentDefinition);
        appliedChangesToViewer = true;
        if (m_inspectorAdapter->engineClient())
            m_inspectorAdapter->engineClient()->setMethodBody(debugId,
                                                              methodName, methodBody);
    }

    virtual void updateScriptBinding(DebugId debugId,
                                     UiObjectMember *parentDefinition,
                                     UiScriptBinding *scriptBinding,
                                     const QString &propertyName,
                                     const QString &scriptCode)
    {
        if (unsyncronizableChanges
                == QmlLiveTextPreview::NoUnsyncronizableChanges) {
            if (propertyName  == QLatin1String("id")) {
                unsyncronizableElementName = propertyName;
                unsyncronizableChanges
                        = QmlLiveTextPreview::AttributeChangeWarning;
                unsyncronizableChangeLine
                        = parentDefinition->firstSourceLocation().startLine;
                unsyncronizableChangeColumn
                        = parentDefinition->firstSourceLocation().startColumn;
            }
        }

        QVariant expr = scriptCode;
        const bool isLiteral = isLiteralValue(scriptBinding);
        if (isLiteral)
            expr = castToLiteral(scriptCode, scriptBinding);
        appliedChangesToViewer = true;
        if (m_inspectorAdapter->engineClient())
            m_inspectorAdapter->engineClient()->setBindingForObject(
                        debugId, propertyName, expr,
                        isLiteral, document()->fileName(),
                        scriptBinding->firstSourceLocation().startLine);
    }

    virtual void resetBindingForObject(int debugId, const QString &propertyName)
    {
        appliedChangesToViewer = true;
        if (m_inspectorAdapter->engineClient())
            m_inspectorAdapter->engineClient()->resetBindingForObject(debugId, propertyName);
    }

    virtual void removeObject(int debugId)
    {
        appliedChangesToViewer = true;
        if (m_inspectorAdapter->toolsClient())
            m_inspectorAdapter->toolsClient()->destroyQmlObject(debugId);
    }

    virtual void createObject(const QString &qmlText, DebugId ref,
                              const QStringList &importList,
                              const QString &filename,
                              int order)
    {
        appliedChangesToViewer = true;
        referenceRefreshRequired = true;
        if (m_inspectorAdapter->toolsClient())
            m_inspectorAdapter->toolsClient()->createQmlObject(qmlText, ref, importList, filename, order);
    }

    virtual void reparentObject(int debugId, int newParent)
    {
        appliedChangesToViewer = true;
        if (m_inspectorAdapter->toolsClient())
            m_inspectorAdapter->toolsClient()->reparentQmlObject(debugId, newParent);
    }

    void notifyUnsyncronizableElementChange(UiObjectMember *parent)
    {
        if (unsyncronizableChanges == QmlLiveTextPreview::NoUnsyncronizableChanges) {
            UiObjectDefinition *parentDefinition = cast<UiObjectDefinition *>(parent);
            if (parentDefinition && parentDefinition->qualifiedTypeNameId
                    && !parentDefinition->qualifiedTypeNameId->name.isEmpty())
            {
                unsyncronizableElementName
                        = parentDefinition->qualifiedTypeNameId->name.toString();
                unsyncronizableChanges
                        = QmlLiveTextPreview::ElementChangeWarning;
                unsyncronizableChangeLine
                        = parentDefinition->firstSourceLocation().startLine;
                unsyncronizableChangeColumn
                        = parentDefinition->firstSourceLocation().startColumn;
            }
        }
    }

public:
    UpdateInspector(QmlInspectorAdapter *inspectorAdapter)
        : appliedChangesToViewer(false)
        , referenceRefreshRequired(false)
        , unsyncronizableChanges(QmlLiveTextPreview::NoUnsyncronizableChanges)
        , m_inspectorAdapter(inspectorAdapter)
    {
    }

    bool appliedChangesToViewer;
    bool referenceRefreshRequired;
    QString unsyncronizableElementName;
    QmlLiveTextPreview::UnsyncronizableChangeType unsyncronizableChanges;
    unsigned unsyncronizableChangeLine;
    unsigned unsyncronizableChangeColumn;
    QmlInspectorAdapter *m_inspectorAdapter;

};

bool MapObjectWithDebugReference::visit(UiObjectDefinition *ast)
{
    if (lookupObjects.contains(ast))
        activated++;
    return true;
}

bool MapObjectWithDebugReference::visit(UiObjectBinding *ast)
{
    if (lookupObjects.contains(ast))
        activated++;
    return true;
}

void MapObjectWithDebugReference::endVisit(UiObjectDefinition *ast)
{
    process(ast);
    if (lookupObjects.contains(ast))
        activated--;
}

void MapObjectWithDebugReference::endVisit(UiObjectBinding *ast)
{
    process(ast);
    if (lookupObjects.contains(ast))
        activated--;
}

void MapObjectWithDebugReference::process(UiObjectMember *ast)
{
    if (lookupObjects.isEmpty() || activated) {
        SourceLocation loc = ast->firstSourceLocation();
        QHash<QPair<int, int>, DebugIdList>::const_iterator it
                = ids.constFind(qMakePair<int, int>(loc.startLine, loc.startColumn));
        if (it != ids.constEnd())
            result[ast].append(*it);
    }
}

void MapObjectWithDebugReference::process(UiObjectBinding *ast)
{
    if (lookupObjects.isEmpty() || activated) {
        SourceLocation loc = ast->qualifiedTypeNameId->identifierToken;
        QHash<QPair<int, int>, DebugIdList>::const_iterator it
                = ids.constFind(qMakePair<int, int>(loc.startLine, loc.startColumn));
        if (it != ids.constEnd())
            result[ast].append(*it);
    }
}

/*!
 * Manages a Qml/JS document for the inspector
 */
QmlLiveTextPreview::QmlLiveTextPreview(const QmlJS::Document::Ptr &doc,
                                       const QmlJS::Document::Ptr &initDoc,
                                       QmlInspectorAdapter *inspectorAdapter,
                                       QObject *parent)
    : QObject(parent)
    , m_previousDoc(doc)
    , m_initialDoc(initDoc)
    , m_applyChangesToQmlInspector(true)
    , m_inspectorAdapter(inspectorAdapter)
    , m_nodeForOffset(0)
    , m_updateNodeForOffset(false)
    , m_changesUnsynchronizable(false)
    , m_contentsChanged(false)
{
    QTC_CHECK(doc->fileName() == initDoc->fileName());

    QmlJS::ModelManagerInterface *modelManager
            = QmlJS::ModelManagerInterface::instance();
    if (modelManager) {
        connect(modelManager, SIGNAL(documentChangedOnDisk(QmlJS::Document::Ptr)),
                SLOT(documentChanged(QmlJS::Document::Ptr)));
    }
    connect(m_inspectorAdapter->agent(), SIGNAL(objectTreeUpdated()),
            SLOT(updateDebugIds()));
    connect(this,
            SIGNAL(fetchObjectsForLocation(QString,int,int)),
            m_inspectorAdapter->agent(),
            SLOT(fetchContextObjectsForLocation(QString,int,int)));
    connect(m_inspectorAdapter->agent(), SIGNAL(automaticUpdateFailed()),
            SLOT(onAutomaticUpdateFailed()));
}

QmlLiveTextPreview::~QmlLiveTextPreview()
{
    removeOutofSyncInfo();
}

void QmlLiveTextPreview::associateEditor(Core::IEditor *editor)
{
    using namespace TextEditor;
    if (editor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID) {
        QTC_ASSERT(QLatin1String(editor->widget()->metaObject()->className()) ==
                   QLatin1String("QmlJSEditor::QmlJSTextEditorWidget"),
                   return);

        BaseTextEditorWidget *editWidget
                = qobject_cast<BaseTextEditorWidget*>(editor->widget());
        QTC_ASSERT(editWidget, return);

        if (!m_editors.contains(editWidget)) {
            m_editors << editWidget;
            if (m_inspectorAdapter) {
                connect(editWidget, SIGNAL(changed()), SLOT(editorContentsChanged()));
                connect(editWidget,
                        SIGNAL(selectedElementsChanged(QList<QmlJS::AST::UiObjectMember*>,QString)),
                        SLOT(changeSelectedElements(QList<QmlJS::AST::UiObjectMember*>,QString)));
            }
        }
    }
}

void QmlLiveTextPreview::unassociateEditor(Core::IEditor *oldEditor)
{
    using namespace TextEditor;
    if (oldEditor && oldEditor->id()
            == QmlJSEditor::Constants::C_QMLJSEDITOR_ID) {
        BaseTextEditorWidget *editWidget
                = qobject_cast<BaseTextEditorWidget*>(oldEditor->widget());
        QTC_ASSERT(editWidget, return);

        if (m_editors.contains(editWidget)) {
            m_editors.removeOne(editWidget);
            disconnect(editWidget, 0, this, 0);
        }
    }
}

void QmlLiveTextPreview::resetInitialDoc(const QmlJS::Document::Ptr &doc)
{
    m_initialDoc = doc;
    m_previousDoc = doc;
    m_createdObjects.clear();
    m_debugIds.clear();
    m_docWithUnappliedChanges.clear();
    m_changesUnsynchronizable = false;
    removeOutofSyncInfo();
}

const QString QmlLiveTextPreview::fileName()
{
    return m_previousDoc->fileName();
}

void QmlLiveTextPreview::setApplyChangesToQmlInspector(bool applyChanges)
{
    if (applyChanges && !m_applyChangesToQmlInspector) {
        if (m_docWithUnappliedChanges) {
            m_applyChangesToQmlInspector = true;
            documentChanged(m_docWithUnappliedChanges);
        }
    }

    m_applyChangesToQmlInspector = applyChanges;
}

void QmlLiveTextPreview::updateDebugIds()
{
    if (!m_initialDoc->qmlProgram())
        return;

    DebugIdHash::const_iterator it
            = m_inspectorAdapter->agent()->debugIdHash().constFind(
                qMakePair<QString, int>(m_initialDoc->fileName(), 0));
    if (it != m_inspectorAdapter->agent()->debugIdHash().constEnd()) {
        // Map all the object that comes from the document as it has been loaded
        // by the server.
        const QmlJS::Document::Ptr &doc = m_initialDoc;

        MapObjectWithDebugReference visitor;
        visitor.ids = (*it);
        visitor.filename = doc->fileName();
        doc->qmlProgram()->accept(&visitor);

        m_debugIds = visitor.result;
        if (doc != m_previousDoc) {
            Delta delta;
            m_debugIds = delta(doc, m_previousDoc, m_debugIds);
        }
    }

    const QmlJS::Document::Ptr &doc = m_previousDoc;
    if (!doc->qmlProgram())
        return;

    // Map the root nodes of the document.
    if (doc->qmlProgram()->members &&  doc->qmlProgram()->members->member) {
        UiObjectMember *root = doc->qmlProgram()->members->member;
        QHashIterator<int,QString> rIds(m_inspectorAdapter->agent()->rootObjectIds());
        QList<int> r;
        while (rIds.hasNext()) {
            rIds.next();
            if (rIds.value() == doc->componentName())
                r += rIds.key();
            }
        if (!r.isEmpty())
            m_debugIds[root] += r;
    }

    // Map the node of the later created objects.
    for (QHash<Document::Ptr,QSet<UiObjectMember*> >::const_iterator it
         = m_createdObjects.constBegin();
         it != m_createdObjects.constEnd(); ++it) {

        const QmlJS::Document::Ptr &doc = it.key();

        DebugIdHash::const_iterator id_it = m_inspectorAdapter->agent()->debugIdHash().constFind(
                    qMakePair<QString, int>(doc->fileName(), doc->editorRevision()));
        if (id_it == m_inspectorAdapter->agent()->debugIdHash().constEnd())
            continue;

        MapObjectWithDebugReference visitor;
        visitor.ids = *id_it;
        visitor.filename = doc->fileName();
        visitor.lookupObjects = it.value();
        doc->qmlProgram()->accept(&visitor);

        Delta::DebugIdMap debugIds = visitor.result;
        if (doc != m_previousDoc) {
            Delta delta;
            debugIds = delta(doc, m_previousDoc, debugIds);
        }
        for (Delta::DebugIdMap::const_iterator it2 = debugIds.constBegin();
             it2 != debugIds.constEnd(); ++it2) {
            m_debugIds[it2.key()] += it2.value();
        }
    }
    if (m_updateNodeForOffset)
        changeSelectedElements(m_lastOffsets, QString());
}

void QmlLiveTextPreview::changeSelectedElements(const QList<QmlJS::AST::UiObjectMember*> offsetObjects,
                                                const QString &wordAtCursor)
{
    if (m_editors.isEmpty() || !m_previousDoc)
        return;

    QList<int> offsets;
    foreach (QmlJS::AST::UiObjectMember *member, offsetObjects)
        offsets << member->firstSourceLocation().offset;

    if (!changeSelectedElements(offsets, wordAtCursor) && m_initialDoc && offsetObjects.count()) {
        m_updateNodeForOffset = true;
        emit fetchObjectsForLocation(m_initialDoc->fileName(),
                                     offsetObjects.first()->firstSourceLocation().startLine,
                                     offsetObjects.first()->firstSourceLocation().startColumn);
    }
}

bool QmlLiveTextPreview::changeSelectedElements(const QList<int> offsets,
                                                const QString &wordAtCursor)
{
    m_updateNodeForOffset = false;
    m_lastOffsets = offsets;
    ObjectReference objectRefUnderCursor;
    objectRefUnderCursor
            = m_inspectorAdapter->agent()->objectForName(wordAtCursor);

    QList<int> selectedReferences;
    bool containsReferenceUnderCursor = false;

    foreach (int offset, offsets) {
        if (offset >= 0) {
            QList<int> list = objectReferencesForOffset(offset);

            if (!containsReferenceUnderCursor
                    && objectRefUnderCursor.isValid()) {
                foreach (int id, list) {
                    if (id == objectRefUnderCursor.debugId()) {
                        containsReferenceUnderCursor = true;
                        break;
                    }
                }
            }

            selectedReferences << list;
        }
    }

    // fallback: use ref under cursor if nothing else is found
    if (selectedReferences.isEmpty()
            && !containsReferenceUnderCursor
            && objectRefUnderCursor.isValid()) {
        selectedReferences << objectRefUnderCursor.debugId();
    }

    if (selectedReferences.isEmpty())
        return false;
    emit selectedItemsChanged(selectedReferences);
    return true;
}

void QmlLiveTextPreview::documentChanged(QmlJS::Document::Ptr doc)
{
    if (doc->fileName() != m_previousDoc->fileName())
        return;

    // Changes to be applied when changes were made from the editor.
    // m_contentsChanged ensures that the changes were made by the user in
    // the editor before starting with the comparisons.
    if (!m_contentsChanged)
        return;

    if (m_applyChangesToQmlInspector) {
        m_docWithUnappliedChanges.clear();

        if (doc && m_previousDoc && doc->fileName() == m_previousDoc->fileName()) {
            if (doc->fileName().endsWith(QLatin1String(".js"))) {
                showSyncWarning(JSChangeWarning, QString(), 0, 0);
                m_previousDoc = doc;
                return;
            }
            if (doc->qmlProgram() && m_previousDoc->qmlProgram()) {
                UpdateInspector delta(m_inspectorAdapter);
                m_debugIds = delta(m_previousDoc, doc, m_debugIds);

                if (delta.referenceRefreshRequired)
                    m_inspectorAdapter->agent()->queryEngineContext();


                if (delta.unsyncronizableChanges != NoUnsyncronizableChanges) {
                    showSyncWarning(delta.unsyncronizableChanges,
                                    delta.unsyncronizableElementName,
                                    delta.unsyncronizableChangeLine,
                                    delta.unsyncronizableChangeColumn);
                    m_previousDoc = doc;
                    return;
                }
                m_previousDoc = doc;
                if (!delta.newObjects.isEmpty())
                    m_createdObjects[doc] += delta.newObjects;
                if (m_inspectorAdapter->toolsClient())
                    m_inspectorAdapter->toolsClient()->clearComponentCache();
            }
        }
    } else {
        m_docWithUnappliedChanges = doc;
    }
    m_contentsChanged = false;
}

void QmlLiveTextPreview::editorContentsChanged()
{
    m_contentsChanged = true;
}

void QmlLiveTextPreview::onAutomaticUpdateFailed()
{
    showSyncWarning(AutomaticUpdateFailed, QString(), -1, -1);
}

QList<int> QmlLiveTextPreview::objectReferencesForOffset(quint32 offset)
{
    QList<int> result;
    QHashIterator<QmlJS::AST::UiObjectMember*, QList<int> > iter(m_debugIds);
    QmlJS::AST::UiObjectMember *possibleNode = 0;
    while (iter.hasNext()) {
        iter.next();
        QmlJS::AST::UiObjectMember *member = iter.key();
        quint32 startOffset = member->firstSourceLocation().offset;
        quint32 endOffset = member->lastSourceLocation().offset;
        if (startOffset <= offset && offset <= endOffset) {
            if (!possibleNode)
                possibleNode = member;
            if (possibleNode->firstSourceLocation().offset <= startOffset &&
                    endOffset <= possibleNode->lastSourceLocation().offset)
                possibleNode = member;
        }
    }
    if (possibleNode) {
        if (possibleNode != m_nodeForOffset) {
            //We have found a better match, set flag so that we can
            //query again to check if this is the best match for the offset
            m_updateNodeForOffset = true;
            m_nodeForOffset = possibleNode;
        }
        result = m_debugIds.value(possibleNode);
    }
    return result;
}

void QmlLiveTextPreview::showSyncWarning(
        UnsyncronizableChangeType unsyncronizableChangeType,
        const QString &elementName, unsigned line, unsigned column)
{
    QString errorMessage;
    switch (unsyncronizableChangeType) {
    case AttributeChangeWarning:
        errorMessage = tr("The %1 attribute at line %2, column %3 cannot be "
                          "changed without reloading the QML application. ")
                .arg(elementName, QString::number(line), QString::number(column));
        break;
    case ElementChangeWarning:
        errorMessage = tr("The %1 element at line %2, column %3 cannot be "
                          "changed without reloading the QML application. ")
                .arg(elementName, QString::number(line), QString::number(column));
        break;
    case JSChangeWarning:
        errorMessage = tr("The changes in JavaScript cannot be applied "
                          "without reloading the QML application. ");
        break;
    case AutomaticUpdateFailed:
        errorMessage = tr("The changes made cannot be applied without "
                          "reloading the QML application. ");
        break;
    case QmlLiveTextPreview::NoUnsyncronizableChanges:
    default:
        return;
    }

    m_changesUnsynchronizable = true;
    errorMessage.append(tr("You can continue debugging, but behavior can be unexpected."));

    // Clear infobars if present before showing the same. Otherwise multiple infobars
    // will be shown in case the user changes and saves the file multiple times.
    removeOutofSyncInfo();

    foreach (TextEditor::BaseTextEditorWidget *editor, m_editors) {
        if (editor) {
            Core::InfoBar *infoBar = editor->editorDocument()->infoBar();
            Core::InfoBarEntry info(Core::Id(INFO_OUT_OF_SYNC), errorMessage);
            BaseToolsClient *toolsClient = m_inspectorAdapter->toolsClient();
            if (toolsClient && toolsClient->supportReload())
                info.setCustomButtonInfo(tr("Reload QML"), this,
                                         SLOT(reloadQml()));
            infoBar->addInfo(info);
        }
    }
}

void QmlLiveTextPreview::reloadQml()
{
    removeOutofSyncInfo();
    emit reloadRequest();
}

void QmlLiveTextPreview::removeOutofSyncInfo()
{
    foreach (TextEditor::BaseTextEditorWidget *editor, m_editors) {
        if (editor) {
            Core::InfoBar *infoBar = editor->editorDocument()->infoBar();
            infoBar->removeInfo(Core::Id(INFO_OUT_OF_SYNC));
        }
    }
}

} // namespace Internal
} // namespace Debugger
