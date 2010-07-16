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

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJSInspector::Internal;

namespace {

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

}

void Delta::insert(UiObjectMember *member, UiObjectMember *parentMember, const QList<QDeclarativeDebugObjectReference > &debugReferences, const Document::Ptr &doc)
{
    if (!member || !parentMember)
        return;

    // create new objects
    if (UiObjectDefinition* uiObjectDef = cast<UiObjectDefinition *>(member)) {
        unsigned begin = uiObjectDef->firstSourceLocation().begin();
        unsigned end = uiObjectDef->lastSourceLocation().end();
        QString qmlText = doc->source().mid(begin, end - begin);
        QStringList importList;
        for (UiImportList *it = doc->qmlProgram()->imports; it; it = it->next) {
            if (!it->import)
                continue;
            unsigned importBegin = it->import->firstSourceLocation().begin();
            unsigned importEnd = it->import->lastSourceLocation().end();

            importList << doc->source().mid(importBegin, importEnd - importBegin);
        }

        foreach(const QDeclarativeDebugObjectReference &ref, debugReferences) {
            if (ref.debugId() != -1) {
                ClientProxy::instance()->createQmlObject(qmlText, ref, importList, doc->fileName());
            }
        }
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

        if (!M.way2.contains(y)) {
            insert(y, parents2.parent.value(y), newDebuggIds.value(parents2.parent.value(y)), doc2);
            qDebug () << "insert " << label(y, doc2) << " to " << label(parents2.parent.value(y), doc2);
            continue;
        }
        UiObjectMember *x = M.way2[y];

//--8<---------------------------------------------------------------------------------------
        if (debugIds.contains(x)) {
            newDebuggIds[y] = debugIds[x];

#if 1
            UiObjectMember *object = y;
            UiObjectMember *previousObject = x;

            for (UiObjectMemberList *objectMemberIt = objectMembers(object); objectMemberIt; objectMemberIt = objectMemberIt->next) {
                if (UiScriptBinding *script = cast<UiScriptBinding *>(objectMemberIt->member)) {
                    bool found = false;
                    for (UiObjectMemberList *previousObjectMemberIt = Delta::objectMembers(previousObject); previousObjectMemberIt; previousObjectMemberIt = previousObjectMemberIt->next) {
                        if (UiScriptBinding *previousScript = cast<UiScriptBinding *>(previousObjectMemberIt->member)) {
                            if (compare(script->qualifiedId, previousScript->qualifiedId)) {
                                found = true;
                                const QString scriptCode = _scriptCode(script, doc2);
                                const QString previousScriptCode = _scriptCode(previousScript, doc1);

                                if (scriptCode != previousScriptCode) {
                                    const QString property = _propertyName(script->qualifiedId);
                                    foreach (const QDeclarativeDebugObjectReference &ref, debugIds[x]) {
                                        if (ref.debugId() != -1)
                                            updateScriptBinding(ref, script, property, scriptCode);
                                    }
                                }
                            }
                        }
                    }
                    if (!found) {
                        const QString scriptCode = _scriptCode(script, doc2);
                        const QString property = _propertyName(script->qualifiedId);
                        foreach (const QDeclarativeDebugObjectReference &ref, debugIds[x]) {
                            if (ref.debugId() != -1)
                                updateScriptBinding(ref, script, property, scriptCode);
                        }
                    }
                } else if (UiSourceElement *uiSource = cast<UiSourceElement*>(objectMemberIt->member)) {
                    for (UiObjectMemberList *previousObjectMemberIt = objectMembers(previousObject);
                    previousObjectMemberIt; previousObjectMemberIt = previousObjectMemberIt->next)
                    {
                        if (UiSourceElement *previousSource = cast<UiSourceElement*>(previousObjectMemberIt->member)) {
                            if (compare(uiSource, previousSource))
                            {
                                const QString methodCode = _methodCode(uiSource, doc2);
                                const QString previousMethodCode = _methodCode(previousSource, doc1);

                                if (methodCode != previousMethodCode) {
                                    const QString methodName = _methodName(uiSource);
                                    foreach (const QDeclarativeDebugObjectReference &ref, debugIds[x]) {
                                        if (ref.debugId() != -1)
                                            updateMethodBody(ref, script, methodName, methodCode);
                                    }
                                }
                            }
                        }
                    }
                }
            }
#endif
        }

//--8<--------------------------------------------------------------------------------------------------

        //qDebug() << "match "<< label(x, doc1) << "with parent " << label(parents1.parent.value(x), doc1)
        //     << " to "<< label(y, doc2) << "with parent " << label(parents2.parent.value(y), doc2);
        if (!M.contains(parents1.parent.value(x),parents2.parent.value(y))) {
            qDebug () << "move " << label(y, doc2) << " from " << label(parents1.parent.value(x), doc1)
            << " to " << label(parents2.parent.value(y), doc2);
            continue;
        }
    }

    todo.append(doc1->qmlProgram()->members->member);
    while(!todo.isEmpty()) {
        UiObjectMember *x = todo.takeFirst();
        todo += children(x);
        if (!M.way1.contains(x)) {
            qDebug () << "remove " << label(x, doc1);
            continue;
        }
    }
    return newDebuggIds;
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

void Delta::updateMethodBody(const QDeclarativeDebugObjectReference &objectReference,
                               UiScriptBinding *scriptBinding,
                               const QString &methodName,
                               const QString &methodBody)
{
    Change change;
    change.script = scriptBinding;
    change.ref = objectReference;
    change.isLiteral = false;
    _changes.append(change);

    ClientProxy::instance()->setMethodBodyForObject(objectReference.debugId(), methodName, methodBody); // ### remove
}

void Delta::updateScriptBinding(const QDeclarativeDebugObjectReference &objectReference,
                                UiScriptBinding *scriptBinding,
                                const QString &propertyName,
                                const QString &scriptCode)
{
    QVariant expr = scriptCode;

    const bool isLiteral = isLiteralValue(scriptBinding);
    if (isLiteral)
        expr = castToLiteral(scriptCode, scriptBinding);

    Change change;
    change.script = scriptBinding;
    change.ref = objectReference;
    change.isLiteral = isLiteral;
    _changes.append(change);

    ClientProxy::instance()->setBindingForObject(objectReference.debugId(), propertyName, expr, isLiteral); // ### remove
}

bool Delta::compare(UiQualifiedId *id, UiQualifiedId *other)
{
    if (id == other)
        return true;

    else if (id && other) {
        if (id->name && other->name) {
            if (id->name->asString() == other->name->asString())
                return compare(id->next, other->next);
        }
    }

    return false;
}

bool Delta::compare(UiSourceElement *source, UiSourceElement *other)
{
    if (source == other)
        return true;

    else if (source && other) {
        if (source->sourceElement && other->sourceElement) {
            FunctionDeclaration *decl = cast<FunctionDeclaration*>(source->sourceElement);
            FunctionDeclaration *otherDecl = cast<FunctionDeclaration*>(other->sourceElement);
            if (decl && otherDecl
                && decl->name && otherDecl->name
                && decl->name->asString() == otherDecl->name->asString())
            {
                    return true;
            }
        }
    }

    return false;
}

UiObjectMemberList *Delta::objectMembers(UiObjectMember *object)
{
    if (UiObjectDefinition *def = cast<UiObjectDefinition *>(object))
        return def->initializer->members;
    else if (UiObjectBinding *binding = cast<UiObjectBinding *>(object))
        return binding->initializer->members;

    return 0;
}

Document::Ptr Delta::document() const
{
    return _doc;
}

Document::Ptr Delta::previousDocument() const
{
    return _previousDoc;
}

QList<Delta::Change> Delta::changes() const
{
    return _changes;
}
