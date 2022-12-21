// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "consoleitem.h"
#include <utils/treemodel.h>

#include <QItemSelectionModel>

QT_BEGIN_NAMESPACE
class QFont;
QT_END_NAMESPACE

namespace Debugger::Internal {

class ConsoleItemModel : public Utils::TreeModel<>
{
    Q_OBJECT
public:

    explicit ConsoleItemModel(QObject *parent = nullptr);

    void shiftEditableRow();

    void appendItem(ConsoleItem *item, int position = -1);

    int sizeOfFile(const QFont &font);
    int sizeOfLineNumber(const QFont &font);

    void clear();

    void setCanFetchMore(bool canFetchMore);
    bool canFetchMore(const QModelIndex &parent) const override;

signals:
    void selectEditableRow(const QModelIndex &index, QItemSelectionModel::SelectionFlags flags);

private:
    int m_maxSizeOfFileName = 0;
    bool m_canFetchMore = false;
};

} // Debugger::Internal
