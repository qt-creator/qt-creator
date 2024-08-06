// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filetypes.h"

namespace QmlProjectManager {

QStringList imageFiles(const std::function<QString(QString)> &transformer)
{
    static const QStringList suffixes = {
        "bmp", "dae", "gif", "hdr", "ico",
        "jpeg", "jpg", "ktx", "pbm", "pgm",
        "png", "ppm", "svg", "svgz", "tif",
        "tiff", "ttf", "webp", "xbm", "xpm"
    };

    if (transformer) {
        QStringList result;
        for (const QString& suffix : suffixes)
            result << transformer(suffix);
        return result;
    }
    return suffixes;
}

bool isQmlFile(const Utils::FilePath &path)
{
    const QString suffix = path.suffix();
    return suffix == "qml" || suffix == "ui.qml";
}

bool isAssetFile(const Utils::FilePath &path)
{
    static const QStringList suffixes = {
        "js", "ts", "json", "hints", "mesh", "qad", "qsb", "frag",
        "frag.qsb", "vert", "vert.qsb", "mng"
    };
    return
        suffixes.contains(path.suffix(), Qt::CaseInsensitive) ||
        imageFiles().contains(path.suffix(), Qt::CaseInsensitive);
}

bool isResource(const Utils::FilePath &path)
{
    auto isOtherFile = [](const Utils::FilePath &p) -> bool {
        static QStringList suffixes = { "qmlproject", "conf" };
        return p.fileName() == "qmldir" || suffixes.contains(p.suffix());
    };
    return isQmlFile(path) || isAssetFile(path) || isOtherFile(path);
}

} // namespace QmlProjectManager.
