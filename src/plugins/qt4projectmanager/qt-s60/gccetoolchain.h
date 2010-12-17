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

#ifndef GCCETOOLCHAIN_H
#define GCCETOOLCHAIN_H

#include "s60devices.h"

#include <projectexplorer/toolchain.h>

namespace Qt4ProjectManager {
namespace Internal {

class GCCEToolChain : public ProjectExplorer::GccToolChain
{
    explicit GCCEToolChain(const S60Devices::Device &device,
                           const QString &gcceBinPath,
                           const QString &gcceCommand,
                           ProjectExplorer::ToolChainType type);
public:
    static GCCEToolChain *create(const S60Devices::Device &device,
                                 const QString &gcceRoot,
                                 ProjectExplorer::ToolChainType type);

    QByteArray predefinedMacros();
    virtual QList<ProjectExplorer::HeaderPath> systemHeaderPaths();
    virtual void addToEnvironment(Utils::Environment &env);
    virtual ProjectExplorer::ToolChainType type() const;
    virtual QString makeCommand() const;

protected:
    virtual bool equals(const ToolChain *other) const;

private:
    QString gcceVersion() const;
    const S60ToolChainMixin m_mixin;
    const ProjectExplorer::ToolChainType m_type;
    const QString m_gcceBinPath;
    mutable QString m_gcceVersion;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // GCCETOOLCHAIN_H
