// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/itemviews.h>

namespace Core {
namespace Internal { class OpenDocumentsDelegate; }

class CORE_EXPORT OpenDocumentsTreeView : public Utils::TreeView
{
    Q_OBJECT
public:
    explicit OpenDocumentsTreeView(QWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model) override;
    void setCloseButtonVisible(bool visible);

signals:
    void closeActivated(const QModelIndex &index);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Internal::OpenDocumentsDelegate *m_delegate;
};

} // namespace Core
