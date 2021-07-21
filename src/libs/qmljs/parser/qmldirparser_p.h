/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

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

#include <languageutils/componentversion.h>

#include "qmljs/parser/qmljsglobal_p.h"
#include "qmljs/parser/qmljsengine_p.h"
#include "qmljs/parser/qmljsdiagnosticmessage_p.h"

QT_QML_BEGIN_NAMESPACE

class QmlEngine;
class QML_PARSER_EXPORT QmlDirParser
{
public:
    void clear();
    bool parse(const QString &source);

    bool hasError() const { return !_errors.isEmpty(); }
    void setError(const QmlJS::DiagnosticMessage &);
    QList<QmlJS::DiagnosticMessage> errors(const QString &uri) const;

    QString typeNamespace() const { return _typeNamespace; }
    void setTypeNamespace(const QString &s) { _typeNamespace = s; }

    static void checkNonRelative(const char *item, const QString &typeName, const QString &fileName)
    {
        if (fileName.startsWith(QLatin1Char('/'))) {
            qWarning() << item << typeName
                       << "is specified with non-relative URL" << fileName << "in a qmldir file."
                       << "URLs in qmldir files should be relative to the qmldir file's directory.";
        }
    }

    struct Plugin
    {
        Plugin() = default;

        Plugin(const QString &name, const QString &path, bool optional)
            : name(name), path(path), optional(optional)
        {
            checkNonRelative("Plugin", name, path);
        }

        QString name;
        QString path;
        bool optional = false;
    };

    struct Component
    {
        Component() = default;

        Component(const QString &typeName, const QString &fileName, int majorVersion, int minorVersion)
            : typeName(typeName), fileName(fileName), majorVersion(majorVersion), minorVersion(minorVersion),
            internal(false), singleton(false)
        {
            checkNonRelative("Component", typeName, fileName);
        }

        QString typeName;
        QString fileName;
        int majorVersion = 0;
        int minorVersion = 0;
        bool internal = false;
        bool singleton = false;
    };

    struct Script
    {
        Script() = default;

        Script(const QString &nameSpace, const QString &fileName, int majorVersion, int minorVersion)
            : nameSpace(nameSpace), fileName(fileName), majorVersion(majorVersion), minorVersion(minorVersion)
        {
            checkNonRelative("Script", nameSpace, fileName);
        }

        QString nameSpace;
        QString fileName;
        int majorVersion = 0;
        int minorVersion = 0;
    };

    struct Import
    {
        enum Flag {
            Default  = 0x0,
            Auto     = 0x1, // forward the version of the importing module
            Optional = 0x2  // is not automatically imported but only a tooling hint
        };
        Q_DECLARE_FLAGS(Flags, Flag)

        Import() = default;
        Import(QString module, LanguageUtils::ComponentVersion version, Flags flags)
            : module(module), version(version), flags(flags)
        {
        }

        QString module;
        LanguageUtils::ComponentVersion version;     // invalid version is latest version, unless Flag::Auto
        Flags flags;
    };

    QMultiHash<QString,Component> components() const { return _components; }
    QList<Import> dependencies() const { return _dependencies; }
    QList<Import> imports() const { return _imports; }
    QList<Script> scripts() const { return _scripts; }
    QList<Plugin> plugins() const { return _plugins; }
    bool designerSupported() const { return _designerSupported; }

    QStringList typeInfos() const { return _typeInfos; }
    QStringList classNames() const { return _classNames; }
    QString preferredPath() const { return _preferredPath; }

private:
    bool maybeAddComponent(const QString &typeName, const QString &fileName, const QString &version, QHash<QString,Component> &hash, int lineNumber = -1, bool multi = true);
    void reportError(quint16 line, quint16 column, const QString &message);

private:
    QList<QmlJS::DiagnosticMessage> _errors;
    QString _typeNamespace;
    QString _preferredPath;
    QMultiHash<QString,Component> _components;
    QList<Import> _dependencies;
    QList<Import> _imports;
    QList<Script> _scripts;
    QList<Plugin> _plugins;
    bool _designerSupported = false;
    QStringList _typeInfos;
    QStringList _classNames;
};

using QmlDirComponents = QMultiHash<QString,QmlDirParser::Component>;
using QmlDirScripts = QList<QmlDirParser::Script>;
using QmlDirPlugins = QList<QmlDirParser::Plugin>;
using QmlDirImports = QList<QmlDirParser::Import>;

QDebug &operator<< (QDebug &, const QmlDirParser::Component &);
QDebug &operator<< (QDebug &, const QmlDirParser::Script &);

QT_QML_END_NAMESPACE

