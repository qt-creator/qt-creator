/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef QQMLDIRPARSER_P_H
#define QQMLDIRPARSER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QUrl>
#include <QtCore/QHash>
#include <QtCore/QDebug>


#include "qmljsglobal_p.h"

QT_BEGIN_NAMESPACE

class QmlError;
class QmlEngine;
class QML_PARSER_EXPORT QmlDirParser
{
    Q_DISABLE_COPY(QmlDirParser)

public:
    QmlDirParser();
    ~QmlDirParser();

    bool parse(const QString &source);

    bool hasError() const;
    void setError(const QmlError &);
    QList<QmlError> errors(const QString &uri) const;

    QString typeNamespace() const;
    void setTypeNamespace(const QString &s);

    struct Plugin
    {
        Plugin() {}

        Plugin(const QString &name, const QString &path)
            : name(name), path(path) {}

        QString name;
        QString path;
    };

    struct Component
    {
        Component()
            : majorVersion(0), minorVersion(0), internal(false) {}

        Component(const QString &typeName, const QString &fileName, int majorVersion, int minorVersion)
            : typeName(typeName), fileName(fileName), majorVersion(majorVersion), minorVersion(minorVersion),
            internal(false) {}

        QString typeName;
        QString fileName;
        int majorVersion;
        int minorVersion;
        bool internal;
    };

    struct Script
    {
        Script()
            : majorVersion(0), minorVersion(0) {}

        Script(const QString &nameSpace, const QString &fileName, int majorVersion, int minorVersion)
            : nameSpace(nameSpace), fileName(fileName), majorVersion(majorVersion), minorVersion(minorVersion) {}

        QString nameSpace;
        QString fileName;
        int majorVersion;
        int minorVersion;
    };

    QHash<QString,Component> components() const;
    QList<Script> scripts() const;
    QList<Plugin> plugins() const;

#ifdef QT_CREATOR
    struct TypeInfo
    {
        TypeInfo() {}
        TypeInfo(const QString &fileName)
            : fileName(fileName) {}

        QString fileName;
    };

    QList<TypeInfo> typeInfos() const;
#endif

private:
    void reportError(int line, int column, const QString &message);

private:
    QList<QmlError> _errors;
    QString _typeNamespace;
    QHash<QString,Component> _components; // multi hash
    QList<Script> _scripts;
    QList<Plugin> _plugins;
#ifdef QT_CREATOR
    QList<TypeInfo> _typeInfos;
#endif
};

typedef QHash<QString,QmlDirParser::Component> QmlDirComponents;
typedef QList<QmlDirParser::Script> QmlDirScripts;
typedef QList<QmlDirParser::Plugin> QmlDirPlugins;

QDebug &operator<< (QDebug &, const QmlDirParser::Component &);
QDebug &operator<< (QDebug &, const QmlDirParser::Script &);

QT_END_NAMESPACE

#endif // QQMLDIRPARSER_P_H
