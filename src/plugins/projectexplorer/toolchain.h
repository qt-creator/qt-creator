/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TOOLCHAIN_H
#define TOOLCHAIN_H

#include "projectexplorer_export.h"
#include "headerpath.h"

#include <coreplugin/id.h>
#include <utils/fileutils.h>

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace Utils {
class Environment;
}

namespace ProjectExplorer {

namespace Internal {
class ToolChainPrivate;
}

class Abi;
class IOutputParser;
class ToolChainConfigWidget;
class ToolChainFactory;
class ToolChainManager;

// --------------------------------------------------------------------------
// ToolChain (documentation inside)
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ToolChain
{
public:
    virtual ~ToolChain();

    QString displayName() const;
    void setDisplayName(const QString &name);

    bool isAutoDetected() const;
    QString id() const;

    virtual QList<Utils::FileName> suggestedMkspecList() const { return QList<Utils::FileName>(); }
    virtual Utils::FileName suggestedDebugger() const;

    virtual QString type() const = 0;
    virtual QString typeDisplayName() const = 0;
    virtual Abi targetAbi() const = 0;

    virtual bool isValid() const = 0;

    virtual QByteArray predefinedMacros(const QStringList &cxxflags) const = 0;

    enum CompilerFlags {
        NO_FLAGS = 0,
        STD_CXX11 = 1
    };
    virtual CompilerFlags compilerFlags(const QStringList &cxxflags) const = 0;
    virtual QList<HeaderPath> systemHeaderPaths(const QStringList &cxxflags,
                                                const Utils::FileName &sysRoot) const = 0;
    virtual void addToEnvironment(Utils::Environment &env) const = 0;
    virtual QString makeCommand(const Utils::Environment &env) const = 0;

    virtual Utils::FileName compilerCommand() const = 0;
    virtual IOutputParser *outputParser() const = 0;

    virtual bool operator ==(const ToolChain &) const;

    virtual ToolChainConfigWidget *configurationWidget() = 0;
    virtual bool canClone() const;
    virtual ToolChain *clone() const = 0;

    // Used by the toolchainmanager to save user-generated tool chains.
    // Make sure to call this method when deriving!
    virtual QVariantMap toMap() const;

protected:
    ToolChain(const QString &id, bool autoDetect);
    explicit ToolChain(const ToolChain &);

    void toolChainUpdated();

    // Make sure to call this method when deriving!
    virtual bool fromMap(const QVariantMap &data);

private:
    void setAutoDetected(bool);

    Internal::ToolChainPrivate *const d;

    friend class ToolChainManager;
    friend class ToolChainFactory;
};

class PROJECTEXPLORER_EXPORT ToolChainFactory : public QObject
{
    Q_OBJECT

public:
    virtual QString displayName() const = 0;
    virtual QString id() const = 0;

    virtual QList<ToolChain *> autoDetect();

    virtual bool canCreate();
    virtual ToolChain *create();

    virtual bool canRestore(const QVariantMap &data);
    virtual ToolChain *restore(const QVariantMap &data);

    static QString idFromMap(const QVariantMap &data);
    static void idToMap(QVariantMap &data, const QString id);
    static void autoDetectionToMap(QVariantMap &data, bool detected);
};

} // namespace ProjectExplorer

#endif // TOOLCHAIN_H
