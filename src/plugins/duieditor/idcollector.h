#ifndef IDCOLLECTOR_H
#define IDCOLLECTOR_H

#include <QMap>
#include <QString>

#include "qmljsastvisitor_p.h"

namespace QmlJS { class NameId; }

namespace DuiEditor {
namespace Internal {

class IdCollector: protected QmlJS::AST::Visitor
{
public:
    QMap<QString, QmlJS::AST::SourceLocation> operator()(QmlJS::AST::UiProgram *ast);

protected:
    virtual bool visit(QmlJS::AST::UiScriptBinding *ast);

private:
    void addId(QmlJS::NameId* id, const QmlJS::AST::SourceLocation &idLocation);

private:
    QMap<QString, QmlJS::AST::SourceLocation> _idLocations;
};

} // namespace Internal
} // namespace DuiEditor

#endif // IDCOLLECTOR_H
