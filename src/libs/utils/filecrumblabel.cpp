// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "filecrumblabel.h"

#include "filepath.h"
#include "hostosinfo.h"

#include <QDir>
#include <QUrl>

namespace Utils {

FileCrumbLabel::FileCrumbLabel(QWidget *parent)
    : QLabel(parent)
{
    setTextFormat(Qt::RichText);
    setWordWrap(true);
    connect(this, &QLabel::linkActivated, this, [this](const QString &url) {
        emit pathClicked(FilePath::fromUrl(QUrl(url)));
    });
    setPath(FilePath());
}

static QString linkForPath(const FilePath &path, const QString &display)
{
    return "<a href=\""
            + path.toUrl().toString(QUrl::FullyEncoded) + "\">"
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
        } else if (HostOsInfo::isWindowsHost() && current.isRootPath()) {
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
