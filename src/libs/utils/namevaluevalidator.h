// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "environmentfwd.h"

#include <QPersistentModelIndex>
#include <QTimer>
#include <QValidator>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace Utils {

class NameValueModel;

class QTCREATOR_UTILS_EXPORT NameValueValidator : public QValidator
{
public:
    NameValueValidator(QWidget *parent,
                       NameValueModel *model,
                       QTreeView *view,
                       const QModelIndex &index,
                       const QString &toolTipText);

    QValidator::State validate(QString &in, int &pos) const override;

    void fixup(QString &input) const override;

private:
    const QString m_toolTipText;
    NameValueModel *m_model;
    QTreeView *m_view;
    QPersistentModelIndex m_index;
    mutable QTimer m_hideTipTimer;
};

} // namespace Utils
