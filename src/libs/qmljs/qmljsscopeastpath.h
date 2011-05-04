#ifndef QMLJSSCOPEASTPATH_H
#define QMLJSSCOPEASTPATH_H

#include "qmljs_global.h"
#include "parser/qmljsastvisitor_p.h"
#include "qmljsdocument.h"

namespace QmlJS {

class QMLJS_EXPORT ScopeAstPath: protected AST::Visitor
{
public:
    ScopeAstPath(Document::Ptr doc);

    QList<AST::Node *> operator()(quint32 offset);

protected:
    void accept(AST::Node *node);

    using Visitor::visit;

    virtual bool preVisit(AST::Node *node);
    virtual bool visit(AST::UiObjectDefinition *node);
    virtual bool visit(AST::UiObjectBinding *node);
    virtual bool visit(AST::FunctionDeclaration *node);
    virtual bool visit(AST::FunctionExpression *node);

private:
    bool containsOffset(AST::SourceLocation start, AST::SourceLocation end);

    QList<AST::Node *> _result;
    Document::Ptr _doc;
    quint32 _offset;
};

} // namespace QmlJS

#endif // QMLJSSCOPEASTPATH_H
