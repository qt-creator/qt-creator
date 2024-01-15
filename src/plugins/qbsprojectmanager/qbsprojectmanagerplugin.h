// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace QbsProjectManager::Internal {

class QbsProject;

void buildNamedProduct(QbsProject *project, const QString &product);

} // QbsProjectManager::Private
