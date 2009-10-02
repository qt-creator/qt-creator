#ifndef IDCOLLECTOR_H
#define IDCOLLECTOR_H

#include <QMap>
#include <QPair>
#include <QStack>
#include <QString>

#include "qmldocument.h"
#include "qmljsastvisitor_p.h"
#include "qmlsymbol.h"

namespace QmlEditor {
namespace Internal {

class IdCollector: protected QmlJS::AST::Visitor
{
public:
    QMap<QString, QmlIdSymbol*> operator()(QmlDocument *doc);

protected:
    virtual bool visit(QmlJS::AST::UiArrayBinding *ast);
    virtual bool visit(QmlJS::AST::UiObjectBinding *ast);
    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);
    virtual bool visit(QmlJS::AST::UiScriptBinding *ast);

private:
    QmlSymbolFromFile *switchSymbol(QmlJS::AST::UiObjectMember *node);
    void addId(const QString &id, QmlJS::AST::UiScriptBinding *ast);

private:
    QmlDocument *_doc;
    QMap<QString, QmlIdSymbol*> _ids;
    QmlSymbolFromFile *_currentSymbol;
};

} // namespace Internal
} // namespace QmlEditor

#endif // IDCOLLECTOR_H
