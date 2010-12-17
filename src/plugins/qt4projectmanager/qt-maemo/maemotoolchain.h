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

#ifndef MAEMOTOOLCHAIN_H
#define MAEMOTOOLCHAIN_H

#include <projectexplorer/toolchain.h>

namespace Qt4ProjectManager {
    class QtVersion;
    namespace Internal {

class MaemoToolChain : public ProjectExplorer::GccToolChain
{
public:
    MaemoToolChain(const QString &targetRoot);
    virtual ~MaemoToolChain();

    void addToEnvironment(Utils::Environment &env);
    ProjectExplorer::ToolChainType type() const;
    QString makeCommand() const;

    QString maddeRoot() const;
    QString targetRoot() const;
    QString targetName() const;
    QString sysrootRoot() const;
    QString madAdminCommand() const;

    enum MaemoVersion { Maemo5, Maemo6 };
    MaemoVersion version() const;
    bool allowsRemoteMounts() const { return version() == Maemo5; }
    bool allowsPackagingDisabling() const { return version() == Maemo5; }
    bool allowsQmlDebugging() const { return version() == Maemo6; }

protected:
    bool equals(const ToolChain *other) const;

private:
    void setMaddeRoot() const;
    void setSysroot() const;

private:
    mutable QString m_maddeRoot;
    mutable bool m_maddeInitialized;

    mutable QString m_sysrootRoot;
    mutable bool m_sysrootInitialized;

    const QString m_targetRoot;
};

    } // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOTOOLCHAIN_H
