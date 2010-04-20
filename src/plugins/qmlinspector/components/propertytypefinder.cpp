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
    , m_link(&m_context, doc, snapshot, importPaths), m_depth(0)
{
}

QString PropertyTypeFinder::operator()(int objectLine, int objectColumn, const QString &propertyName)
{
    m_objectLine = objectLine;
    m_objectColumn = objectColumn;
    m_typeNameId = 0;

    Node::accept(m_doc->ast(), this);
    if (m_typeNameId) {
        for (const ObjectValue *iter = m_context.lookupType(m_doc.data(), m_typeNameId); iter; iter = iter->prototype(&m_context)) {
            if (iter->lookupMember(propertyName, &m_context, false)) {
                // gotcha!
                return iter->className();
            }
            ++m_depth;
        }
    }
    //### Eep: we didn't find it...
    return QString();
}

int PropertyTypeFinder::depth() const
{
    return m_depth;
}

bool PropertyTypeFinder::visit(QmlJS::AST::UiObjectBinding *ast)
{
    return check(ast->qualifiedTypeNameId);
}

bool PropertyTypeFinder::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    return check(ast->qualifiedTypeNameId);
}

bool PropertyTypeFinder::check(QmlJS::AST::UiQualifiedId *qId)
{
    if (!qId)
        return true;

    if (qId->identifierToken.startLine == m_objectLine
        && qId->identifierToken.startColumn == m_objectColumn) {
        // got it!
        m_typeNameId = qId;
        return false;
    }

    return true;
}
