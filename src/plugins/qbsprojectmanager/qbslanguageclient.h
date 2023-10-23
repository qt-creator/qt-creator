// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <languageclient/client.h>

namespace QbsProjectManager::Internal {
class QbsBuildSystem;

class QbsLanguageClient : public LanguageClient::Client
{
    Q_OBJECT
public:
    QbsLanguageClient(const QString &serverPath, QbsBuildSystem *buildSystem);
    ~QbsLanguageClient() override;

    bool isActive() const;

private:
    class Private;
    Private * const d;
};

} // namespace QbsProjectManager::Internal
