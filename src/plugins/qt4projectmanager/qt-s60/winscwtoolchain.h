/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef WINSCWTOOLCHAIN_H
#define WINSCWTOOLCHAIN_H

#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainconfigwidget.h>

namespace Qt4ProjectManager {
namespace Internal {
namespace Ui {
class WinscwToolChainConfigWidget;
} // namespace Ui

// --------------------------------------------------------------------------
// WinscwToolChain
// --------------------------------------------------------------------------

class WinscwToolChain : public ProjectExplorer::ToolChain
{
public:
    WinscwToolChain(const WinscwToolChain &);
    ~WinscwToolChain();

    QString typeName() const;
    ProjectExplorer::Abi targetAbi() const;

    bool isValid() const;

    QByteArray predefinedMacros() const;
    QList<ProjectExplorer::HeaderPath> systemHeaderPaths() const;
    void addToEnvironment(Utils::Environment &env) const;
    Utils::FileName mkspec() const;
    QString makeCommand() const;
    virtual QString debuggerCommand() const;
    QString defaultMakeTarget() const;
    ProjectExplorer::IOutputParser *outputParser() const;

    bool operator ==(const ProjectExplorer::ToolChain &) const;

    ProjectExplorer::ToolChainConfigWidget *configurationWidget();
    ProjectExplorer::ToolChain *clone() const;

    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &data);

    void setSystemIncludePathes(const QStringList &);
    QStringList systemIncludePathes() const;

    void setSystemLibraryPathes(const QStringList &);
    QStringList systemLibraryPathes() const;

    void setCompilerPath(const QString &);
    QString compilerPath() const;

private:
    void updateId();

    explicit WinscwToolChain(bool);

    QStringList m_systemIncludePathes;
    QStringList m_systemLibraryPathes;
    QString m_compilerPath;

    friend class WinscwToolChainFactory;
};

// --------------------------------------------------------------------------
// WinscwToolChainConfigWidget
// --------------------------------------------------------------------------

class WinscwToolChainConfigWidget : public ProjectExplorer::ToolChainConfigWidget
{
    Q_OBJECT

public:
    WinscwToolChainConfigWidget(WinscwToolChain *);
    ~WinscwToolChainConfigWidget();

    void apply();
    void discard();
    bool isDirty() const;

private slots:
    void handleCompilerPathUpdate();
    void makeDirty();

private:
    Ui::WinscwToolChainConfigWidget *m_ui;
};

// --------------------------------------------------------------------------
// WinscwToolChainFactory
// --------------------------------------------------------------------------

class WinscwToolChainFactory : public ProjectExplorer::ToolChainFactory
{
    Q_OBJECT

public:
    WinscwToolChainFactory();

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

#endif // WINSCWTOOLCHAIN_H
