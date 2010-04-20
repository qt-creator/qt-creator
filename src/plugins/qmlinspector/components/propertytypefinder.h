#ifndef PROPERTYTYPEFINDER_H
#define PROPERTYTYPEFINDER_H

#include <qmljs/qmljsdocument.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljslink.h>

namespace Qml {
namespace Internal {

class PropertyTypeFinder: protected QmlJS::AST::Visitor
{
public:
    PropertyTypeFinder(QmlJS::Document::Ptr doc, QmlJS::Snapshot snapshot, const QStringList &importPaths);

    QString operator()(int objectLine, int objectColumn, const QString &propertyName);
    int depth() const;
protected:
    using QmlJS::AST::Visitor::visit;

    virtual bool visit(QmlJS::AST::UiObjectBinding *ast);
    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);


private:
    bool check(QmlJS::AST::UiQualifiedId *qId);

private:
    QmlJS::Document::Ptr m_doc;
    QmlJS::Snapshot m_snapshot;
    QmlJS::Interpreter::Engine m_engine;
    QmlJS::Interpreter::Context m_context;
    QmlJS::Link m_link;

    quint32 m_objectLine;
    quint32 m_objectColumn;
    QmlJS::AST::UiQualifiedId *m_typeNameId;
    quint8 m_depth;
};

} // namespace Internal
} // namespace Qml

#endif // PROPERTYTYPEFINDER_H
