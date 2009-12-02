#ifndef QMLIDCOLLECTOR_H
#define QMLIDCOLLECTOR_H

#include <QMap>
#include <QPair>
#include <QStack>
#include <QString>

#include <qml/parser/qmljsastvisitor_p.h>
#include <qml/qmldocument.h>
#include <qml/qmlsymbol.h>

namespace QmlEditor {
namespace Internal {

class QML_EXPORT QmlIdCollector: protected QmlJS::AST::Visitor
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

#endif // QMLIDCOLLECTOR_H
