// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace VcsBase {
namespace Internal {

class VcsJsExtension : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE bool isConfigured(const QString &vcsId) const;
    Q_INVOKABLE QString displayName(const QString &vcsId) const;
    Q_INVOKABLE bool isValidRepoUrl(const QString &vcsId, const QString &location) const;
};

} // namespace Internal
} // namespace VcsBase
