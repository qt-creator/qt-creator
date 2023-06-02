// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fileutils_mac.h"

#include "qtcassert.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>

#include <Foundation/NSURL.h>

namespace Utils {
namespace Internal {

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
