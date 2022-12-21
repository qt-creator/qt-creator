// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppsemanticinfo.h"

#include <QObject>
#include <QScopedPointer>

namespace CppEditor {

class SemanticInfoUpdaterPrivate;

class SemanticInfoUpdater : public QObject
{
    Q_OBJECT

public:
    explicit SemanticInfoUpdater();
    ~SemanticInfoUpdater() override;

    SemanticInfo semanticInfo() const;

    SemanticInfo update(const SemanticInfo::Source &source);
    void updateDetached(const SemanticInfo::Source &source);

signals:
    void updated(const SemanticInfo &semanticInfo);

private:
    QScopedPointer<SemanticInfoUpdaterPrivate> d;
};

} // namespace CppEditor
