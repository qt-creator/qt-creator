#ifndef QMLSYMBOL_H
#define QMLSYMBOL_H

#include <QList>
#include <QString>

#include "qmljsastfwd_p.h"

namespace QmlEditor {

class QmlSymbol
{
public:
    typedef QList<QmlSymbol*> List;

public:
    virtual ~QmlSymbol();

    virtual const QString name() const = 0;
    virtual const List members();

    bool isBuildInSymbol();
    bool isSymbolFromFile();
    bool isIdSymbol();
    bool isPropertyDefinitionSymbol();

    virtual class QmlBuildInSymbol *asBuildInSymbol();
    virtual class QmlSymbolFromFile *asSymbolFromFile();
    virtual class QmlIdSymbol *asIdSymbol();
    virtual class QmlPropertyDefinitionSymbol *asPropertyDefinitionSymbol();

protected:
    List _members;
};

class QmlBuildInSymbol: public QmlSymbol
{
public:
    QmlBuildInSymbol(const QString &name): _name(name) {}
    virtual ~QmlBuildInSymbol();

    virtual QmlBuildInSymbol *asBuildInSymbol();

    virtual const QString name() const
    { return _name; }

    // TODO:
//    virtual const List members();

private:
    QString _name;
};

class QmlSymbolFromFile: public QmlSymbol
{
public:
    QmlSymbolFromFile(const QString &fileName, QmlJS::AST::UiObjectMember *node);
    virtual ~QmlSymbolFromFile();

    virtual QmlSymbolFromFile *asSymbolFromFile();

    QString fileName() const
    { return _fileName; }

    virtual int line() const;
    virtual int column() const;

    QmlJS::AST::UiObjectMember *node() const
    { return _node; }

    virtual const QString name() const;
    virtual const List members();
    virtual QmlSymbolFromFile *findMember(QmlJS::AST::Node *memberNode);

private:
    void fillTodo(QmlJS::AST::UiObjectMemberList *members);

private:
    QString _fileName;
    QmlJS::AST::UiObjectMember *_node;
    QList<QmlJS::AST::Node*> todo;
};

class QmlIdSymbol: public QmlSymbolFromFile
{
public:
    QmlIdSymbol(const QString &fileName, QmlJS::AST::UiScriptBinding *idNode, QmlSymbolFromFile *parentNode);
    virtual ~QmlIdSymbol();

    QmlIdSymbol *asIdSymbol();

    virtual int line() const;
    virtual int column() const;

    QmlSymbolFromFile *parentNode() const
    { return _parentNode; }

    virtual const QString name() const
    { return "id"; }

    virtual const QString id() const;

private:
    QmlJS::AST::UiScriptBinding *idNode() const;

private:
    QmlSymbolFromFile *_parentNode;
};

class QmlPropertyDefinitionSymbol: public QmlSymbolFromFile
{
public:
    QmlPropertyDefinitionSymbol(const QString &fileName, QmlJS::AST::UiPublicMember *propertyNode);
    virtual ~QmlPropertyDefinitionSymbol();

    QmlPropertyDefinitionSymbol *asPropertyDefinitionSymbol();

    virtual int line() const;
    virtual int column() const;

    virtual const QString name() const;

private:
    QmlJS::AST::UiPublicMember *propertyNode() const;
};

} // namespace QmlEditor

#endif // QMLSYMBOL_H
