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


#include "unixutils.h"
#include <QtCore/QSettings>
#include <QtCore/QObject>
#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>

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
    return settings->setValue(QLatin1String("General/FileBrowser"), term);
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
            if (c == QLatin1Char('d'))
                s = QFileInfo(file).path();
            else if (c == QLatin1Char('f'))
                s = file;
            else if (c == QLatin1Char('n'))
                s = QFileInfo(file).fileName();
            else if (c == QLatin1Char('%'))
                s = c;
            else {
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
