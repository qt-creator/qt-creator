#ifndef NAVIGATIONTOKENFINDER_H
#define NAVIGATIONTOKENFINDER_H

#include <QStack>
#include <QString>

#include "qmljsastvisitor_p.h"

namespace QmlJS {
    class NameId;
} // namespace QmlJS

namespace DuiEditor {
namespace Internal {

class NavigationTokenFinder: protected QmlJS::AST::Visitor
{
public:
    bool operator()(QmlJS::AST::UiProgram *ast, int position, bool resolveTarget);

    bool targetFound() const { return _targetLine != -1; }
    bool linkFound() const { return _linkPosition != -1; }

    int linkPosition() const { return _linkPosition; }
    int linkLength() const { return _linkLength; }
    int targetLine() const { return _targetLine; }
    int targetColumn() const { return _targetColumn; }

protected:
    virtual bool visit(QmlJS::AST::Block *ast);
    virtual bool visit(QmlJS::AST::IdentifierExpression *ast);
    virtual bool visit(QmlJS::AST::UiArrayBinding *ast);
    virtual bool visit(QmlJS::AST::UiPublicMember *ast);
    virtual bool visit(QmlJS::AST::UiObjectBinding *ast);
    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);
    virtual bool visit(QmlJS::AST::UiQualifiedId *ast);
    virtual bool visit(QmlJS::AST::UiScriptBinding *ast);
    virtual bool visit(QmlJS::AST::UiSourceElement *ast);

    virtual void endVisit(QmlJS::AST::Block *);
    virtual void endVisit(QmlJS::AST::UiObjectBinding *);
    virtual void endVisit(QmlJS::AST::UiObjectDefinition *);

private:
    QmlJS::AST::Node *findDeclarationInScopes(QmlJS::AST::UiQualifiedId *ast);
    QmlJS::AST::Node *findDeclarationInScopes(QmlJS::NameId *id);

    void rememberStartPosition(QmlJS::AST::Node *node);

private:
    bool _resolveTarget;
    quint32 _pos;
    int _linkPosition;
    int _linkLength;
    int _targetLine;
    int _targetColumn;
    QStack<QmlJS::AST::Node*> _scopes;
};

} // namespace Internal
} // namespace DuiEditor

#endif // NAVIGATIONTOKENFINDER_H
