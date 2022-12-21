// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abstractaction.h"

namespace QmlDesigner {

class AddTabDesignerAction : public QObject, public AbstractAction
{
    Q_OBJECT
public:
    AddTabDesignerAction();

    QByteArray category() const override;
    QByteArray menuId() const override;
    int priority() const override;
    Type type() const override;

protected:
    bool isVisible(const SelectionContext &selectionContext) const override;
    bool isEnabled(const SelectionContext &selectionContext) const override;

private:
    void addNewTab();
};

} // namespace QmlDesigner
