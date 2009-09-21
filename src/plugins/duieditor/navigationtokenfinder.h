#ifndef NAVIGATIONTOKENFINDER_H
#define NAVIGATIONTOKENFINDER_H

#include <QMap>
#include <QStack>
#include <QString>
#include <QStringList>

#include "duidocument.h"
#include "qmljsastvisitor_p.h"

namespace QmlJS {
    class NameId;
} // namespace QmlJS

namespace DuiEditor {
namespace Internal {

class NavigationTokenFinder: protected QmlJS::AST::Visitor
{
public:
    void operator()(const DuiDocument::Ptr &doc, int position, const Snapshot &snapshot);

    bool targetFound() const { return _targetLine != -1; }
    bool linkFound() const { return _linkPosition != -1; }

    int linkPosition() const { return _linkPosition; }
    int linkLength() const { return _linkLength; }
    QString fileName() const { return _fileName; }
    int targetLine() const { return _targetLine; }
    int targetColumn() const { return _targetColumn; }

protected:
    virtual bool visit(QmlJS::AST::Block *ast);
    virtual bool visit(QmlJS::AST::FieldMemberExpression *ast);
    virtual bool visit(QmlJS::AST::IdentifierExpression *ast);
    virtual bool visit(QmlJS::AST::UiArrayBinding *ast);
    virtual bool visit(QmlJS::AST::UiImportList *);
    virtual bool visit(QmlJS::AST::UiPublicMember *ast);
    virtual bool visit(QmlJS::AST::UiObjectBinding *ast);
    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);
    virtual bool visit(QmlJS::AST::UiScriptBinding *ast);
    virtual bool visit(QmlJS::AST::UiSourceElement *ast);

    virtual void endVisit(QmlJS::AST::Block *);
    virtual void endVisit(QmlJS::AST::UiObjectBinding *);
    virtual void endVisit(QmlJS::AST::UiObjectDefinition *);

private:
    void checkType(QmlJS::AST::UiQualifiedId *ast);
    bool findInJS(const QStringList &qualifiedId, QmlJS::AST::Block *block);
    bool findProperty(const QStringList &qualifiedId, QmlJS::AST::UiQualifiedId *typeId, QmlJS::AST::UiObjectMemberList *ast, int scopeLevel);
    void findAsId(const QStringList &qualifiedId);
    void findDeclaration(const QStringList &qualifiedId, int scopeLevel);
    void findDeclaration(const QStringList &id);
    void findTypeDeclaration(const QStringList &id);
    void rememberLocation(const QmlJS::AST::SourceLocation &loc);
    DuiDocument::Ptr findCustomType(const QStringList& qualifiedId) const;

private:
    quint32 _pos;
    DuiDocument::Ptr _doc;
    Snapshot _snapshot;
    int _linkPosition;
    int _linkLength;
    QString _fileName;
    int _targetLine;
    int _targetColumn;
    QStack<QmlJS::AST::Node*> _scopes;
};

} // namespace Internal
} // namespace DuiEditor

#endif // NAVIGATIONTOKENFINDER_H
