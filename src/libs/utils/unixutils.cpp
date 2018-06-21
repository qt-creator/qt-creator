/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "unixutils.h"
#include "fileutils.h"

#include <QSettings>
#include <QFileInfo>
#include <QCoreApplication>

using namespace Utils;

QString UnixUtils::defaultFileBrowser()
{
    return QLatin1String("xdg-open %d");
}

QString UnixUtils::fileBrowser(const QSettings *settings)
{
    const QString dflt = defaultFileBrowser();
    if (!settings)
        return dflt;
    return settings->value(QLatin1String("General/FileBrowser"), dflt).toString();
}

void UnixUtils::setFileBrowser(QSettings *settings, const QString &term)
{
    settings->setValue(QLatin1String("General/FileBrowser"), term);
}


QString UnixUtils::fileBrowserHelpText()
{
    QString help = QCoreApplication::translate("Utils::UnixTools",
            "<table border=1 cellspacing=0 cellpadding=3>"
            "<tr><th>Variable</th><th>Expands to</th></tr>"
            "<tr><td>%d</td><td>directory of current file</td></tr>"
            "<tr><td>%f</td><td>file name (with full path)</td></tr>"
            "<tr><td>%n</td><td>file name (without path)</td></tr>"
            "<tr><td>%%</td><td>%</td></tr>"
            "</table>");
    return help;
}

QString UnixUtils::substituteFileBrowserParameters(const QString &pre, const QString &file)
{
    QString cmd;
    for (int i = 0; i < pre.size(); ++i) {
        QChar c = pre.at(i);
        if (c == QLatin1Char('%') && i < pre.size()-1) {
            c = pre.at(++i);
            QString s;
            if (c == QLatin1Char('d')) {
                s = QLatin1Char('"') + QFileInfo(file).path() + QLatin1Char('"');
            } else if (c == QLatin1Char('f')) {
                s = QLatin1Char('"') + file + QLatin1Char('"');
            } else if (c == QLatin1Char('n')) {
                s = QLatin1Char('"') + FileName::fromString(file).fileName() + QLatin1Char('"');
            } else if (c == QLatin1Char('%')) {
                s = c;
            } else {
                s = QLatin1Char('%');
                s += c;
            }
            cmd += s;
            continue;

        }
        cmd += c;
    }

    return cmd;
}
