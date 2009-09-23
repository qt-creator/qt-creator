#ifndef IDCOLLECTOR_H
#define IDCOLLECTOR_H

#include <QMap>
#include <QPair>
#include <QStack>
#include <QString>

#include "qmljsastvisitor_p.h"
#include "qmlsymbol.h"

namespace DuiEditor {
namespace Internal {

class IdCollector: protected QmlJS::AST::Visitor
{
public:
    QMap<QString, QmlIdSymbol*> operator()(const QString &fileName, QmlJS::AST::UiProgram *ast);

protected:
    virtual bool visit(QmlJS::AST::UiObjectBinding *ast);
    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);
    virtual bool visit(QmlJS::AST::UiScriptBinding *ast);

    virtual void endVisit(QmlJS::AST::UiObjectBinding *);
    virtual void endVisit(QmlJS::AST::UiObjectDefinition *);

private:
    void addId(const QString &id, QmlJS::AST::UiScriptBinding *ast);

private:
    QString _fileName;
    QMap<QString, QmlIdSymbol*> _ids;
    QStack<QmlJS::AST::Node *> _scopes;
};

} // namespace Internal
} // namespace DuiEditor

#endif // IDCOLLECTOR_H
