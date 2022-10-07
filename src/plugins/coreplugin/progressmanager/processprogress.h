// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/core_global.h>

#include <QObject>

QT_BEGIN_NAMESPACE
template <typename T>
class QFutureInterface;
QT_END_NAMESPACE

namespace Utils { class QtcProcess; }

namespace Core {

using ProgressParser = std::function<void(QFutureInterface<void> &, const QString &)>;

class ProcessProgressPrivate;

class CORE_EXPORT ProcessProgress : public QObject
{
public:
    ProcessProgress(Utils::QtcProcess *process); // Makes ProcessProgress a child of process

    void setDisplayName(const QString &name);
    void setProgressParser(const ProgressParser &parser);

private:
    ProcessProgressPrivate *d;
};

} // namespace Core
