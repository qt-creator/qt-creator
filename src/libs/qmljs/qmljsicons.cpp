#include "qmljsicons.h"

using namespace QmlJS;
using namespace QmlJS::AST;

namespace QmlJS {

class IconsPrivate
{
public:
    QIcon elementIcon;
    QIcon propertyIcon;
};

} // namespace QmlJS

Icons::Icons()
    : m_d(new IconsPrivate)
{
    m_d->elementIcon = QIcon(QLatin1String(":/qmljs/images/element.png"));
    m_d->propertyIcon = QIcon(QLatin1String(":/qmljs/images/property.png"));
}

Icons::~Icons()
{
    delete m_d;
}

QIcon Icons::icon(Node *node) const
{
    if (dynamic_cast<AST::UiObjectDefinition*>(node)) {
        return objectDefinitionIcon();
    }
    if (dynamic_cast<AST::UiScriptBinding*>(node)) {
        return scriptBindingIcon();
    }

    return QIcon();
}

QIcon Icons::objectDefinitionIcon() const
{
    return m_d->elementIcon;
}

QIcon Icons::scriptBindingIcon() const
{
    return m_d->propertyIcon;
}
