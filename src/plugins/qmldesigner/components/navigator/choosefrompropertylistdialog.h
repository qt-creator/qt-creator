/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <modelnode.h>
#include <nodeinstanceglobal.h>

#include <QtWidgets/qdialog.h>

namespace QmlDesigner {
namespace Ui {
class ChooseFromPropertyListDialog;
class ChooseFromPropertyListFilter;
}

class ChooseFromPropertyListFilter
{
public:
    ChooseFromPropertyListFilter(const NodeMetaInfo &metaInfo, const NodeMetaInfo &newInfo, bool breakOnFirst = false);
    ~ChooseFromPropertyListFilter() {}
    QStringList propertyList;
};

class ChooseFromPropertyListDialog : public QDialog
{
    Q_OBJECT

public:
    ~ChooseFromPropertyListDialog();

    TypeName selectedProperty() const;
    bool isSoloProperty() const { return m_isSoloProperty; }

    static ChooseFromPropertyListDialog *createIfNeeded(const ModelNode &targetNode,
                                                        const ModelNode &newNode,
                                                        QWidget *parent = 0);
    static ChooseFromPropertyListDialog *createIfNeeded(const ModelNode &targetNode,
                                                        TypeName type,
                                                        QWidget *parent = 0);

private:
    explicit ChooseFromPropertyListDialog(const QStringList &propNames, QWidget *parent = 0);
    void fillList(const QStringList &propNames);

    Ui::ChooseFromPropertyListDialog *m_ui;
    TypeName m_selectedProperty;
    bool m_isSoloProperty = false;
};
}
