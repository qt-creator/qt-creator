// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "scxmltag.h"

#include <QPointer>
#include <QStyledItemDelegate>

namespace ScxmlEditor {

namespace PluginInterface {

class AttributeItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit AttributeItemDelegate(QObject *parent = nullptr);

    void setTag(ScxmlTag *tag);
    ScxmlTag *tag() const;

protected:
    QPointer<ScxmlTag> m_tag;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
