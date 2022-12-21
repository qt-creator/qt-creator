// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
                                                        const NodeMetaInfo &propertyType,
                                                        QWidget *parent = 0);

private:
    explicit ChooseFromPropertyListDialog(const QStringList &propNames, QWidget *parent = 0);
    void fillList(const QStringList &propNames);

    Ui::ChooseFromPropertyListDialog *m_ui;
    TypeName m_selectedProperty;
    bool m_isSoloProperty = false;
};
}
