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

#include "qmljsdelta.h"
#include "qmljsutils.h"
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsastvisitor_p.h>


#include <typeinfo>
#include <QDebug>
#include <QStringList>


using namespace QmlJS::AST;

namespace {
using namespace QmlJS;

enum {
    debug = false
};

/*!
    Build a hash of the parents
 */
struct BuildParentHash : public Visitor
{
    virtual void postVisit(Node* );
    virtual bool preVisit(Node* );
    QHash<UiObjectMember *, UiObjectMember *> parent;
private:
    QList<UiObjectMember *> stack;
};

bool BuildParentHash::preVisit(Node* ast)
{
    if (ast->uiObjectMemberCast())
        stack.append(ast->uiObjectMemberCast());
    return true;
}

void BuildParentHash::postVisit(Node* ast)
{
    if (ast->uiObjectMemberCast()) {
        stack.removeLast();
        if (!stack.isEmpty())
            parent.insert(ast->uiObjectMemberCast(), stack.last());
    }
}

static QString label(UiQualifiedId *id)
{
    QString str;
    for (; id ; id = id->next) {
        if (id->name.isEmpty())
            return QString();
        if (!str.isEmpty())
            str += QLatin1Char('.');
        str += id->name;
    }
    return str;
}

//return a label string for a AST node that is suitable to compare them.
static QString label(UiObjectMember *member, Document::Ptr doc)
{
    QString str;
    if (!member)
        return str;

    if (UiObjectDefinition* foo = cast<UiObjectDefinition *>(member)) {
        str = label(foo->qualifiedTypeNameId);
    } else if (UiObjectBinding *foo = cast<UiObjectBinding *>(member)) {
        str = label(foo->qualifiedId) + QLatin1Char(' ') + label(foo->qualifiedTypeNameId);
    } else if (UiArrayBinding *foo = cast<UiArrayBinding *>(member)) {
        str = label(foo->qualifiedId) + QLatin1String("[]");
    } else if (UiScriptBinding *foo = cast<UiScriptBinding *>(member)) {
        str = label(foo->qualifiedId) + QLatin1Char(':');
        if (foo->statement) {
            quint32 start = foo->statement->firstSourceLocation().begin();
            quint32 end = foo->statement->lastSourceLocation().end();
            str += doc->source().midRef(start, end-start);
        }
    } else {
        quint32 start = member->firstSourceLocation().begin();
        quint32 end = member->lastSourceLocation().end();
        str = doc->source().mid(start, end-start);
    }
    return str;
}

/* Find nodes in the tree that have the given label */
struct FindObjectMemberWithLabel : public Visitor
{
    virtual void endVisit(UiObjectDefinition *ast) ;
    virtual void endVisit(UiObjectBinding *ast) ;

    QList<UiObjectMember *> found;
    QString ref_label;
    Document::Ptr doc;
};

void FindObjectMemberWithLabel::endVisit(UiObjectDefinition* ast)
{
    if (label(ast, doc) == ref_label)
        found.append(ast);
}
void FindObjectMemberWithLabel::endVisit(UiObjectBinding* ast)
{
    if (label(ast, doc) == ref_label)
        found.append(ast);
}

/*
 bidirrection mapping between the old and the new tree nodes
*/
struct Map {
    typedef UiObjectMember *T1;
    typedef UiObjectMember *T2;
    QHash<T1, T2> way1;
    QHash<T2, T1> way2;
    void insert(T1 t1, T2 t2) {
        way1.insert(t1, t2);
        way2.insert(t2, t1);
    }
    int count() { return way1.count(); }
    void operator+=(const Map &other) {
        way1.unite(other.way1);
        way2.unite(other.way2);
    }
    bool contains(T1 t1, T2 t2) {
        return way1.value(t1) == t2;
    }
};

//return the list of children node of an ast node
static QList<UiObjectMember *> children(UiObjectMember *ast)
{
    QList<UiObjectMember *> ret;
    if (UiObjectInitializer * foo = QmlJS::initializerOfObject(ast)) {
        UiObjectMemberList* list = foo->members;
        while (list) {
            ret.append(list->member);
            list = list->next;
        }
    } else if (UiArrayBinding *foo = cast<UiArrayBinding *>(ast)) {
        UiArrayMemberList* list = foo->members;
        while (list) {
            ret.append(list->member);
            list = list->next;
        }
    }
    return ret;
}

/* build a mapping between nodes of two subtree of an ast.  x and y are the root of the two subtrees   */
static Map buildMapping_helper(UiObjectMember *x, UiObjectMember *y, const Map &M, Document::Ptr doc1, Document::Ptr doc2) {
    Map M2;
    if (M.way1.contains(x))
        return M2;
    if (M.way2.contains(y))
        return M2;
    if (label(x, doc1) != label(y, doc2))
        return M2;
    M2.insert(x, y);
    QList<UiObjectMember *> list1 = children(x);
    QList<UiObjectMember *> list2 = children(y);
    for (int i = 0; i < list1.count(); i++) {
        QString l = label(list1[i], doc1);
        int foundJ = -1;
        Map M3;
        for (int j = 0; j < list2.count(); j++) {
            if (l != label(list2[j], doc2))
                continue;
            Map M4 = buildMapping_helper(list1[i], list2[j], M, doc1, doc2);
            if (M4.count() > M3.count()) {
                M3 = M4;
                foundJ = j;
            }
        }
        if (foundJ != -1) {
            list2.removeAt(foundJ);
            M2 += M3;
        }
    }
    return M2;
}

/* given two AST, build a mapping between the corresponding nodes of the two tree
*/
static Map buildMapping(Document::Ptr doc1, Document::Ptr doc2)
{
    Map M;
    QList<UiObjectMember *> todo;
    todo.append(doc1->qmlProgram()->members->member);
    while (!todo.isEmpty()) {
        UiObjectMember *x = todo.takeFirst();
        todo += children(x);

        if (M.way1.contains(x))
            continue;

        //If this is too slow, we could use some sort of indexing
        FindObjectMemberWithLabel v3;
        v3.ref_label = label(x, doc1);
        v3.doc = doc2;
        doc2->qmlProgram()->accept(&v3);
        Map M2;
        foreach (UiObjectMember *y, v3.found) {
            if (M.way2.contains(y))
                continue;
            Map M3 = buildMapping_helper(x, y, M, doc1, doc2);
            if (M3.count() > M2.count())
                M2 = M3;
        }
        M += M2;
    }
    return M;
}


static QString _scriptCode(UiScriptBinding *script, Document::Ptr doc)
{
    if (script) {
        const int begin = script->statement->firstSourceLocation().begin();
        const int end = script->statement->lastSourceLocation().end();
        return doc->source().mid(begin, end - begin);
    }
    return QString();
}

static QString _methodCode(UiSourceElement *source, Document::Ptr doc)
{
    if (source) {
        if (FunctionDeclaration *declaration = cast<FunctionDeclaration*>(source->sourceElement)) {
            const int begin = declaration->lbraceToken.begin() + 1;
            const int end = declaration->rbraceToken.end() - 1;
            return doc->source().mid(begin, end - begin);
        }
    }
    return QString();
}


static QString _propertyName(UiQualifiedId *id)
{
    QString s;

    for (; id; id = id->next) {
        if (id->name.isEmpty())
            return QString();

        s += id->name;

        if (id->next)
            s += QLatin1Char('.');
    }

    return s;
}

static QString _methodName(UiSourceElement *source)
{
    if (source) {
        if (FunctionDeclaration *declaration = cast<FunctionDeclaration*>(source->sourceElement))
            return declaration->name.toString();
    }
    return QString();
}

static UiObjectMemberList *objectMembers(UiObjectMember *object)
{
    if (UiObjectInitializer *init = QmlJS::initializerOfObject(object))
        return init->members;

    return 0;
}


static QHash<QString, UiObjectMember*> extractProperties(UiObjectMember *object)
{
    QHash<QString, UiObjectMember*> result;
    for (UiObjectMemberList *objectMemberIt = objectMembers(object); objectMemberIt; objectMemberIt = objectMemberIt->next) {
        if (UiScriptBinding *script = cast<UiScriptBinding *>(objectMemberIt->member)) {
            const QString property = _propertyName(script->qualifiedId);
            result.insert(property, script);
        } else if (UiObjectDefinition *uiObject = cast<UiObjectDefinition *>(objectMemberIt->member)) {
            const QString l = label(uiObject->qualifiedTypeNameId);
            if (!l.isEmpty() && !l[0].isUpper()) {
                QHash<QString, UiObjectMember *> recursiveResult = extractProperties(uiObject);
                for (QHash<QString, UiObjectMember *>::const_iterator it = recursiveResult.constBegin();
                     it != recursiveResult.constEnd(); ++it) {
                    result.insert(l + QLatin1Char('.') + it.key(), it.value());
                }
            }
        } else if (UiSourceElement *uiSource = cast<UiSourceElement*>(objectMemberIt->member)) {
            const QString methodName = _methodName(uiSource);
            result.insert(methodName, uiSource);
        }
    }
    return result;
}

} //end namespace

namespace QmlJS {

void Delta::insert(UiObjectMember *member, UiObjectMember *parentMember, const QList<DebugId> &debugReferences, const Document::Ptr &doc)
{
    if (!member || !parentMember)
        return;

    bool accepted = false;
    // initialized with garbage
    unsigned begin = 1, end = 2, startColumn = 3, startLine = 4;

    // create new objects
    if (UiObjectDefinition* uiObjectDef = cast<UiObjectDefinition *>(member)) {
        begin = uiObjectDef->firstSourceLocation().begin();
        end = uiObjectDef->lastSourceLocation().end();
        startColumn = uiObjectDef->firstSourceLocation().startColumn;
        startLine = uiObjectDef->firstSourceLocation().startLine;
        accepted = true;
    }

    if (UiObjectBinding* uiObjectBind = cast<UiObjectBinding *>(member)) {
        SourceLocation definitionLocation = uiObjectBind->qualifiedTypeNameId->identifierToken;
        begin = definitionLocation.begin();
        end = uiObjectBind->lastSourceLocation().end();
        startColumn = definitionLocation.startColumn;
        startLine = definitionLocation.startLine;
        accepted = true;
    }

    if (accepted) {
        QString qmlText = QString(startColumn - 1, QLatin1Char(' '));
        qmlText += doc->source().midRef(begin, end - begin);

        QStringList importList;
        for (UiImportList *it = doc->qmlProgram()->imports; it; it = it->next) {
            if (!it->import)
                continue;
            unsigned importBegin = it->import->firstSourceLocation().begin();
            unsigned importEnd = it->import->lastSourceLocation().end();

            importList << doc->source().mid(importBegin, importEnd - importBegin);
        }

        // encode editorRevision, lineNumber in URL. See ClientProxy::buildDebugIdHashRecursive
        // Also, a relative URL is expected (no "file://" prepending!)
        QString filename =  doc->fileName() + QLatin1Char('_') + QString::number(doc->editorRevision())
                         + QLatin1Char(':') + QString::number(startLine-importList.count());
        foreach (DebugId debugId, debugReferences) {
            if (debugId != -1) {
                int order = 0;
                // skip children which are not objects
                foreach (const UiObjectMember *child, children(parentMember)) {
                    if (child == member) break;
                    if (child->kind == AST::Node::Kind_UiObjectDefinition)
                        order++;
                }
                createObject(qmlText, debugId, importList, filename, order);
            }
        }
        newObjects += member;
    }
}

void Delta::update(UiObjectMember* oldObject, const QmlJS::Document::Ptr& oldDoc,
                   UiObjectMember* newObject, const QmlJS::Document::Ptr& newDoc,
                   const QList<DebugId>& debugReferences)
{
    Q_ASSERT (oldObject && newObject);
    QSet<QString> presentBinding;

    const QHash<QString, UiObjectMember *> oldProperties = extractProperties(oldObject);
    const QHash<QString, UiObjectMember *> newProperties = extractProperties(newObject);

    for (QHash<QString, UiObjectMember *>::const_iterator it = newProperties.constBegin();
         it != newProperties.constEnd(); ++it) {

        UiObjectMember *oldMember = oldProperties.value(it.key());
        if (UiScriptBinding *script = cast<UiScriptBinding *>(*it)) {
            const QString property = it.key();
            const QString scriptCode = _scriptCode(script, newDoc);
            UiScriptBinding *previousScript = cast<UiScriptBinding *>(oldMember);
            if (!previousScript || _scriptCode(previousScript, oldDoc) != scriptCode) {
                if (debugReferences.count()==0)
                    notifyUnsyncronizableElementChange(newObject);
                foreach (DebugId ref, debugReferences) {
                    if (ref != -1)
                        updateScriptBinding(ref, newObject, script, property, scriptCode);
                }
            }
        } else if (UiSourceElement *uiSource = cast<UiSourceElement*>(*it)) {
            const QString methodName = it.key();
            const QString methodCode = _methodCode(uiSource, newDoc);
            UiSourceElement *previousSource = cast<UiSourceElement*>(oldMember);

            if (!previousSource || _methodCode(previousSource, oldDoc) != methodCode) {
                if (debugReferences.count()==0)
                    notifyUnsyncronizableElementChange(newObject);
                foreach (DebugId ref, debugReferences) {
                    if (ref != -1)
                        updateMethodBody(ref, newObject, script, methodName, methodCode);
                }
            }
        }
    }

    //reset property that are not present in the new object.
    for (QHash<QString, UiObjectMember *>::const_iterator it2 = oldProperties.constBegin();
         it2 != oldProperties.constEnd(); ++it2) {

        if (!newProperties.contains(it2.key())) {
            if (cast<UiScriptBinding *>(*it2)) {
                foreach (DebugId ref, debugReferences) {
                    if (ref != -1)
                        resetBindingForObject(ref, it2.key());
                }
            }
        }
    }
}

void Delta::remove(const QList<DebugId>& debugReferences)
{
    foreach (DebugId ref, debugReferences) {
        if (ref != -1)
            removeObject(ref);
    }
}

void Delta::reparent(const QList <DebugId> &member, const QList<DebugId> &newParent)
{
    if (member.length() != newParent.length()) return;

    for (int i=0; i<member.length(); i++)
        reparentObject(member.at(i), newParent.at(i));
}


Delta::DebugIdMap Delta::operator()(const Document::Ptr &doc1, const Document::Ptr &doc2, const DebugIdMap &debugIds)
{
    Q_ASSERT(doc1->qmlProgram());
    Q_ASSERT(doc2->qmlProgram());

    m_previousDoc = doc1;
    m_currentDoc = doc2;

    Delta::DebugIdMap newDebuggIds;

    Map M = buildMapping(doc1, doc2);

    BuildParentHash parents2;
    doc2->qmlProgram()->accept(&parents2);
    BuildParentHash parents1;
    doc1->qmlProgram()->accept(&parents1);

    QList<UiObjectMember *> todo;
    todo.append(doc2->qmlProgram()->members->member);
    //UiObjectMemberList *list = 0;
    while (!todo.isEmpty()) {
        UiObjectMember *y = todo.takeFirst();
        todo += children(y);

        if (!cast<UiObjectDefinition *>(y) && !cast<UiObjectBinding *>(y))
            continue;

        if (!M.way2.contains(y)) {
            UiObjectMember* parent = parents2.parent.value(y);
            if (parent) {
                if ( parent->kind == QmlJS::AST::Node::Kind_UiArrayBinding )
                    parent = parents2.parent.value(parent);

                if (M.way2.contains(parent) && newDebuggIds.value(parent).count() > 0) {
                    if (debug)
                        qDebug () << "Delta::operator():  insert " << label(y, doc2) << " to " << label(parent, doc2);
                    insert(y, parent, newDebuggIds.value(parent), doc2);
                }
            }
            continue;
        }
        UiObjectMember *x = M.way2[y];
        Q_ASSERT(cast<UiObjectDefinition *>(x) || cast<UiObjectBinding *>(x) );

        {
            QList<DebugId> updateIds;
            if (debugIds.contains(x)) {
                updateIds = debugIds[x];
                newDebuggIds[y] = updateIds;
            }
            if (debug)
                qDebug () << "Delta::operator(): update " << label(x,doc1);
            update(x, doc1, y, doc2, updateIds);
        }
        //qDebug() << "Delta::operator():  match "<< label(x, doc1) << "with parent " << label(parents1.parent.value(x), doc1)
        //     << " to "<< label(y, doc2) << "with parent " << label(parents2.parent.value(y), doc2);

        if (!M.contains(parents1.parent.value(x),parents2.parent.value(y))) {
            if (debug)
                qDebug () << "Delta::operator():  move " << label(y, doc2) << " from " << label(parents1.parent.value(x), doc1)
            << " to " << label(parents2.parent.value(y), doc2);
            reparent(newDebuggIds.value(y), newDebuggIds.value(parents2.parent.value(y)));
            continue;
        }
    }

    todo.append(doc1->qmlProgram()->members->member);
    while (!todo.isEmpty()) {
        UiObjectMember *x = todo.takeFirst();
        todo += children(x);
        if (!cast<UiObjectDefinition *>(x))
            continue;
        if (!M.way1.contains(x)) {
            if (debug)
                qDebug () << "Delta::operator():  remove " << label(x, doc1);
            QList<DebugId> ids = debugIds.value(x);
            if (!ids.isEmpty())
                remove(ids);
            continue;
        }
    }
    return newDebuggIds;
}

Document::Ptr Delta::document() const
{
    return m_currentDoc;
}

Document::Ptr Delta::previousDocument() const
{
    return m_previousDoc;
}

void Delta::createObject(const QString &, DebugId, const QStringList &, const QString&, int)
{}
void Delta::removeObject(int)
{}
void Delta::reparentObject(int, int)
{}
void Delta::resetBindingForObject(int, const QString &)
{}
void Delta::updateMethodBody(DebugId, UiObjectMember *, UiScriptBinding *, const QString &, const QString &)
{}

void Delta::updateScriptBinding(DebugId, UiObjectMember *, UiScriptBinding *, const QString &, const QString &)
{}

void Delta::notifyUnsyncronizableElementChange(UiObjectMember *)
{}

} //namespace QmlJs

