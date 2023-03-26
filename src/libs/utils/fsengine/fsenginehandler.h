// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtCore/private/qabstractfileengine_p.h>

namespace Utils::Internal {

class FSEngineHandler : public QAbstractFileEngineHandler
{
public:
    QAbstractFileEngine *create(const QString &fileName) const override;
};

} // Utils::Internal
