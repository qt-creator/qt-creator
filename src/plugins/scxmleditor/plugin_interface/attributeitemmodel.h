// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "scxmldocument.h"
#include "scxmltag.h"

#include <QAbstractTableModel>
#include <QPointer>

namespace ScxmlEditor {

namespace PluginInterface {

class AttributeItemModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    AttributeItemModel(QObject *parent = nullptr);

    virtual void setTag(ScxmlTag *tag);
    ScxmlTag *tag() const;

protected:
    QPointer<ScxmlDocument> m_document;
    QPointer<ScxmlTag> m_tag;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
