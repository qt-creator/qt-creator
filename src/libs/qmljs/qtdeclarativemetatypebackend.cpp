/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qtdeclarativemetatypebackend.h"

#include <QDebug>

namespace QmlJS {
namespace Internal {

class DeclarativeSymbol: public PrimitiveSymbol
{
public:
    virtual ~DeclarativeSymbol()
    {}

protected:
    DeclarativeSymbol(QtDeclarativeMetaTypeBackend* backend):
            m_backend(backend)
    { Q_ASSERT(backend); }
    
    QtDeclarativeMetaTypeBackend* backend() const
    { return m_backend; }

private:
    QtDeclarativeMetaTypeBackend* m_backend;
};

class DeclarativeObjectSymbol: public DeclarativeSymbol
{
    DeclarativeObjectSymbol(const DeclarativeObjectSymbol &);
    DeclarativeObjectSymbol &operator=(const DeclarativeObjectSymbol &);

public:
    DeclarativeObjectSymbol(QtDeclarativeMetaTypeBackend* backend):
            DeclarativeSymbol(backend)
    {
    }

    virtual ~DeclarativeObjectSymbol()
    { qDeleteAll(m_members); }

    virtual const QString name() const
    { return m_name; }

    virtual PrimitiveSymbol *type() const
    { return 0; }

    virtual const List members()
    {
        return m_members;
    }

    virtual List members(bool includeBaseClassMembers)
    {
        List result = members();
        return result;
    }

    virtual bool isProperty() const
    { return false; }

public:
    static QString key(const QString &typeNameWithPackage, int majorVersion, int minorVersion)
    {
        return QString(typeNameWithPackage)
                + QLatin1Char('@')
                + QString::number(majorVersion)
                + QLatin1Char('.')
                + QString::number(minorVersion);
    }

    static QString key(const QString &packageName, const QString &typeName, int majorVersion, int minorVersion)
    {
        return packageName
                + QLatin1Char('/')
                + typeName
                + QLatin1Char('@')
                + QString::number(majorVersion)
                + QLatin1Char('.')
                + QString::number(minorVersion);
    }

private:
    QString m_name;

    bool m_membersToBeDone;
    List m_members;
};

class DeclarativePropertySymbol: public DeclarativeSymbol
{
    DeclarativePropertySymbol(const DeclarativePropertySymbol &);
    DeclarativePropertySymbol &operator=(const DeclarativePropertySymbol &);

public:
    DeclarativePropertySymbol(QtDeclarativeMetaTypeBackend* backend):
            DeclarativeSymbol(backend)
    {
    }

    virtual ~DeclarativePropertySymbol()
    {}

    virtual const QString name() const
    { return QString(); }

    virtual PrimitiveSymbol *type() const
    { return 0; }

    virtual const List members()
    {
        return List();
    }

    virtual List members(bool /*includeBaseClassMembers*/)
    {
        return members();
    }

    virtual bool isProperty() const
    { return true; }

private:
};
    
} // namespace Internal
} // namespace Qml

using namespace QmlJS;
using namespace QmlJS::Internal;

QtDeclarativeMetaTypeBackend::QtDeclarativeMetaTypeBackend(TypeSystem *typeSystem):
        MetaTypeBackend(typeSystem)
{
}

QtDeclarativeMetaTypeBackend::~QtDeclarativeMetaTypeBackend()
{
}

QList<Symbol *> QtDeclarativeMetaTypeBackend::availableTypes(const QString &package, int majorVersion, int minorVersion)
{
    QList<Symbol *> result;

    return result;
}

Symbol *QtDeclarativeMetaTypeBackend::resolve(const QString &typeName, const QList<PackageInfo> &packages)
{
    QList<Symbol *> result;


    return 0;
}
