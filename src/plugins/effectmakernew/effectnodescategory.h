// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "effectnode.h"

#include <QObject>

namespace EffectMaker {

class EffectNodesCategory : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString categoryName MEMBER m_name CONSTANT)
    Q_PROPERTY(QList<EffectNode *> categoryNodes MEMBER m_categoryNodes CONSTANT)

public:
    EffectNodesCategory(const QString &name, const QList<EffectNode *> &nodes);

    QString name() const;
    QList<EffectNode *> nodes() const;

private:
    QString m_name;
    QList<EffectNode *> m_categoryNodes;
};

} // namespace EffectMaker

