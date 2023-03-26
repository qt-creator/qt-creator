// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "layeritem.h"
#include "formeditoritem.h"

#include <QPointer>
#include <QGraphicsRectItem>

namespace QmlDesigner {

class ContentNotEditableIndicator
{
public:
    using EntryPair = QPair<FormEditorItem*, QGraphicsRectItem *>;

    ContentNotEditableIndicator(LayerItem *layerItem);
    ContentNotEditableIndicator();
    ~ContentNotEditableIndicator();

    void clear();

    void setItems(const QList<FormEditorItem*> &itemList);
    void updateItems(const QList<FormEditorItem*> &itemList);

protected:
    void addAddiationEntries(const QList<FormEditorItem*> &itemList);
    void removeEntriesWhichAreNotInTheList(const QList<FormEditorItem*> &itemList);

private:
    QPointer<LayerItem> m_layerItem;
    QList<EntryPair> m_entryList;
};

} // namespace QmlDesigner
