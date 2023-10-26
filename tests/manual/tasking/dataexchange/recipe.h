// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef RECIPE_H
#define RECIPE_H

#include <QImage>
#include <QMap>
#include <QUrl>

#include <optional>

namespace Tasking {
class Group;
template <typename T>
class TreeStorage;
}

static const int s_sizeInterval = 10;
static const int s_imageCount = 100;
static const int s_maxSize = s_sizeInterval * s_imageCount;

class QNetworkAccessManager;

class ExternalData
{
public:
    QNetworkAccessManager *inputNam = nullptr;
    QUrl inputUrl;
    QMap<int, QImage> outputImages;
    std::optional<QString> outputError;
};

Tasking::Group recipe(const Tasking::TreeStorage<ExternalData> &externalStorage);

#endif // RECIPE_H
