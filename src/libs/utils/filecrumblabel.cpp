/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "filecrumblabel.h"

#include <utils/hostosinfo.h>

#include <QDir>
#include <QUrl>

namespace Utils {

FileCrumbLabel::FileCrumbLabel(QWidget *parent)
    : QLabel(parent)
{
    setTextFormat(Qt::RichText);
    setWordWrap(true);
    connect(this, &QLabel::linkActivated, this, [this](const QString &url) {
        emit pathClicked(FilePath::fromString(QUrl(url).toLocalFile()));
    });
    setPath(FilePath());
}

static QString linkForPath(const FilePath &path, const QString &display)
{
    return "<a href=\""
            + QUrl::fromLocalFile(path.toString()).toString(QUrl::FullyEncoded) + "\">"
            + display + "</a>";
}

void FileCrumbLabel::setPath(const FilePath &path)
{
    QStringList links;
    FilePath current = path;
    while (!current.isEmpty()) {
        const QString fileName = current.fileName();
        if (!fileName.isEmpty()) {
            links.prepend(linkForPath(current, fileName));
        } else if (HostOsInfo::isWindowsHost() && QDir(current.toString()).isRoot()) {
            // Only on Windows add the drive letter, without the '/' at the end
            QString display = current.toString();
            if (display.endsWith('/'))
                display.chop(1);
            links.prepend(linkForPath(current, display));
        }
        current = current.parentDir();
    }
    const auto pathSeparator = HostOsInfo::isWindowsHost() ? QLatin1String("&nbsp;\\ ")
                                                           : QLatin1String("&nbsp;/ ");
    const QString prefix = HostOsInfo::isWindowsHost() ? QString("\\ ") : QString("/ ");
    setText(prefix + links.join(pathSeparator));
}

} // Utils
