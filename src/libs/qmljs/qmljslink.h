#ifndef QMLJSLINK_H
#define QMLJSLINK_H

#include <qmljs/qmljsinterpreter.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/parser/qmljsengine_p.h>

#include <QtCore/QList>

namespace QmlJS {

class Bind;

class LinkImports
{
public:
    void operator()(const QList<Bind *> &binds);
private:
    void importObject(Bind *bind, const QString &name, Interpreter::ObjectValue *object, NameId* targetNamespace);
    void linkImports(Bind *bind, const QList<Bind *> &binds);
};

class Link
{
public:
    Interpreter::ObjectValue *operator()(const QList<Bind *> &binds, Bind *currentBind, AST::UiObjectMember *currentObject);
};

} // namespace QmlJS

#endif // QMLJSLINK_H
