/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef TOOLCHAIN_H
#define TOOLCHAIN_H

#include "projectexplorer_export.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariantMap>

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
class HeaderPath;

// --------------------------------------------------------------------------
// ToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ToolChain
{
public:
    virtual ~ToolChain();

    QString displayName() const;
    void setDisplayName(const QString &name) const;

    bool isAutoDetected() const;
    QString id() const;

    virtual QString typeName() const = 0;
    virtual Abi targetAbi() const = 0;

    virtual bool isValid() const = 0;

    /// Returns a list of target ids that this ToolChain is restricted to.
    /// An empty list is shows that the toolchain is compatible with all targets.
    virtual QStringList restrictedToTargets() const;

    virtual QByteArray predefinedMacros() const = 0;
    virtual QList<HeaderPath> systemHeaderPaths() const = 0;
    virtual void addToEnvironment(Utils::Environment &env) const = 0;
    virtual QString makeCommand() const = 0;

    virtual QString debuggerCommand() const = 0;
    virtual QString defaultMakeTarget() const;
    virtual IOutputParser *outputParser() const = 0;

    virtual bool operator ==(const ToolChain &) const;

    virtual ToolChainConfigWidget *configurationWidget() = 0;
    virtual bool canClone() const;
    virtual ToolChain *clone() const = 0;

    // Used by the toolchainmanager to save user-generated ToolChains.
    // Make sure to call this method when deriving!
    virtual QVariantMap toMap() const;

protected:
    ToolChain(const QString &id, bool autoDetect);
    explicit ToolChain(const ToolChain &);

    void setId(const QString &id);

    // Make sure to call this method when deriving!
    virtual bool fromMap(const QVariantMap &data);

private:
    Internal::ToolChainPrivate *const m_d;

    friend class ToolChainFactory;
};

// --------------------------------------------------------------------------
// ToolChainFactory
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ToolChainFactory : public QObject
{
    Q_OBJECT

public:
    // Name used to display the name of the toolchain that will be created.
    virtual QString displayName() const = 0;
    virtual QString id() const = 0;

    virtual QList<ToolChain *> autoDetect();

    virtual bool canCreate();
    virtual ToolChain *create();

    // Used by the ToolChainManager to restore user-generated ToolChains
    virtual bool canRestore(const QVariantMap &data);
    virtual ToolChain *restore(const QVariantMap &data);

    static QString idFromMap(const QVariantMap &data);
};

} // namespace ProjectExplorer

#endif // TOOLCHAIN_H
