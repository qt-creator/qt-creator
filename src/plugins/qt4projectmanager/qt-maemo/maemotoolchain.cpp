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

#include "maemotoolchain.h"

#include "maemoconstants.h"
#include "maemoglobal.h"
#include "qtversionmanager.h"

#include <QtCore/QDir>
#include <QtCore/QStringBuilder>
#include <QtCore/QTextStream>

using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {

MaemoToolChain::MaemoToolChain(const QtVersion *qtVersion)
    : GccToolChain(MaemoGlobal::targetRoot(qtVersion) % QLatin1String("/bin/gcc"))
    , m_sysrootInitialized(false)
    , m_qtVersionId(qtVersion->uniqueId())
{
}

MaemoToolChain::~MaemoToolChain()
{
}

ProjectExplorer::ToolChainType MaemoToolChain::type() const
{
    return ProjectExplorer::ToolChain_GCC_MAEMO;
}

void MaemoToolChain::addToEnvironment(Utils::Environment &env)
{
    QtVersion *version = QtVersionManager::instance()->version(m_qtVersionId);
    const QString maddeRoot = MaemoGlobal::maddeRoot(version);
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/bin")
        .arg(maddeRoot)));
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/bin")
        .arg(MaemoGlobal::targetRoot(version))));

    // put this into environment to make pkg-config stuff work
    env.prependOrSet(QLatin1String("SYSROOT_DIR"), sysroot());
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/madbin")
        .arg(maddeRoot)));
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/madlib")
        .arg(maddeRoot)));
    env.prependOrSet(QLatin1String("PERL5LIB"),
        QDir::toNativeSeparators(QString("%1/madlib/perl5").arg(maddeRoot)));
}

QString MaemoToolChain::makeCommand() const
{
    return QLatin1String("make" EXEC_SUFFIX);
}

bool MaemoToolChain::equals(const ToolChain *other) const
{
    const MaemoToolChain *toolChain = static_cast<const MaemoToolChain*> (other);
    return other->type() == type() && toolChain->m_qtVersionId == m_qtVersionId;
}

QString MaemoToolChain::sysroot() const
{
    if (!m_sysrootInitialized)
        setSysroot();
    return m_sysrootRoot;
}

void MaemoToolChain::setSysroot() const
{
    QtVersion *version = QtVersionManager::instance()->version(m_qtVersionId);
    QFile file(QDir::cleanPath(MaemoGlobal::targetRoot(version))
        + QLatin1String("/information"));
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            const QString &line = stream.readLine().trimmed();
            const QStringList &list = line.split(QLatin1Char(' '));
            if (list.count() <= 1)
                continue;
            if (list.at(0) == QLatin1String("sysroot")) {
                m_sysrootRoot = MaemoGlobal::maddeRoot(version)
                    + QLatin1String("/sysroots/") + list.at(1);
            }
        }
    }

    m_sysrootInitialized = true;
}

} // namespace Internal
} // namespace Qt4ProjectManager
