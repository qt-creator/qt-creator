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

#include "maemotoolchain.h"
#include "qtversionmanager.h"

#include <QtCore/QDir>
#include <QtCore/QtDebug>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

#ifdef Q_OS_WIN32
#define EXEC_SUFFIX ".exe"
#else
#define EXEC_SUFFIX ""
#endif

namespace {
    const char *GCC_MAEMO_COMMAND = "arm-none-linux-gnueabi-gcc" EXEC_SUFFIX;
}

MaemoToolChain::MaemoToolChain(const Qt4ProjectManager::QtVersion *version)
    : GccToolChain(QLatin1String(GCC_MAEMO_COMMAND))
    , m_maddeInitialized(false)
    , m_sysrootInitialized(false)
    , m_simulatorInitialized(false)
    , m_toolchainInitialized(false)
{
    QString qmake = QDir::cleanPath(version->qmakeCommand());
    m_targetRoot = qmake.remove(QLatin1String("/bin/qmake" EXEC_SUFFIX));
}

MaemoToolChain::~MaemoToolChain()
{
}

ToolChain::ToolChainType MaemoToolChain::type() const
{
    return ToolChain::GCC_MAEMO;
}

QList<HeaderPath> MaemoToolChain::systemHeaderPaths()
{
    if (m_systemHeaderPaths.isEmpty()) {
        GccToolChain::systemHeaderPaths();
        m_systemHeaderPaths
            .append(HeaderPath(QString("%1/usr/include").arg(sysrootRoot()),
                HeaderPath::GlobalHeaderPath));
    }
    return m_systemHeaderPaths;
}

void MaemoToolChain::addToEnvironment(ProjectExplorer::Environment &env)
{
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/bin")
        .arg(maddeRoot())));
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/bin")
        .arg(targetRoot())));
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/bin")
        .arg(toolchainRoot())));
#ifdef Q_OS_WIN
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/madbin")
        .arg(maddeRoot())));
#endif
}

QString MaemoToolChain::makeCommand() const
{
    return QLatin1String("make" EXEC_SUFFIX);
}

bool MaemoToolChain::equals(ToolChain *other) const
{
    MaemoToolChain *toolChain = static_cast<MaemoToolChain*> (other);
    return (other->type() == type()
        && toolChain->sysrootRoot() == sysrootRoot()
        && toolChain->simulatorRoot() == simulatorRoot()
        && toolChain->targetRoot() == targetRoot()
        && toolChain->toolchainRoot() == toolchainRoot());
}

QString MaemoToolChain::maddeRoot() const
{
    if (!m_maddeInitialized)
        (const_cast<MaemoToolChain*> (this))->setMaddeRoot();
    return m_maddeRoot;
}

QString MaemoToolChain::targetRoot() const
{
    return m_targetRoot;
}

QString MaemoToolChain::sysrootRoot() const
{
    if (!m_sysrootInitialized)
        (const_cast<MaemoToolChain*> (this))->setSysrootAndToolchain();
    return m_sysrootRoot;
}

QString MaemoToolChain::simulatorRoot() const
{
    if (!m_simulatorInitialized)
        (const_cast<MaemoToolChain*> (this))->setSimulatorRoot();
    return m_simulatorRoot;
}

QString MaemoToolChain::toolchainRoot() const
{
    if (!m_toolchainInitialized)
        (const_cast<MaemoToolChain*> (this))->setSysrootAndToolchain();
    return m_toolchainRoot;
}

void MaemoToolChain::setMaddeRoot()
{
    QDir dir(targetRoot());
    dir.cdUp(); dir.cdUp();

    m_maddeInitialized = true;
    m_maddeRoot = dir.absolutePath();
}

void MaemoToolChain::setSimulatorRoot()
{
    QString target = QDir::cleanPath(targetRoot());
    target = target.mid(target.lastIndexOf(QLatin1Char('/')) + 1);

    QFile file(maddeRoot() + QLatin1String("/cache/madde.conf"));
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            QString line = stream.readLine().trimmed();
            if (!line.startsWith(QLatin1String("target")))
                continue;

            const QStringList &list = line.split(QLatin1Char(' '));
            if (list.count() <= 1 || list.at(1) != target)
                continue;

            line = stream.readLine().trimmed();
            while (!stream.atEnd() && line != QLatin1String("end")) {
                if (line.startsWith(QLatin1String("runtime"))) {
                    const QStringList &list = line.split(QLatin1Char(' '));
                    if (list.count() > 1) {
                        m_simulatorRoot = maddeRoot()
                            + QLatin1String("/runtimes/") + list.at(1).trimmed();
                    }
                    break;
                }
                line = stream.readLine().trimmed();
            }
        }
    }

    m_simulatorInitialized = true;
}

void MaemoToolChain::setSysrootAndToolchain()
{
    QFile file(QDir::cleanPath(targetRoot()) + QLatin1String("/information"));
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            const QString &line = stream.readLine().trimmed();
            const QStringList &list = line.split(QLatin1Char(' '));
            if (list.count() <= 1)
                continue;
            if (list.at(0) == QLatin1String("sysroot")) {
                m_sysrootRoot = maddeRoot() + QLatin1String("/sysroots/")
                    + list.at(1);
            }
            if (list.at(0) == QLatin1String("toolchain")) {
                m_toolchainRoot = maddeRoot() + QLatin1String("/toolchains/")
                    + list.at(1);
            }
        }
    }

    m_sysrootInitialized = true;
    m_toolchainInitialized = true;
}
