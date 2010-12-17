/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include <typeinfo>

#include "qmljsinspector.h"
#include "qmljsclientproxy.h"
#include "qmljslivetextpreview.h"
#include "qmljsprivateapi.h"

#include "qmljsinspectorconstants.h"
#include <qmljseditor/qmljseditorconstants.h>
#include <qmljseditor/qmljseditor.h>
#include <qmljs/qmljsdelta.h>
#include <qmljs/parser/qmljsast_p.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>

#include <debugger/debuggerconstants.h>

#include <QDebug>

using namespace QmlJS;
using namespace QmlJS::AST;

namespace QmlJSInspector {
namespace Internal {

/*!
   Associates the UiObjectMember* to their QDeclarativeDebugObjectReference.
 */
class MapObjectWithDebugReference : public Visitor
{
    public:
        typedef QList<int> DebugIdList;
        MapObjectWithDebugReference() : activated(0) {}
        virtual void endVisit(UiObjectDefinition *ast) ;
        virtual void endVisit(UiObjectBinding *ast) ;
        virtual bool visit(UiObjectDefinition *ast) ;
        virtual bool visit(UiObjectBinding *ast) ;

        QHash<QPair<int, int>, DebugIdList> ids;
        QString filename;
        QHash<UiObjectMember *, DebugIdList> result;
        QSet<UiObjectMember *> lookupObjects;

    private:
        void process(UiObjectMember *ast);
    private:
        int activated;
};

bool MapObjectWithDebugReference::visit(UiObjectDefinition* ast)
{
    if (lookupObjects.contains(ast))
        activated++;
    return true;
}

bool MapObjectWithDebugReference::visit(UiObjectBinding* ast)
{
    if (lookupObjects.contains(ast))
        activated++;
    return true;
}

void MapObjectWithDebugReference::endVisit(UiObjectDefinition* ast)
{
    process(ast);
    if (lookupObjects.contains(ast))
        activated--;
}

void MapObjectWithDebugReference::endVisit(UiObjectBinding* ast)
{
    process(ast);
    if (lookupObjects.contains(ast))
        activated--;
}

void MapObjectWithDebugReference::process(UiObjectMember* ast)
{
    if (lookupObjects.isEmpty() || activated) {
        SourceLocation loc = ast->firstSourceLocation();
        QHash<QPair<int, int>, DebugIdList>::const_iterator it = ids.constFind(qMakePair<int, int>(loc.startLine, loc.startColumn));
        if (it != ids.constEnd())
            result[ast].append(*it);
    }
}

QmlJS::ModelManagerInterface *QmlJSLiveTextPreview::modelManager()
{
    return ExtensionSystem::PluginManager::instance()->getObject<QmlJS::ModelManagerInterface>();
}

void QmlJSLiveTextPreview::associateEditor(Core::IEditor *editor)
{
    if (editor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID) {
        QmlJSEditor::QmlJSTextEditor* qmljsEditor = qobject_cast<QmlJSEditor::QmlJSTextEditor*>(editor->widget());
        if (qmljsEditor && !m_editors.contains(qmljsEditor)) {
            qmljsEditor->setUpdateSelectedElements(true);
            m_editors << qmljsEditor;
            connect(qmljsEditor,
                    SIGNAL(selectedElementsChanged(QList<int>,QString)),
                    SLOT(changeSelectedElements(QList<int>,QString)));
        }
    }
}

void QmlJSLiveTextPreview::unassociateEditor(Core::IEditor *oldEditor)
{
    if (oldEditor && oldEditor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID) {
        QmlJSEditor::QmlJSTextEditor* qmljsEditor = qobject_cast<QmlJSEditor::QmlJSTextEditor*>(oldEditor->widget());
        if (qmljsEditor && m_editors.contains(qmljsEditor)) {
            m_editors.removeOne(qmljsEditor);
            qmljsEditor->setUpdateSelectedElements(false);
            disconnect(qmljsEditor,
                       SIGNAL(selectedElementsChanged(QList<int>,QString)),
                       this,
                       SLOT(changeSelectedElements(QList<int>,QString)));
        }
    }
}

QmlJSLiveTextPreview::QmlJSLiveTextPreview(const QmlJS::Document::Ptr &doc,
                                           const QmlJS::Document::Ptr &initDoc,
                                           ClientProxy *clientProxy,
                                           QObject* parent)
    : QObject(parent)
    , m_previousDoc(doc)
    , m_initialDoc(initDoc)
    , m_applyChangesToQmlObserver(true)
    , m_clientProxy(clientProxy)
{
    Q_ASSERT(doc->fileName() == initDoc->fileName());
    m_filename = doc->fileName();

    connect(modelManager(), SIGNAL(documentChangedOnDisk(QmlJS::Document::Ptr)),
            SLOT(documentChanged(QmlJS::Document::Ptr)));

    if (m_clientProxy.data()) {
        connect(m_clientProxy.data(), SIGNAL(objectTreeUpdated()), SLOT(updateDebugIds()));
    }
}

void QmlJSLiveTextPreview::resetInitialDoc(const QmlJS::Document::Ptr &doc)
{
    m_initialDoc = doc;
    m_previousDoc = doc;
    m_createdObjects.clear();
    m_debugIds.clear();
    m_docWithUnappliedChanges.clear();
}


QList<int> QmlJSLiveTextPreview::objectReferencesForOffset(quint32 offset) const
{
    QList<int> result;
    QHashIterator<QmlJS::AST::UiObjectMember*, QList<int> > iter(m_debugIds);
    while(iter.hasNext()) {
        iter.next();
        QmlJS::AST::UiObjectMember *member = iter.key();
        if (member->firstSourceLocation().offset == offset) {
            result = iter.value();
            break;
        }
    }
    return result;
}

void QmlJSLiveTextPreview::changeSelectedElements(QList<int> offsets, const QString &wordAtCursor)
{
    if (m_editors.isEmpty() || !m_previousDoc || !m_clientProxy)
        return;

    QDeclarativeDebugObjectReference objectRefUnderCursor;
    objectRefUnderCursor = m_clientProxy.data()->objectReferenceForId(wordAtCursor);

    QList<int> selectedReferences;
    bool containsReferenceUnderCursor = false;

    foreach(int offset, offsets) {
        if (offset >= 0) {
            QList<int> list = objectReferencesForOffset(offset);

            if (!containsReferenceUnderCursor && objectRefUnderCursor.debugId() != -1) {
                foreach(int id, list) {
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
        && objectRefUnderCursor.debugId() != -1)
    {
        selectedReferences << objectRefUnderCursor.debugId();
    }

    if (!selectedReferences.isEmpty()) {
        QList<QDeclarativeDebugObjectReference> refs;
        foreach(int i, selectedReferences)
            refs << QDeclarativeDebugObjectReference(i);
        emit selectedItemsChanged(refs);
    }
}

static QList<int> findRootObjectRecursive(const QDeclarativeDebugObjectReference &object, const Document::Ptr &doc)
{
    QList<int> result;
    if (object.className() == doc->componentName())
        result += object.debugId();

    foreach (const QDeclarativeDebugObjectReference &it, object.children()) {
        result += findRootObjectRecursive(it, doc);
    }
    return result;
}

void QmlJSLiveTextPreview::updateDebugIds()
{
    if (!m_initialDoc->qmlProgram())
        return;

    ClientProxy* clientProxy = m_clientProxy.data();
    if (!clientProxy)
        return;

    DebugIdHash::const_iterator it = clientProxy->debugIdHash().constFind(qMakePair<QString, int>(m_initialDoc->fileName(), 0));
    if (it != clientProxy->debugIdHash().constEnd()) {
        // Map all the object that comes from the document as it has been loaded by the server.
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
    if(doc->qmlProgram()->members &&  doc->qmlProgram()->members->member) {
        UiObjectMember* root = doc->qmlProgram()->members->member;
        QList<int> r;
        foreach(const QDeclarativeDebugObjectReference& it, clientProxy->rootObjectReference())
            r += findRootObjectRecursive(it, doc);
        if (!r.isEmpty())
            m_debugIds[root] += r;
    }

    // Map the node of the later created objects.
    for(QHash<Document::Ptr,QSet<UiObjectMember*> >::const_iterator it = m_createdObjects.constBegin();
        it != m_createdObjects.constEnd(); ++it) {

        const QmlJS::Document::Ptr &doc = it.key();

        DebugIdHash::const_iterator id_it = clientProxy->debugIdHash().constFind(
            qMakePair<QString, int>(doc->fileName(), doc->editorRevision()));
        if (id_it == clientProxy->debugIdHash().constEnd())
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
        for(Delta::DebugIdMap::const_iterator it2 = debugIds.constBegin();
            it2 != debugIds.constEnd(); ++it2) {
            m_debugIds[it2.key()] += it2.value();
        }
    }
}


class UpdateObserver : public Delta {
private:
    static inline QString stripQuotes(const QString &str)
    {
        if ((str.startsWith(QLatin1Char('"')) && str.endsWith(QLatin1Char('"')))
                || (str.startsWith(QLatin1Char('\'')) && str.endsWith(QLatin1Char('\''))))
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

    static QString cleanExpression(const QString &expression, UiScriptBinding *scriptBinding)
    {
        QString trimmedExpression = expression.trimmed();

        if (ExpressionStatement *expStatement = cast<ExpressionStatement*>(scriptBinding->statement)) {
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
        else if (UnaryPlusExpression *plusExpr = cast<UnaryPlusExpression*>(expr))
            return isLiteralValue(plusExpr->expression);
        else if (UnaryMinusExpression *minusExpr = cast<UnaryMinusExpression*>(expr))
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

        ExpressionStatement *exprStmt = cast<ExpressionStatement *>(script->statement);
        if (exprStmt)
            return isLiteralValue(exprStmt->expression);
        else
            return false;
    }

    static QVariant castToLiteral(const QString &expression, UiScriptBinding *scriptBinding)
    {
        const QString cleanedValue = cleanExpression(expression, scriptBinding);
        QVariant castedExpression;

        ExpressionStatement *expStatement = cast<ExpressionStatement*>(scriptBinding->statement);

        switch(expStatement->expression->kind) {
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
                                  UiObjectMember *parentDefinition, UiScriptBinding* scriptBinding,
                                  const QString& methodName, const QString& methodBody)
    {
        Q_UNUSED(scriptBinding);
        Q_UNUSED(parentDefinition);
        appliedChangesToViewer = true;
        m_clientProxy->setMethodBodyForObject(debugId, methodName, methodBody);
    }

    virtual void updateScriptBinding(DebugId debugId,
                                     UiObjectMember *parentDefinition, UiScriptBinding* scriptBinding,
                                     const QString& propertyName, const QString& scriptCode)
    {
        if (unsyncronizableChanges == QmlJSLiveTextPreview::NoUnsyncronizableChanges) {
            if (propertyName  == QLatin1String("id")) {
                unsyncronizableElementName = propertyName;
                unsyncronizableChanges = QmlJSLiveTextPreview::AttributeChangeWarning;
                unsyncronizableChangeLine = parentDefinition->firstSourceLocation().startLine;
                unsyncronizableChangeColumn = parentDefinition->firstSourceLocation().startColumn;
            }
        }

        QVariant expr = scriptCode;
        const bool isLiteral = isLiteralValue(scriptBinding);
        if (isLiteral)
            expr = castToLiteral(scriptCode, scriptBinding);
        appliedChangesToViewer = true;
        m_clientProxy->setBindingForObject(debugId, propertyName, expr, isLiteral);
    }

    virtual void resetBindingForObject(int debugId, const QString &propertyName)
    {
        appliedChangesToViewer = true;
        m_clientProxy->resetBindingForObject(debugId, propertyName);
    }

    virtual void removeObject(int debugId)
    {
        appliedChangesToViewer = true;
        m_clientProxy->destroyQmlObject(debugId);
    }

    virtual void createObject(const QString& qmlText, DebugId ref,
                         const QStringList& importList, const QString& filename)
    {
        appliedChangesToViewer = true;
        referenceRefreshRequired = true;
        m_clientProxy->createQmlObject(qmlText, ref, importList, filename);
    }

    virtual void reparentObject(int debugId, int newParent)
    {
        appliedChangesToViewer = true;
        m_clientProxy->reparentQmlObject(debugId, newParent);
    }

    void notifyUnsyncronizableElementChange(UiObjectMember *parent)
    {
        if (unsyncronizableChanges == QmlJSLiveTextPreview::NoUnsyncronizableChanges) {
            UiObjectDefinition *parentDefinition = cast<UiObjectDefinition *>(parent);
            if (parentDefinition && parentDefinition->qualifiedTypeNameId
                       && parentDefinition->qualifiedTypeNameId->name)
            {
                unsyncronizableElementName = parentDefinition->qualifiedTypeNameId->name->asString();
                unsyncronizableChanges = QmlJSLiveTextPreview::ElementChangeWarning;
                unsyncronizableChangeLine = parentDefinition->firstSourceLocation().startLine;
                unsyncronizableChangeColumn = parentDefinition->firstSourceLocation().startColumn;
            }
        }
    }

public:
    UpdateObserver(ClientProxy *clientProxy)
        : appliedChangesToViewer(false)
        , referenceRefreshRequired(false)
        , unsyncronizableChanges(QmlJSLiveTextPreview::NoUnsyncronizableChanges)
        , m_clientProxy(clientProxy)
    {

    }
    bool appliedChangesToViewer;
    bool referenceRefreshRequired;
    QString unsyncronizableElementName;
    QmlJSLiveTextPreview::UnsyncronizableChangeType unsyncronizableChanges;
    unsigned unsyncronizableChangeLine;
    unsigned unsyncronizableChangeColumn;
    ClientProxy *m_clientProxy;

};

void QmlJSLiveTextPreview::documentChanged(QmlJS::Document::Ptr doc)
{
    if (doc->fileName() != m_previousDoc->fileName() || m_clientProxy.isNull())
        return;

    bool experimentalWarningShown = false;

    if (m_applyChangesToQmlObserver) {
        m_docWithUnappliedChanges.clear();

        if (doc && m_previousDoc && doc->fileName() == m_previousDoc->fileName()
            && doc->qmlProgram() && m_previousDoc->qmlProgram())
        {
            UpdateObserver delta(m_clientProxy.data());
            m_debugIds = delta(m_previousDoc, doc, m_debugIds);

            if (delta.referenceRefreshRequired)
                m_clientProxy.data()->refreshObjectTree();

            if (InspectorUi::instance()->showExperimentalWarning() && delta.appliedChangesToViewer) {
                showExperimentalWarning();
                experimentalWarningShown = true;
                InspectorUi::instance()->setShowExperimentalWarning(false);
            }

            if (delta.unsyncronizableChanges != NoUnsyncronizableChanges && !experimentalWarningShown)
                showSyncWarning(delta.unsyncronizableChanges, delta.unsyncronizableElementName,
                                delta.unsyncronizableChangeLine, delta.unsyncronizableChangeColumn);

            m_previousDoc = doc;
            if (!delta.newObjects.isEmpty())
                m_createdObjects[doc] += delta.newObjects;

            m_clientProxy.data()->clearComponentCache();
        }
    } else {
        m_docWithUnappliedChanges = doc;
    }
}

void QmlJSLiveTextPreview::showExperimentalWarning()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    em->showEditorInfoBar(Constants::INFO_EXPERIMENTAL,
                          tr("You changed a QML file in Live Preview mode, which modifies the running QML application. "
                             "In case of unexpected behavior, please reload the QML application. "
                             ),
                          tr("Disable Live Preview"), this, SLOT(disableLivePreview()));
}

void QmlJSLiveTextPreview::showSyncWarning(UnsyncronizableChangeType unsyncronizableChangeType,
                                           const QString &elementName, unsigned line, unsigned column)
{
    Core::EditorManager *em = Core::EditorManager::instance();

    QString errorMessage;
    switch (unsyncronizableChangeType) {
        case AttributeChangeWarning:
           errorMessage = tr("The %1 attribute at line %2, column %3 cannot be changed without reloading the QML application. ")
                          .arg(elementName, QString::number(line), QString::number(column));
            break;
        case ElementChangeWarning:
            errorMessage = tr("The %1 element at line %2, column %3 cannot be changed without reloading the QML application. ")
                           .arg(elementName, QString::number(line), QString::number(column));
            break;
        case QmlJSLiveTextPreview::NoUnsyncronizableChanges:
        default:
            return;
    }

    errorMessage.append(tr("You can continue debugging, but behavior can be unexpected."));

    em->showEditorInfoBar(Constants::INFO_OUT_OF_SYNC, errorMessage);
}

void QmlJSLiveTextPreview::reloadQmlViewer()
{
    Core::EditorManager::instance()->hideEditorInfoBar(Constants::INFO_OUT_OF_SYNC);
    emit reloadQmlViewerRequested();
}

void QmlJSLiveTextPreview::disableLivePreview()
{
    Core::EditorManager::instance()->hideEditorInfoBar(Constants::INFO_EXPERIMENTAL);
    emit disableLivePreviewRequested();
}

void QmlJSLiveTextPreview::setApplyChangesToQmlObserver(bool applyChanges)
{
    if (applyChanges && !m_applyChangesToQmlObserver) {
        if (m_docWithUnappliedChanges) {
            m_applyChangesToQmlObserver = true;
            documentChanged(m_docWithUnappliedChanges);
        }
    }

    m_applyChangesToQmlObserver = applyChanges;
}

void QmlJSLiveTextPreview::setClientProxy(ClientProxy *clientProxy)
{
    if (m_clientProxy.data()) {
        disconnect(m_clientProxy.data(), SIGNAL(objectTreeUpdated()),
                   this, SLOT(updateDebugIds()));
    }

    m_clientProxy = clientProxy;

    if (m_clientProxy.data()) {
        connect(m_clientProxy.data(), SIGNAL(objectTreeUpdated()),
                   SLOT(updateDebugIds()));

        foreach(QWeakPointer<QmlJSEditor::QmlJSTextEditor> qmlEditor, m_editors) {
            if (qmlEditor)
                qmlEditor.data()->setUpdateSelectedElements(true);
        }
    } else {
        foreach(QWeakPointer<QmlJSEditor::QmlJSTextEditor> qmlEditor, m_editors) {
            if (qmlEditor)
                qmlEditor.data()->setUpdateSelectedElements(false);
        }
    }
}

} // namespace Internal
} // namespace QmlJSInspector
