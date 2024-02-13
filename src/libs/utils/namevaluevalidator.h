// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QPersistentModelIndex>
#include <QTimer>
#include <QValidator>

QT_BEGIN_NAMESPACE
class QModelIndex;
class QTreeView;
QT_END_NAMESPACE

namespace Utils {

class EnvironmentModel;

class QTCREATOR_UTILS_EXPORT NameValueValidator : public QValidator
{
public:
    NameValueValidator(QWidget *parent,
                       EnvironmentModel *model,
                       QTreeView *view,
                       const QModelIndex &index,
                       const QString &toolTipText);

    QValidator::State validate(QString &in, int &pos) const override;

    void fixup(QString &input) const override;

private:
    const QString m_toolTipText;
    EnvironmentModel *m_model;
    QTreeView *m_view;
    QPersistentModelIndex m_index;
    mutable QTimer m_hideTipTimer;
};

} // namespace Utils
