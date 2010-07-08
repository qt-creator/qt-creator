#ifndef QMLJSICONS_H
#define QMLJSICONS_H

#include <qmljs/qmljs_global.h>
#include <qmljs/parser/qmljsast_p.h>
#include <QtGui/QIcon>

namespace QmlJS {

class IconsPrivate;

class QMLJS_EXPORT Icons
{
public:
    Icons();
    ~Icons();

    QIcon icon(AST::Node *node) const;

    QIcon objectDefinitionIcon() const;
    QIcon scriptBindingIcon() const;

    IconsPrivate *m_d;
};

} // namespace QmlJS

#endif // QMLJSICONS_H
