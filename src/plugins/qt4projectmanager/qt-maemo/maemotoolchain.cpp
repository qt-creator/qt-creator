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

#include "maemotoolchain.h"
#include "maemoconstants.h"

#include <QtCore/QDir>
#include <QtCore/QStringBuilder>
#include <QtCore/QTextStream>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

MaemoToolChain::MaemoToolChain(const QString &targetRoot)
    : GccToolChain(targetRoot % QLatin1String("/bin/gcc"))
    , m_maddeInitialized(false)
    , m_sysrootInitialized(false)
    , m_targetRoot(targetRoot)
{
}

MaemoToolChain::~MaemoToolChain()
{
}

ToolChain::ToolChainType MaemoToolChain::type() const
{
    return ToolChain::GCC_MAEMO;
}

void MaemoToolChain::addToEnvironment(ProjectExplorer::Environment &env)
{
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/bin")
        .arg(maddeRoot())));
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/bin")
        .arg(targetRoot())));

    // put this into environment to make pkg-config stuff work
    env.prependOrSet(QLatin1String("SYSROOT_DIR"), sysrootRoot());
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/madbin")
        .arg(maddeRoot())));
    env.prependOrSetPath(QDir::toNativeSeparators(QString("%1/madlib")
        .arg(maddeRoot())));
    env.prependOrSet(QLatin1String("PERL5LIB"),
        QDir::toNativeSeparators(QString("%1/madlib/perl5").arg(maddeRoot())));
}

QString MaemoToolChain::makeCommand() const
{
    return QLatin1String("make" EXEC_SUFFIX);
}

bool MaemoToolChain::equals(ToolChain *other) const
{
    MaemoToolChain *toolChain = static_cast<MaemoToolChain*> (other);
    return other->type() == type()
        && toolChain->sysrootRoot() == sysrootRoot()
        && toolChain->targetRoot() == targetRoot();
}

QString MaemoToolChain::maddeRoot() const
{
    if (!m_maddeInitialized)
        setMaddeRoot();
    return m_maddeRoot;
}

QString MaemoToolChain::targetRoot() const
{
    return m_targetRoot;
}

QString MaemoToolChain::sysrootRoot() const
{
    if (!m_sysrootInitialized)
        setSysroot();
    return m_sysrootRoot;
}

void MaemoToolChain::setMaddeRoot() const
{
    QDir dir(targetRoot());
    dir.cdUp(); dir.cdUp();

    m_maddeInitialized = true;
    m_maddeRoot = dir.absolutePath();
}

void MaemoToolChain::setSysroot() const
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
        }
    }

    m_sysrootInitialized = true;
}
