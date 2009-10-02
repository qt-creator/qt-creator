#ifndef QMLSCOPE_H
#define QMLSCOPE_H

namespace QmlEditor {
namespace Internal {

class QmlScope;
class JSScope;

class Scope
{
public:
    Scope(Scope *parentScope);
    virtual ~Scope();

    bool isQmlScope();
    bool isJSScope();

    virtual QmlScope *asQmlScope();
    virtual JSScope *asJSScope();

private:
    Scope *_parentScope;
};

class QmlScope: public Scope
{
public:
    QmlScope(QmlScope *parentScope);
};

class JSScope: public Scope
{
    JSScope(Scope *parentScope);
};

} // namespace Internal
} // namespace QmlEditor

#endif // QMLSCOPE_H
