/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
**
****************************************************************************/

#ifndef TESTTREEITEMDELEGATE_H
#define TESTTREEITEMDELEGATE_H

#include <QStyledItemDelegate>

namespace Autotest {
namespace Internal {

class TestTreeItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    TestTreeItemDelegate(QObject *parent = 0);
    ~TestTreeItemDelegate();

public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

} // namespace Internal
} // namespace Autotest

#endif // TESTTREEITEMDELEGATE_H
