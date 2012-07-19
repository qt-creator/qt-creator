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

#ifndef QMLDIRPARSER_P_H
#define QMLDIRPARSER_P_H

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

#include <QUrl>
#include <QHash>

#include "qmljsglobal_p.h"

QT_BEGIN_NAMESPACE

class QmlError;
class QML_PARSER_EXPORT QmlDirParser
{
    Q_DISABLE_COPY(QmlDirParser)

public:
    QmlDirParser();
    ~QmlDirParser();

    QUrl url() const;
    void setUrl(const QUrl &url);

    QString fileSource() const;
    void setFileSource(const QString &filePath);

    QString source() const;
    void setSource(const QString &source);

    bool isParsed() const;
    bool parse();

    bool hasError() const;
    QList<QmlError> errors(const QString &uri) const;

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

    QList<Component> components() const;
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
    QUrl _url;
    QString _source;
    QString _filePathSouce;
    QList<Component> _components;
    QList<Plugin> _plugins;
#ifdef QT_CREATOR
    QList<TypeInfo> _typeInfos;
#endif
    unsigned _isParsed: 1;
};

typedef QList<QmlDirParser::Component> QmlDirComponents;


QT_END_NAMESPACE

#endif // QMLDIRPARSER_P_H
