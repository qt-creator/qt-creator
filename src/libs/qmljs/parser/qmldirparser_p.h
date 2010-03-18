/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

#include <QtCore/QUrl>
#include <QtCore/QHash>

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

    QString source() const;
    void setSource(const QString &source);

    bool isParsed() const;
    bool parse();

    bool hasError() const;
    QList<QmlError> errors() const;

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
            : majorVersion(0), minorVersion(0) {}

        Component(const QString &typeName, const QString &fileName, int majorVersion, int minorVersion)
            : typeName(typeName), fileName(fileName), majorVersion(majorVersion), minorVersion(minorVersion) {}

        QString typeName;
        QString fileName;
        int majorVersion;
        int minorVersion;
    };

    QList<Component> components() const;
    QList<Plugin> plugins() const;

private:
    void reportError(int line, int column, const QString &message);

private:
    QList<QmlError> _errors;
    QUrl _url;
    QString _source;
    QList<Component> _components;
    QList<Plugin> _plugins;
    unsigned _isParsed: 1;
};

QT_END_NAMESPACE

#endif // QMLDIRPARSER_P_H
