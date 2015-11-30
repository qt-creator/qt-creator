/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "fileutils_mac.h"

#include "qtcassert.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>

#include <Foundation/NSURL.h>

namespace Utils {
namespace Internal {

QUrl filePathUrl(const QUrl &url)
{
    QUrl ret = url;
    @autoreleasepool {
        NSURL *nsurl = url.toNSURL();
        if ([nsurl isFileReferenceURL])
            ret = QUrl::fromNSURL([nsurl filePathURL]);
    }
    return ret;
}

QString normalizePathName(const QString &filePath)
{
    QString result;
    @autoreleasepool {
        // NSURL getResourceValue returns values based on the cleaned path so we need to work on that.
        // It also returns the disk name for "/" and "/.." and errors on "" and relative paths,
        // so avoid that

        // we cannot know the normalized name for relative paths
        if (QFileInfo(filePath).isRelative())
            return filePath;

        QString path = QDir::cleanPath(filePath);
        // avoid empty paths and paths like "/../foo" or "/.."
        if (path.isEmpty() || path.contains(QLatin1String("/../")) || path.endsWith(QLatin1String("/..")))
            return filePath;

        while (path != QLatin1String("/") /*be defensive->*/&& path != QLatin1String(".") && !path.isEmpty()) {
            QFileInfo info(path);
            NSURL *nsurl = [NSURL fileURLWithPath:path.toNSString()];
            NSString *out;
            QString component;
            if ([nsurl getResourceValue:(NSString **)&out forKey:NSURLNameKey error:nil])
                component = QString::fromNSString(out);
            else // e.g. if the full path does not exist
                component = info.fileName();
            result.prepend(QLatin1Char('/') + component);
            path = info.path();
        }
        QTC_ASSERT(path == QLatin1String("/"), return filePath);
    }
    return result;
}

} // Internal
} // Utils
