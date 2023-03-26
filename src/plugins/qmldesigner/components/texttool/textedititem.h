// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "textedititemwidget.h"

namespace QmlDesigner {

class FormEditorScene;
class FormEditorItem;

class TextEditItem : public TextEditItemWidget
{
    Q_OBJECT
public:
    TextEditItem(FormEditorScene* scene);
    ~TextEditItem() override;
    int type() const override;

    void setFormEditorItem(FormEditorItem *formEditorItem);
    FormEditorItem *formEditorItem() const;

    void updateText();
    void writeTextToProperty();

signals:
    void returnPressed();
private:
    FormEditorItem *m_formEditorItem;
};

inline int TextEditItem::type() const
{
    return 0xEAAB;
}
} // namespace QmlDesigner
