/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
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
