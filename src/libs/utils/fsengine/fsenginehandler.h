// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtCore/private/qabstractfileengine_p.h>

namespace Utils::Internal {

class FSEngineHandler : public QAbstractFileEngineHandler
{
public:
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    std::unique_ptr<QAbstractFileEngine> create(const QString &fileName) const override;
#else
    QAbstractFileEngine *create(const QString &fileName) const override;
#endif
};

} // Utils::Internal
