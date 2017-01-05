/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "consoleitem.h"
#include <utils/treemodel.h>

#include <QItemSelectionModel>

QT_BEGIN_NAMESPACE
class QFont;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class ConsoleItemModel : public Utils::TreeModel<>
{
    Q_OBJECT
public:

    explicit ConsoleItemModel(QObject *parent = 0);

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
    int m_maxSizeOfFileName;
    bool m_canFetchMore;
};

} // Internal
} // Debugger
