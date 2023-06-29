// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace QmlDesigner {

class EffectNode
{
public:
    EffectNode(const QString &name);

    QString name() const;

private:
    QString m_name;
};

} // namespace QmlDesigner
