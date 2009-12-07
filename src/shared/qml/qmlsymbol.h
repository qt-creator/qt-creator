#ifndef QMLSYMBOL_H
#define QMLSYMBOL_H

#include <qml/parser/qmljsastfwd_p.h>
#include <qml/qml_global.h>

#include <QList>
#include <QString>

namespace Qml {

class QML_EXPORT QmlSymbol
{
public:
    typedef QList<QmlSymbol *> List;

public:
    virtual ~QmlSymbol() = 0;

    virtual const QString name() const = 0;
    virtual const List members() = 0;

    bool isBuildInSymbol();
    bool isSymbolFromFile();
    bool isIdSymbol();
    bool isPropertyDefinitionSymbol();

    virtual class QmlBuildInSymbol *asBuildInSymbol();
    virtual class QmlSymbolFromFile *asSymbolFromFile();
    virtual class QmlIdSymbol *asIdSymbol();
    virtual class QmlPropertyDefinitionSymbol *asPropertyDefinitionSymbol();
};

class QML_EXPORT QmlBuildInSymbol: public QmlSymbol
{
public:
    virtual ~QmlBuildInSymbol() = 0;

    virtual QmlBuildInSymbol *asBuildInSymbol();

    virtual QmlBuildInSymbol *type() const = 0;
    virtual List members(bool includeBaseClassMembers) = 0;
};

class QML_EXPORT QmlPrimitiveSymbol: public QmlBuildInSymbol
{
public:
    virtual ~QmlPrimitiveSymbol() = 0;

    virtual bool isString() const = 0;
    virtual bool isNumber() const = 0;
    virtual bool isObject() const = 0;
};

class QML_EXPORT QmlSymbolWithMembers: public QmlSymbol
{
public:
    virtual ~QmlSymbolWithMembers() = 0;

    virtual const List members();

protected:
    List _members;
};

class QML_EXPORT QmlSymbolFromFile: public QmlSymbolWithMembers
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

class QML_EXPORT QmlIdSymbol: public QmlSymbolFromFile
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

class QML_EXPORT QmlPropertyDefinitionSymbol: public QmlSymbolFromFile
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

} // namespace Qml

#endif // QMLSYMBOL_H
