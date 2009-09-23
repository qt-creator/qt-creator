#ifndef QMLSYMBOL_H
#define QMLSYMBOL_H

#include <QString>

#include "qmljsastfwd_p.h"

namespace DuiEditor {

class QmlSymbol
{
public:
    virtual ~QmlSymbol() = 0;

    bool isBuildInSymbol() const;
    bool isSymbolFromFile() const;
    bool isIdSymbol() const;

    virtual class QmlBuildInSymbol const *asBuildInSymbol() const;
    virtual class QmlSymbolFromFile const *asSymbolFromFile() const;
    virtual class QmlIdSymbol const *asIdSymbol() const;
};

class QmlBuildInSymbol: public QmlSymbol
{
public:
    virtual ~QmlBuildInSymbol();

    virtual QmlBuildInSymbol const *asBuildInSymbol() const;

private:
};

class QmlSymbolFromFile: public QmlSymbol
{
public:
    QmlSymbolFromFile(const QString &fileName, QmlJS::AST::UiObjectMember *node);
    virtual ~QmlSymbolFromFile();

    virtual QmlSymbolFromFile const *asSymbolFromFile() const;

    QString fileName() const
    { return _fileName; }

    virtual int line() const;
    virtual int column() const;

    QmlJS::AST::UiObjectMember *node() const
    { return _node; }

private:
    QString _fileName;
    QmlJS::AST::UiObjectMember *_node;
};

class QmlIdSymbol: public QmlSymbolFromFile
{
public:
    QmlIdSymbol(const QString &fileName, QmlJS::AST::UiScriptBinding *idNode, const QmlSymbolFromFile &parentNode);
    virtual ~QmlIdSymbol();

    QmlIdSymbol const *asIdSymbol() const;

    virtual int line() const;
    virtual int column() const;

    QmlSymbolFromFile const *parentNode() const
    { return &_parentNode; }

private:
    QmlJS::AST::UiScriptBinding *idNode() const;

private:
    QmlSymbolFromFile _parentNode;
};

class QmlPropertyDefinitionSymbol: public QmlSymbolFromFile
{
public:
    QmlPropertyDefinitionSymbol(const QString &fileName, QmlJS::AST::UiPublicMember *propertyNode);
    virtual ~QmlPropertyDefinitionSymbol();

    virtual int line() const;
    virtual int column() const;

private:
    QmlJS::AST::UiPublicMember *propertyNode() const;
};

} // namespace DuiEditor

#endif // QMLSYMBOL_H
