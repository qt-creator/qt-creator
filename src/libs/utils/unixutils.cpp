// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "unixutils.h"

#include "filepath.h"
#include "qtcsettings.h"
#include "utilstr.h"

#include <QFileInfo>

namespace Utils::UnixUtils {

QString defaultFileBrowser()
{
    return QLatin1String("xdg-open %d");
}

QString fileBrowser(const QtcSettings *settings)
{
    const QString dflt = defaultFileBrowser();
    if (!settings)
        return dflt;
    return settings->value("General/FileBrowser", dflt).toString();
}

void setFileBrowser(QtcSettings *settings, const QString &term)
{
    settings->setValueWithDefault("General/FileBrowser", term, defaultFileBrowser());
}

QString fileBrowserHelpText()
{
    return Tr::tr("<table border=1 cellspacing=0 cellpadding=3>"
                  "<tr><th>Variable</th><th>Expands to</th></tr>"
                  "<tr><td>%d</td><td>directory of current file</td></tr>"
                  "<tr><td>%f</td><td>file name (with full path)</td></tr>"
                  "<tr><td>%n</td><td>file name (without path)</td></tr>"
                  "<tr><td>%%</td><td>%</td></tr>"
                  "</table>");
}

QString substituteFileBrowserParameters(const QString &pre, const QString &file)
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
                s = QLatin1Char('"') + FilePath::fromString(file).fileName() + QLatin1Char('"');
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

} // Utils::UnixUtils
