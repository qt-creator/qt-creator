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

#include "metainfo.h"
#include "QtDeclarativeMetaTypeBackend.h"

#include <QDebug>

namespace Qml {
namespace Internal {

class QmlDeclarativeSymbol: public QmlBuildInSymbol
{
public:
    virtual ~QmlDeclarativeSymbol()
    {}

protected:
    QmlDeclarativeSymbol(QtDeclarativeMetaTypeBackend* backend):
            m_backend(backend)
    { Q_ASSERT(backend); }
    
    QtDeclarativeMetaTypeBackend* backend() const
    { return m_backend; }

private:
    QtDeclarativeMetaTypeBackend* m_backend;
};

class QmlDeclarativeObjectSymbol: public QmlDeclarativeSymbol
{
    QmlDeclarativeObjectSymbol(const QmlDeclarativeObjectSymbol &);
    QmlDeclarativeObjectSymbol &operator=(const QmlDeclarativeObjectSymbol &);

public:
    QmlDeclarativeObjectSymbol(const NodeMetaInfo &metaInfo, QtDeclarativeMetaTypeBackend* backend):
            QmlDeclarativeSymbol(backend),
            m_metaInfo(metaInfo),
            m_membersToBeDone(true)
    {
        Q_ASSERT(metaInfo.isValid());

        m_name = m_metaInfo.typeName();
        const int slashIdx = m_name.indexOf('/');
        if (slashIdx != -1)
            m_name = m_name.mid(slashIdx + 1);
    }

    virtual ~QmlDeclarativeObjectSymbol()
    { qDeleteAll(m_members); }

    virtual const QString name() const
    { return m_name; }

    virtual QmlBuildInSymbol *type() const
    { return 0; }

    virtual const List members()
    {
        if (m_membersToBeDone)
            initMembers();

        return m_members;
    }

    virtual List members(bool includeBaseClassMembers)
    {
        List result = members();

        if (includeBaseClassMembers)
            result.append(backend()->inheritedMembers(m_metaInfo));

        return result;
    }

    virtual bool isProperty() const
    { return false; }

public:
    static QString key(const NodeMetaInfo &metaInfo)
    {
        return key(metaInfo.typeName(), metaInfo.majorVersion(), metaInfo.minorVersion());
    }

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
    void initMembers()
    {
        if (!m_membersToBeDone)
            return;
        m_membersToBeDone = false;

        m_members = backend()->members(m_metaInfo);
    }

private:
    NodeMetaInfo m_metaInfo;
    QString m_name;

    bool m_membersToBeDone;
    List m_members;
};

class QmlDeclarativePropertySymbol: public QmlDeclarativeSymbol
{
    QmlDeclarativePropertySymbol(const QmlDeclarativePropertySymbol &);
    QmlDeclarativePropertySymbol &operator=(const QmlDeclarativePropertySymbol &);

public:
    QmlDeclarativePropertySymbol(const PropertyMetaInfo &metaInfo, QtDeclarativeMetaTypeBackend* backend):
            QmlDeclarativeSymbol(backend),
            m_metaInfo(metaInfo)
    {
    }

    virtual ~QmlDeclarativePropertySymbol()
    {}

    virtual const QString name() const
    { return m_metaInfo.name(); }

    virtual QmlBuildInSymbol *type() const
    { return backend()->typeOf(m_metaInfo); }

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
    PropertyMetaInfo m_metaInfo;
};
    
} // namespace Internal
} // namespace Qml

using namespace Qml;
using namespace Qml::Internal;

QtDeclarativeMetaTypeBackend::QtDeclarativeMetaTypeBackend(QmlTypeSystem *typeSystem):
        QmlMetaTypeBackend(typeSystem)
{
    foreach (const NodeMetaInfo &metaInfo, MetaInfo::global().allTypes()) {
        m_symbols.insert(QmlDeclarativeObjectSymbol::key(metaInfo), new QmlDeclarativeObjectSymbol(metaInfo, this));
    }
}

QtDeclarativeMetaTypeBackend::~QtDeclarativeMetaTypeBackend()
{
    qDeleteAll(m_symbols.values());
}

QList<QmlSymbol *> QtDeclarativeMetaTypeBackend::availableTypes(const QString &package, int majorVersion, int minorVersion)
{
    QList<QmlSymbol *> result;
    const QString prefix = package + QLatin1Char('/');

    foreach (const NodeMetaInfo &metaInfo, MetaInfo::global().allTypes()) {
        if (metaInfo.typeName().startsWith(prefix) && metaInfo.majorVersion() == majorVersion && metaInfo.minorVersion() == minorVersion)
            result.append(getSymbol(metaInfo));
    }

    return result;
}

QmlSymbol *QtDeclarativeMetaTypeBackend::resolve(const QString &typeName, const QList<PackageInfo> &packages)
{
    QList<QmlSymbol *> result;

    foreach (const PackageInfo &package, packages) {
        if (QmlSymbol *symbol = m_symbols.value(QmlDeclarativeObjectSymbol::key(package.name(), typeName, package.majorVersion(), package.minorVersion()), 0))
            return symbol;
    }

    return 0;
}

QList<QmlSymbol *> QtDeclarativeMetaTypeBackend::members(const NodeMetaInfo &metaInfo)
{
    QList<QmlSymbol *> result;

    foreach (const PropertyMetaInfo &propertyInfo, metaInfo.properties(false).values()) {
        result.append(new QmlDeclarativePropertySymbol(propertyInfo, this));
    }

    return result;
}

QList<QmlSymbol *> QtDeclarativeMetaTypeBackend::inheritedMembers(const NodeMetaInfo &metaInfo)
{
    QList<QmlSymbol *> result;

    foreach (const NodeMetaInfo &superNode, metaInfo.directSuperClasses()) {
        result.append(getSymbol(superNode)->members(true));
    }

    return result;
}

QmlDeclarativeSymbol *QtDeclarativeMetaTypeBackend::typeOf(const PropertyMetaInfo &metaInfo)
{
    const QString key = QmlDeclarativeObjectSymbol::key(metaInfo.type(), metaInfo.typeMajorVersion(), metaInfo.typeMinorVersion());

    return m_symbols.value(key, 0);
}

QmlDeclarativeSymbol *QtDeclarativeMetaTypeBackend::getSymbol(const NodeMetaInfo &metaInfo)
{
    const QString key = QmlDeclarativeObjectSymbol::key(metaInfo);

    return m_symbols.value(key, 0);
}
