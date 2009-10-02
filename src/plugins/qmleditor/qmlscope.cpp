#include "qmlscope.h"

using namespace QmlEditor;
using namespace QmlEditor::Internal;

Scope::Scope(Scope *parentScope):
        _parentScope(parentScope)
{
}

Scope::~Scope()
{
}

bool Scope::isJSScope()
{ return asJSScope() != 0; }

bool Scope::isQmlScope()
{ return asQmlScope() != 0; }

JSScope *Scope::asJSScope()
{ return 0; }

QmlScope *Scope::asQmlScope()
{ return 0; }

QmlScope::QmlScope(QmlScope *parentScope):
        Scope(parentScope)
{
}

JSScope::JSScope(Scope *parentScope):
        Scope(parentScope)
{
}
