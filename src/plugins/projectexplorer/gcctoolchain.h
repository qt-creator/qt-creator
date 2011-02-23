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

#ifndef GCCTOOLCHAIN_H
#define GCCTOOLCHAIN_H

#include "projectexplorer_export.h"

#include "toolchain.h"

#include "toolchainconfigwidget.h"

#include <utils/pathchooser.h>

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace ProjectExplorer {

namespace Internal {
class GccToolChainFactory;
class MingwToolChainFactory;
class LinuxIccToolChainFactory;
}

// --------------------------------------------------------------------------
// GccToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT GccToolChain : public ToolChain
{
public:
    QString typeName() const;
    Abi targetAbi() const;

    bool isValid() const;

    QByteArray predefinedMacros() const;
    QList<HeaderPath> systemHeaderPaths() const;
    void addToEnvironment(Utils::Environment &env) const;
    QString makeCommand() const;
    void setDebuggerCommand(const QString &);
    QString debuggerCommand() const;
    IOutputParser *outputParser() const;

    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &data);

    ToolChainConfigWidget *configurationWidget();

    bool operator ==(const ToolChain &) const;

    void setCompilerPath(const QString &);
    QString compilerPath() const;

    bool isForcedTo32Bit() const;
    void forceTo32Bit(bool);

    bool supports64Bit() const;

    ToolChain *clone() const;

protected:
    GccToolChain(const QString &id, bool autodetect);
    GccToolChain(const GccToolChain &);

    QString defaultDisplayName() const;

    void updateId();

    mutable QByteArray m_predefinedMacros;

private:
    GccToolChain(bool autodetect);

    QString m_compilerPath;
    QString m_debuggerCommand;
    bool m_forcedTo32Bit;
    mutable bool m_supports64Bit;

    mutable Abi m_targetAbi;
    mutable QList<HeaderPath> m_headerPathes;

    friend class Internal::GccToolChainFactory;
    friend class ToolChainFactory;
};


// --------------------------------------------------------------------------
// GccToolChainFactory
// --------------------------------------------------------------------------

namespace Internal {

class GccToolChainFactory : public ToolChainFactory
{
    Q_OBJECT

public:
    // Name used to display the name of the toolchain that will be created.
    QString displayName() const;
    QString id() const;

    QList<ToolChain *> autoDetect();

    bool canCreate();
    ToolChain *create();

    // Used by the ToolChainManager to restore user-generated ToolChains
    bool canRestore(const QVariantMap &data);
    ToolChain *restore(const QVariantMap &data);

protected:
    virtual GccToolChain *createToolChain(bool autoDetect);
    QList<ToolChain *> autoDetectToolchains(const QString &compiler,
                                            const QStringList &debuggers,
                                            const Abi &);
};

} // namespace Internal

// --------------------------------------------------------------------------
// GccToolChainConfigWidget
// --------------------------------------------------------------------------

namespace Internal {

class GccToolChainConfigWidget : public ToolChainConfigWidget
{
    Q_OBJECT

public:
    GccToolChainConfigWidget(GccToolChain *);
    void apply();
    void discard() { setFromToolchain(); }
    bool isDirty() const;

private slots:
    void handlePathChange();
    void handle32BitChange();

private:
    void setFromToolchain();

    Utils::PathChooser *m_compilerPath;
    QCheckBox *m_force32BitCheckBox;
};

} // namespace Internal

// --------------------------------------------------------------------------
// MingwToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT MingwToolChain : public GccToolChain
{
public:
    QString typeName() const;
    QString makeCommand() const;

    ToolChain *clone() const;

private:
    MingwToolChain(bool autodetect);

    friend class Internal::MingwToolChainFactory;
    friend class ToolChainFactory;
};

// --------------------------------------------------------------------------
// MingwToolChainFactory
// --------------------------------------------------------------------------

namespace Internal {

class MingwToolChainFactory : public GccToolChainFactory
{
    Q_OBJECT

public:
    // Name used to display the name of the toolchain that will be created.
    QString displayName() const;
    QString id() const;

    QList<ToolChain *> autoDetect();

    bool canCreate();
    ToolChain *create();

    // Used by the ToolChainManager to restore user-generated ToolChains
    bool canRestore(const QVariantMap &data);
    ToolChain *restore(const QVariantMap &data);

protected:
    GccToolChain *createToolChain(bool autoDetect);
};

} // namespace Internal

// --------------------------------------------------------------------------
// LinuxIccToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT LinuxIccToolChain : public GccToolChain
{
public:

    QString typeName() const;

    IOutputParser *outputParser() const;

    ToolChain *clone() const;

private:
    LinuxIccToolChain(bool autodetect);

    friend class Internal::LinuxIccToolChainFactory;
    friend class ToolChainFactory;
};

// --------------------------------------------------------------------------
// LinuxIccToolChainFactory
// --------------------------------------------------------------------------

namespace Internal {

class LinuxIccToolChainFactory : public GccToolChainFactory
{
    Q_OBJECT

public:
    // Name used to display the name of the toolchain that will be created.
    QString displayName() const;
    QString id() const;

    QList<ToolChain *> autoDetect();

    ToolChain *create();

    // Used by the ToolChainManager to restore user-generated ToolChains
    bool canRestore(const QVariantMap &data);
    ToolChain *restore(const QVariantMap &data);

protected:
    GccToolChain *createToolChain(bool autoDetect);
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // GCCTOOLCHAIN_H
