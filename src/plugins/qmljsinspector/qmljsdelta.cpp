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

#include "qmljsdelta.h"
#include "qmljsclientproxy.h"
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsastvisitor_p.h>

#include <typeinfo>
#include <QtCore/QDebug>

using namespace QmlJS::AST;

namespace {
using namespace QmlJS;

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
    if (ast->uiObjectMemberCast()) {
        stack.append(ast->uiObjectMemberCast());
    }
    return true;
}


void BuildParentHash::postVisit(Node* ast)
{
    if (ast->uiObjectMemberCast()) {
        stack.removeLast();
        if (!stack.isEmpty()) {
            parent.insert(ast->uiObjectMemberCast(), stack.last());
        }
    }
}

static QString label(UiQualifiedId *id)
{
    QString str;
    for (; id ; id = id->next) {
        if (!id->name)
            return QString();
        if (!str.isEmpty())
            str += QLatin1Char('.');
        str += id->name->asString();
    }
    return str;
}

static QString label(UiObjectMember *member, Document::Ptr doc)
{
    QString str;
    if(!member)
        return str;

    if (UiObjectDefinition* foo = cast<UiObjectDefinition *>(member)) {
        str = label(foo->qualifiedTypeNameId);
    } else if(UiObjectBinding *foo = cast<UiObjectBinding *>(member)) {
        str = label(foo->qualifiedId) + QLatin1Char(' ') + label(foo->qualifiedTypeNameId);
    } else if(UiArrayBinding *foo = cast<UiArrayBinding *>(member)) {
        str = label(foo->qualifiedId) + QLatin1String("[]");
    } else if(UiScriptBinding *foo = cast<UiScriptBinding *>(member)) {
        Q_UNUSED(foo)
    } else {
        quint32 start = member->firstSourceLocation().begin();
        quint32 end = member->lastSourceLocation().end();
        str = doc->source().mid(start, end-start);
    }
    return str;
}

struct FindObjectMemberWithLabel : public Visitor
{
    virtual void endVisit(UiObjectDefinition *ast) ;
    virtual void endVisit(UiObjectBinding *ast) ;

    QList<UiObjectMember *> found;
    QString cmp;
    Document::Ptr doc;
};

void FindObjectMemberWithLabel::endVisit(UiObjectDefinition* ast)
{
    if (label(ast, doc) == cmp)
        found.append(ast);
}
void FindObjectMemberWithLabel::endVisit(UiObjectBinding* ast)
{
    if (label(ast, doc) == cmp)
        found.append(ast);
}

struct Map {
    typedef UiObjectMember*T;
    QHash<T, T> way1;
    QHash<T, T> way2;
    void insert(T t1, T t2) {
        way1.insert(t1,t2);
        way2.insert(t2,t1);
    }
    int count() { return way1.count(); }
    void operator+=(const Map &other) {
        way1.unite(other.way1);
        way2.unite(other.way2);
    }
    bool contains(T t1, T t2) {
        return way1.value(t1) == t2;
    }
};

static QList<UiObjectMember *> children(UiObjectMember *ast)
{
    QList<UiObjectMember *> ret;
    if (UiObjectDefinition* foo = cast<UiObjectDefinition *>(ast)) {
        UiObjectMemberList* list = foo->initializer->members;
        while (list) {
            ret.append(list->member);
            list = list->next;
        }
    } else if(UiObjectBinding *foo = cast<UiObjectBinding *>(ast)) {
        UiObjectMemberList* list = foo->initializer->members;
        while (list) {
            ret.append(list->member);
            list = list->next;
        }
    } else if(UiArrayBinding *foo = cast<UiArrayBinding *>(ast)) {
        UiArrayMemberList* list = foo->members;
        while (list) {
            ret.append(list->member);
            list = list->next;
        }
    }
    return ret;
}

Map MatchFragment(UiObjectMember *x, UiObjectMember *y, const Map &M, Document::Ptr doc1, Document::Ptr doc2) {
    Map M2;
    if (M.way1.contains(x))
        return M2;
    if (M.way2.contains(y))
        return M2;
    if(label(x, doc1) != label(y, doc2))
        return M2;
    M2.insert(x, y);
    QList<UiObjectMember *> list1 = children(x);
    QList<UiObjectMember *> list2 = children(y);
    for (int i = 0; i < list1.count(); i++) {
        QString l = label(list1[i], doc1);
        for (int j = 0; j < list2.count(); j++) {
            if (l != label(list2[j], doc2))
                continue;
            M2 += MatchFragment(list1[i], list2[j], M, doc1, doc2);
            list2.removeAt(j);
            break;
        }
    }
    return M2;
}

Map Mapping(Document::Ptr doc1, Document::Ptr doc2)
{
    Map M;
    QList<UiObjectMember *> todo;
    todo.append(doc1->qmlProgram()->members->member);
    while(!todo.isEmpty()) {
        UiObjectMember *x = todo.takeFirst();
        todo += children(x);

        if (M.way1.contains(x))
            continue;

        //If this is too slow, we could use some sort of indexing
        FindObjectMemberWithLabel v3;
        v3.cmp = label(x, doc1);
        v3.doc = doc2;
        doc2->qmlProgram()->accept(&v3);
        Map M2;
        foreach (UiObjectMember *y, v3.found) {
            if (M.way2.contains(y))
                continue;
            Map M3 = MatchFragment(x, y, M, doc1, doc2);
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
        if (! id->name)
            return QString();

        s += id->name->asString();

        if (id->next)
            s += QLatin1Char('.');
    }

    return s;
}

static QString _methodName(UiSourceElement *source)
{
    if (source) {
        if (FunctionDeclaration *declaration = cast<FunctionDeclaration*>(source->sourceElement)) {
            return declaration->name->asString();
        }
    }
    return QString();
}

static UiObjectMemberList *objectMembers(UiObjectMember *object)
{
    if (UiObjectDefinition *def = cast<UiObjectDefinition *>(object))
        return def->initializer->members;
    else if (UiObjectBinding *binding = cast<UiObjectBinding *>(object))
        return binding->initializer->members;

    return 0;
}


static QHash<QString, UiObjectMember*> extractProperties(UiObjectDefinition *object)
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

void Delta::insert(UiObjectMember *member, UiObjectMember *parentMember, const QList<QDeclarativeDebugObjectReference > &debugReferences, const Document::Ptr &doc)
{
    if (!member || !parentMember)
        return;

    // create new objects
    if (UiObjectDefinition* uiObjectDef = cast<UiObjectDefinition *>(member)) {
        unsigned begin = uiObjectDef->firstSourceLocation().begin();
        unsigned end = uiObjectDef->lastSourceLocation().end();
        QString qmlText = QString(uiObjectDef->firstSourceLocation().startColumn - 1, QLatin1Char(' '));
        qmlText += doc->source().midRef(begin, end - begin);
        QStringList importList;
        for (UiImportList *it = doc->qmlProgram()->imports; it; it = it->next) {
            if (!it->import)
                continue;
            unsigned importBegin = it->import->firstSourceLocation().begin();
            unsigned importEnd = it->import->lastSourceLocation().end();

            importList << doc->source().mid(importBegin, importEnd - importBegin);
        }

        QString filename = doc->fileName() + QLatin1Char('_') + QString::number(doc->editorRevision())
                         + QLatin1Char(':') + QString::number(uiObjectDef->firstSourceLocation().startLine-importList.count());
        foreach(const QDeclarativeDebugObjectReference &ref, debugReferences) {
            if (ref.debugId() != -1) {
                createObject(qmlText, ref, importList, filename);
            }
        }
        newObjects += member;
    }
}


void Delta::update(UiObjectDefinition* oldObject, const QmlJS::Document::Ptr& oldDoc,
                   UiObjectDefinition* newObject, const QmlJS::Document::Ptr& newDoc,
                   const QList< QDeclarativeDebugObjectReference >& debugReferences)
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
                foreach (const QDeclarativeDebugObjectReference &ref, debugReferences) {
                    if (ref.debugId() != -1)
                        updateScriptBinding(ref, script, property, scriptCode);
                }
            }
        } else if (UiSourceElement *uiSource = cast<UiSourceElement*>(*it)) {
            const QString methodName = it.key();
            const QString methodCode = _methodCode(uiSource, newDoc);
            UiSourceElement *previousSource = cast<UiSourceElement*>(oldMember);

            if (!previousSource || _methodCode(previousSource, oldDoc) != methodCode) {
                foreach (const QDeclarativeDebugObjectReference &ref, debugReferences) {
                    if (ref.debugId() != -1)
                        updateMethodBody(ref, script, methodName, methodCode);
                }
            }
        }
    }

    //reset property that are not present in the new object.
    for (QHash<QString, UiObjectMember *>::const_iterator it2 = oldProperties.constBegin();
         it2 != oldProperties.constEnd(); ++it2) {

        if (!newProperties.contains(it2.key())) {
            if (cast<UiScriptBinding *>(*it2)) {
                foreach (const QDeclarativeDebugObjectReference &ref, debugReferences) {
                    if (ref.debugId() != -1)
                        resetBindingForObject(ref.debugId(), it2.key());
                }
            }
        }
    }
}

void Delta::remove(const QList< QDeclarativeDebugObjectReference >& debugReferences)
{
    foreach (const QDeclarativeDebugObjectReference &ref, debugReferences) {
        if (ref.debugId() != -1)
            removeObject(ref.debugId());
    }
}


Delta::DebugIdMap Delta::operator()(const Document::Ptr &doc1, const Document::Ptr &doc2, const DebugIdMap &debugIds)
{
    Q_ASSERT(doc1->qmlProgram());
    Q_ASSERT(doc2->qmlProgram());

    QHash< UiObjectMember*, QList<QDeclarativeDebugObjectReference > > newDebuggIds;

    Map M = Mapping(doc1, doc2);

    BuildParentHash parents2;
    doc2->qmlProgram()->accept(&parents2);
    BuildParentHash parents1;
    doc1->qmlProgram()->accept(&parents1);

    QList<UiObjectMember *> todo;
    todo.append(doc2->qmlProgram()->members->member);
    //UiObjectMemberList *list = 0;
    while(!todo.isEmpty()) {
        UiObjectMember *y = todo.takeFirst();
        todo += children(y);

        if (!cast<UiObjectDefinition *>(y))
            continue;

        if (!M.way2.contains(y)) {
            UiObjectMember* parent = parents2.parent.value(y);
            if (!M.way2.contains(parent))
                continue;
            qDebug () << "Delta::operator():  insert " << label(y, doc2) << " to " << label(parent, doc2);
            insert(y, parent, newDebuggIds.value(parent), doc2);
            continue;
        }
        UiObjectMember *x = M.way2[y];
        Q_ASSERT(cast<UiObjectDefinition *>(x));

        if (debugIds.contains(x)) {
            QList< QDeclarativeDebugObjectReference > ids = debugIds[x];
            newDebuggIds[y] = ids;
            update(cast<UiObjectDefinition *>(x), doc1, cast<UiObjectDefinition *>(y), doc2, ids);
        }
        //qDebug() << "Delta::operator():  match "<< label(x, doc1) << "with parent " << label(parents1.parent.value(x), doc1)
        //     << " to "<< label(y, doc2) << "with parent " << label(parents2.parent.value(y), doc2);

        if (!M.contains(parents1.parent.value(x),parents2.parent.value(y))) {
            qDebug () << "Delta::operator():  move " << label(y, doc2) << " from " << label(parents1.parent.value(x), doc1)
            << " to " << label(parents2.parent.value(y), doc2)  << " ### TODO";
            continue;
        }
    }

    todo.append(doc1->qmlProgram()->members->member);
    while(!todo.isEmpty()) {
        UiObjectMember *x = todo.takeFirst();
        todo += children(x);
        if (!cast<UiObjectDefinition *>(x))
            continue;
        if (!M.way1.contains(x)) {
            qDebug () << "Delta::operator():  remove " << label(x, doc1);
            QList< QDeclarativeDebugObjectReference > ids = debugIds.value(x);
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

void Delta::createObject(const QString &, const QDeclarativeDebugObjectReference &,
                         const QStringList &, const QString&)
{}
void Delta::removeObject(int)
{}
void Delta::resetBindingForObject(int, const QString &)
{}
void Delta::updateMethodBody(const QDeclarativeDebugObjectReference &,
                             UiScriptBinding *, const QString &, const QString &)
{}

void Delta::updateScriptBinding(const QDeclarativeDebugObjectReference &,
                                UiScriptBinding *, const QString &, const QString &)
{}

} //namespace QmlJs

