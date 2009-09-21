#ifndef IDCOLLECTOR_H
#define IDCOLLECTOR_H

#include <QMap>
#include <QPair>
#include <QStack>
#include <QString>

#include "qmljsastvisitor_p.h"

namespace QmlJS { class NameId; }

namespace DuiEditor {
namespace Internal {

class IdCollector: protected QmlJS::AST::Visitor
{
public:
    QMap<QString, QPair<QmlJS::AST::SourceLocation, QmlJS::AST::Node*> > operator()(QmlJS::AST::UiProgram *ast);

protected:
    virtual bool visit(QmlJS::AST::UiObjectBinding *ast);
    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);
    virtual bool visit(QmlJS::AST::UiScriptBinding *ast);

    virtual void endVisit(QmlJS::AST::UiObjectBinding *);
    virtual void endVisit(QmlJS::AST::UiObjectDefinition *);

private:
    void addId(QmlJS::NameId* id, const QmlJS::AST::SourceLocation &idLocation);

private:
    QMap<QString, QPair<QmlJS::AST::SourceLocation, QmlJS::AST::Node*> > _ids;
    QStack<QmlJS::AST::Node *> _scopes;
};

} // namespace Internal
} // namespace DuiEditor

#endif // IDCOLLECTOR_H
