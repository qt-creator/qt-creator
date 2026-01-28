// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QList>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Core {  class IWizardFactory; }

namespace Core::Internal {

void showNewDialog(
    const QString &title,
    QList<IWizardFactory *> factories,
    const Utils::FilePath &defaultLocation,
    const QVariantMap &extraVariables);

QWidget *currentNewDialog();

} // namespace Core::Internal
