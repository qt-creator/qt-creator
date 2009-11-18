/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef WINSCWTOOLCHAIN_H
#define WINSCWTOOLCHAIN_H

#include "s60devices.h"

#include <projectexplorer/project.h>
#include <projectexplorer/toolchain.h>

namespace Qt4ProjectManager {
namespace Internal {

class WINSCWToolChain : public ProjectExplorer::ToolChain
{
public:
    WINSCWToolChain(S60Devices::Device device, const QString &mwcDirectory);
    QByteArray predefinedMacros();
    QList<ProjectExplorer::HeaderPath> systemHeaderPaths();
    void addToEnvironment(ProjectExplorer::Environment &env);
    ProjectExplorer::ToolChain::ToolChainType type() const;
    QString makeCommand() const;

protected:
    bool equals(ToolChain *other) const;

private:
    QStringList systemIncludes() const;

    const S60ToolChainMixin m_mixin;

    QString m_carbidePath;
    QString m_deviceId;
    QString m_deviceName;
    QString m_deviceRoot;
    QList<ProjectExplorer::HeaderPath> m_systemHeaderPaths;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // WINSCWTOOLCHAIN_H
