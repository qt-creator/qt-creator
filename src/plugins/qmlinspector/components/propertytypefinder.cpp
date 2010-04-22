#include "propertytypefinder.h"

#include <qmljs/parser/qmljsast_p.h>

#include <QDebug>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJS::Interpreter;
using namespace Qml::Internal;

PropertyTypeFinder::PropertyTypeFinder(QmlJS::Document::Ptr doc, QmlJS::Snapshot snapshot, const QStringList &importPaths)
    : m_doc(doc)
    , m_snapshot(snapshot)
    , m_engine()
    , m_context(&m_engine)
    , m_link(&m_context, doc, snapshot, importPaths)
    , m_scopeBuilder(doc, &m_context)
    , m_depth(0)
{
}

QString PropertyTypeFinder::operator()(int objectLine, int objectColumn, const QString &propertyName)
{
    m_objectLine = objectLine;
    m_objectColumn = objectColumn;
    m_propertyName = propertyName;
    m_definingClass.clear();

    Node::accept(m_doc->ast(), this);
    return m_definingClass;
}

int PropertyTypeFinder::depth() const
{
    return m_depth;
}

bool PropertyTypeFinder::visit(QmlJS::AST::UiObjectBinding *ast)
{
    m_scopeBuilder.push(ast);

    return check(ast->qualifiedTypeNameId);
}

bool PropertyTypeFinder::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    m_scopeBuilder.push(ast);

    return check(ast->qualifiedTypeNameId);
}

void PropertyTypeFinder::endVisit(QmlJS::AST::UiObjectBinding * /*ast*/)
{
    m_scopeBuilder.pop();
}

void PropertyTypeFinder::endVisit(QmlJS::AST::UiObjectDefinition * /*ast*/)
{
    m_scopeBuilder.pop();
}

bool PropertyTypeFinder::check(QmlJS::AST::UiQualifiedId *qId)
{
    if (!qId)
        return true;

    if (qId->identifierToken.startLine == m_objectLine
        && qId->identifierToken.startColumn == m_objectColumn) {
        // got it!
        for (const ObjectValue *iter = m_context.scopeChain().qmlScopeObjects.last(); iter; iter = iter->prototype(&m_context)) {

            if (iter->lookupMember(m_propertyName, &m_context, false)) {
                m_definingClass = iter->className();
                return true;
            }
            ++m_depth;
        }
        return false;
    }

    return true;
}
