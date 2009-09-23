#include "qmljsast_p.h"
#include "qmlsymbol.h"

using namespace DuiEditor;
using namespace QmlJS;
using namespace QmlJS::AST;

QmlSymbol::~QmlSymbol()
{
}

bool QmlSymbol::isBuildInSymbol() const
{ return asBuildInSymbol() != 0; }

bool QmlSymbol::isSymbolFromFile() const
{ return asSymbolFromFile() != 0; }

bool QmlSymbol::isIdSymbol() const
{ return asIdSymbol() != 0; }

QmlBuildInSymbol const *QmlSymbol::asBuildInSymbol() const
{ return 0; }

QmlSymbolFromFile const *QmlSymbol::asSymbolFromFile() const
{ return 0; }

QmlIdSymbol const *QmlSymbol::asIdSymbol() const
{ return 0; }

QmlBuildInSymbol::~QmlBuildInSymbol()
{}

QmlBuildInSymbol const* QmlBuildInSymbol::asBuildInSymbol() const
{ return this; }

QmlSymbolFromFile::QmlSymbolFromFile(const QString &fileName, QmlJS::AST::UiObjectMember *node):
        _fileName(fileName),
        _node(node)
{}

QmlSymbolFromFile::~QmlSymbolFromFile()
{}

const QmlSymbolFromFile *QmlSymbolFromFile::asSymbolFromFile() const
{ return this; }

int QmlSymbolFromFile::line() const
{ return _node->firstSourceLocation().startLine; }

int QmlSymbolFromFile::column() const
{ return _node->firstSourceLocation().startColumn; }

QmlIdSymbol::QmlIdSymbol(const QString &fileName, QmlJS::AST::UiScriptBinding *idNode, const QmlSymbolFromFile &parentNode):
        QmlSymbolFromFile(fileName, idNode),
        _parentNode(parentNode)
{}

QmlIdSymbol::~QmlIdSymbol()
{}

QmlIdSymbol const *QmlIdSymbol::asIdSymbol() const
{ return this; }

int QmlIdSymbol::line() const
{ return idNode()->statement->firstSourceLocation().startLine; }

int QmlIdSymbol::column() const
{ return idNode()->statement->firstSourceLocation().startColumn; }

QmlJS::AST::UiScriptBinding *QmlIdSymbol::idNode() const
{ return cast<UiScriptBinding*>(node()); }

QmlPropertyDefinitionSymbol::QmlPropertyDefinitionSymbol(const QString &fileName, QmlJS::AST::UiPublicMember *propertyNode):
        QmlSymbolFromFile(fileName, propertyNode)
{}

QmlPropertyDefinitionSymbol::~QmlPropertyDefinitionSymbol()
{}

int QmlPropertyDefinitionSymbol::line() const
{ return propertyNode()->identifierToken.startLine; }

int QmlPropertyDefinitionSymbol::column() const
{ return propertyNode()->identifierToken.startColumn; }

QmlJS::AST::UiPublicMember *QmlPropertyDefinitionSymbol::propertyNode() const
{ return cast<UiPublicMember*>(node()); }
