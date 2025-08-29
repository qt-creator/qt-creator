// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#include <memory>

QT_BEGIN_NAMESPACE
class QJsonObject;
class QString;
QT_END_NAMESPACE

namespace Utils { class FilePath; }

namespace QbsProjectManager::Internal {

class QbsProjectNode;

using BuildTreeResult = std::unique_ptr<QbsProjectNode>;

BuildTreeResult buildQbsProjectTree(const QString &projectName,
                                    const Utils::FilePath &projectFile,
                                    const Utils::FilePath &projectDir,
                                    const QJsonObject &projectData);

} // namespace QbsProjectManager::Internal
