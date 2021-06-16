/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Event List module.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
******************************************************************************/
#pragma once

#include <QItemSelectionModel>

namespace QmlDesigner {

class NodeSelectionModel : public QItemSelectionModel
{
    Q_OBJECT

signals:
    void nodeSelected(const QStringList &events);

public:
    NodeSelectionModel(QAbstractItemModel *model);

    void select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command) override;

    QList<int> selectedNodes() const;
    void selectNode(int nodeId);

    void storeSelection();
    void reselect();

private:
    QItemSelection m_stored;
};

} // namespace QmlDesigner.
