// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qbsprojectmanager_global.h"

#include <QObject>
#include <QVariantMap>

namespace ProjectExplorer { class Kit; }

namespace QbsProjectManager {

class QBSPROJECTMANAGER_EXPORT PropertyProvider : public QObject
{
    Q_OBJECT

public:
    PropertyProvider();
    ~PropertyProvider() override;

    virtual bool canHandle(const ProjectExplorer::Kit *k) const = 0;
    virtual QVariantMap properties(const ProjectExplorer::Kit *k, const QVariantMap &defaultData) const = 0;
};

} // namespace QbsProjectManager
