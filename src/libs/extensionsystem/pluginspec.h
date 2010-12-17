/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef PLUGINSPEC_H
#define PLUGINSPEC_H

#include "extensionsystem_global.h"

#include <QtCore/QString>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QStringList;
QT_END_NAMESPACE

namespace ExtensionSystem {

namespace Internal {
    class PluginSpecPrivate;
    class PluginManagerPrivate;
}

class IPlugin;

struct EXTENSIONSYSTEM_EXPORT PluginDependency
{
    QString name;
    QString version;
    bool operator==(const PluginDependency &other);
};

struct EXTENSIONSYSTEM_EXPORT PluginArgumentDescription
{
    QString name;
    QString parameter;
    QString description;
};

class EXTENSIONSYSTEM_EXPORT PluginSpec
{
public:
    enum State { Invalid, Read, Resolved, Loaded, Initialized, Running, Stopped, Deleted};

    ~PluginSpec();

    // information from the xml file, valid after 'Read' state is reached
    QString name() const;
    QString version() const;
    QString compatVersion() const;
    QString vendor() const;
    QString copyright() const;
    QString license() const;
    QString description() const;
    QString url() const;
    QString category() const;
    bool isExperimental() const;
    bool isEnabled() const;
    bool isDisabledIndirectly() const;
    QList<PluginDependency> dependencies() const;

    typedef QList<PluginArgumentDescription> PluginArgumentDescriptions;
    PluginArgumentDescriptions argumentDescriptions() const;

    // other information, valid after 'Read' state is reached
    QString location() const;
    QString filePath() const;

    void setEnabled(bool value);
    void setDisabledIndirectly(bool value);

    QStringList arguments() const;
    void setArguments(const QStringList &arguments);
    void addArgument(const QString &argument);

    bool provides(const QString &pluginName, const QString &version) const;

    // dependency specs, valid after 'Resolved' state is reached
    QList<PluginSpec *> dependencySpecs() const;

    // list of plugins that depend on this - e.g. this plugins provides for them
    QList<PluginSpec *> providesForSpecs() const;

    // linked plugin instance, valid after 'Loaded' state is reached
    IPlugin *plugin() const;

    // state
    State state() const;
    bool hasError() const;
    QString errorString() const;

private:
    PluginSpec();

    Internal::PluginSpecPrivate *d;
    friend class Internal::PluginManagerPrivate;
    friend class Internal::PluginSpecPrivate;
};

} // namespace ExtensionSystem

#endif // PLUGINSPEC_H

