/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "pkgconfigtool.h"

#include <QProcess>
#include <QTextStream>
#include <QDebug>

namespace GenericProjectManager {
namespace Internal {

QList<PkgConfigTool::Package> PkgConfigTool::packages() const
{
    if (m_packages.isEmpty())
        packages_helper();

    return m_packages;
}

void PkgConfigTool::packages_helper() const
{
    QStringList args;
    args.append(QLatin1String("--list-all"));

    QProcess pkgconfig;
    pkgconfig.start(QLatin1String("pkg-config"), args);
    while (! pkgconfig.waitForFinished()) {
    }

    QTextStream in(&pkgconfig);
    forever {
        const QString line = in.readLine();

        if (line.isNull())
            break;

        else if (line.isEmpty())
            continue;

        int i = 0;
        for (; i != line.length(); ++i) {
            if (line.at(i).isSpace())
                break;
        }

        Package package;
        package.name = line.left(i).trimmed();
        package.description = line.mid(i).trimmed();

        QStringList args;
        args << package.name << QLatin1String("--cflags");
        pkgconfig.start(QLatin1String("pkg-config"), args);

        while (! pkgconfig.waitForFinished()) {
        }

        const QString cflags = QString::fromUtf8(pkgconfig.readAll());

        int index = 0;
        while (index != cflags.size()) {
            const QChar ch = cflags.at(index);

            if (ch.isSpace()) {
                do { ++index; }
                while (index < cflags.size() && cflags.at(index).isSpace());
            }

            else if (ch == QLatin1Char('-') && index + 1 < cflags.size()) {
                ++index;

                const QChar opt = cflags.at(index);

                if (opt == QLatin1Char('I')) {
                    // include paths.

                    int start = ++index;
                    for (; index < cflags.size(); ++index) {
                        if (cflags.at(index).isSpace())
                            break;
                    }

                    qDebug() << "*** add include path:" << cflags.mid(start, index - start);
                    package.includePaths.append(cflags.mid(start, index - start));
                }
            }

            else {
                for (; index < cflags.size(); ++index) {
                    if (cflags.at(index).isSpace())
                        break;
                }
            }
        }

        m_packages.append(package);
    }
}

} // namespace Internal
} // namespace GenericProjectManager
