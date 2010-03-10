/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef MAEMOTOOLCHAIN_H
#define MAEMOTOOLCHAIN_H

#include <projectexplorer/toolchain.h>

namespace Qt4ProjectManager {
    class QtVersion;
    namespace Internal {

#ifdef Q_OS_WIN32
#define EXEC_SUFFIX ".exe"
#else
#define EXEC_SUFFIX ""
#endif

class MaemoToolChain : public ProjectExplorer::GccToolChain
{
public:
    MaemoToolChain(const QString &targetRoot);
    virtual ~MaemoToolChain();

    void addToEnvironment(ProjectExplorer::Environment &env);
    ProjectExplorer::ToolChain::ToolChainType type() const;
    QString makeCommand() const;

    QString maddeRoot() const;
    QString targetRoot() const;
    QString sysrootRoot() const;
    QString simulatorRoot() const;

protected:
    bool equals(ToolChain *other) const;

private:
    void setMaddeRoot() const;
    void setSimulatorRoot() const;
    void setSysroot() const;

private:
    mutable QString m_maddeRoot;
    mutable bool m_maddeInitialized;

    mutable QString m_sysrootRoot;
    mutable bool m_sysrootInitialized;

    mutable QString m_simulatorRoot;
    mutable bool m_simulatorInitialized;

    const QString m_targetRoot;
};

    } // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOTOOLCHAIN_H
