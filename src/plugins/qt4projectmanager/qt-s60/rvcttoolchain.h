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

#ifndef RVCTTOOLCHAIN_H
#define RVCTTOOLCHAIN_H

#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainconfigwidget.h>
#include <utils/environment.h>

namespace Utils {
class EnvironmentModel;
class PathChooser;
} // namespace Utils

namespace Qt4ProjectManager {
namespace Internal {

namespace Ui {
class RvctToolChainConfigWidget;
}

class RvctToolChainFactory;

// ==========================================================================
// RvctToolChain
// ==========================================================================

class RvctToolChain : public ProjectExplorer::ToolChain
{
public:
    struct RvctVersion {
        RvctVersion() : majorVersion(0), minorVersion(0), build(0)
        { }

        bool isNull() { return majorVersion == 0 && minorVersion == 0 && build == 0; }
        void reset() { majorVersion = 0; minorVersion = 0; build = 0; }

        int majorVersion;
        int minorVersion;
        int build;
    };

    static RvctVersion version(const QString &rvctPath);

    enum ArmVersion { ARMv5, ARMv6 };

    QString typeName() const;
    ProjectExplorer::Abi targetAbi() const;

    bool isValid() const;

    QByteArray predefinedMacros() const;
    QList<ProjectExplorer::HeaderPath> systemHeaderPaths() const;
    void addToEnvironment(Utils::Environment &env) const;
    QString makeCommand() const;
    QString defaultMakeTarget() const;
    ProjectExplorer::IOutputParser *outputParser() const;

    bool operator ==(const ToolChain &) const;

    void setEnvironmentChanges(const QList<Utils::EnvironmentItem> &changes);
    QList<Utils::EnvironmentItem> environmentChanges() const;

    void setCompilerPath(const QString &path);
    QString compilerPath() const;

    void setDebuggerCommand(const QString &d);
    virtual QString debuggerCommand() const;

    void setArmVersion(ArmVersion);
    ArmVersion armVersion() const;

    void setVersion(const RvctVersion &v) const;

    ProjectExplorer::ToolChainConfigWidget *configurationWidget();
    ProjectExplorer::ToolChain *clone() const;

    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &data);

private:
    void updateId();

    explicit RvctToolChain(bool autodetected = false);
    RvctToolChain(const RvctToolChain &);

    QString varName(const QString &postFix) const;

    QList<ProjectExplorer::HeaderPath> m_systemHeaderPaths;
    QString m_compilerPath;
    QList<Utils::EnvironmentItem> m_environmentChanges;
    ArmVersion m_armVersion;
    mutable RvctVersion m_version;
    QString m_debuggerCommand;

    friend class RvctToolChainFactory;
};

// ==========================================================================
// RvctToolChainConfigWidget
// ==========================================================================

class RvctToolChainConfigWidget : public ProjectExplorer::ToolChainConfigWidget
{
    Q_OBJECT

public:
    RvctToolChainConfigWidget(RvctToolChain *tc);

    void apply();
    void discard() { setFromToolChain(); }
    bool isDirty() const;

private:
    void setFromToolChain();
    QList<Utils::EnvironmentItem> environmentChanges() const;

    Ui::RvctToolChainConfigWidget *m_ui;
    Utils::EnvironmentModel *m_model;
};

// ==========================================================================
// RvctToolChainFactory
// ==========================================================================

class RvctToolChainFactory : public ProjectExplorer::ToolChainFactory
{
    Q_OBJECT

public:
    // Name used to display the name of the tool chain that will be created.
    QString displayName() const;
    QString id() const;

    QList<ProjectExplorer::ToolChain *> autoDetect();

    bool canCreate();
    ProjectExplorer::ToolChain *create();

    // Used by the ToolChainManager to restore user-generated tool chains
    bool canRestore(const QVariantMap &data);
    ProjectExplorer::ToolChain *restore(const QVariantMap &data);
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // RVCTTOOLCHAIN_H
