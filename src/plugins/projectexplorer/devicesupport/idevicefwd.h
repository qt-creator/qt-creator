// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

QT_BEGIN_NAMESPACE
template <class T>
class QSharedPointer;
QT_END_NAMESPACE

namespace ProjectExplorer {

class IDevice;

using IDevicePtr = QSharedPointer<IDevice>;
using IDeviceConstPtr = QSharedPointer<const IDevice>;

} // namespace ProjectExplorer
