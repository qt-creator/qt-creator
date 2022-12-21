// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractactiongroup.h>

#include <QCoreApplication>

namespace QmlDesigner {

class EnterTabDesignerAction : public AbstractActionGroup
{
    Q_DECLARE_TR_FUNCTIONS(EnterTabDesignerAction)

public:
    EnterTabDesignerAction();

    QByteArray category() const override;
    QByteArray menuId() const override;
    int priority() const override;
    void updateContext() override;

protected:
    bool isVisible(const SelectionContext &selectionContext) const override;
    bool isEnabled(const SelectionContext &selectionContext) const override;

private:
    void createActionForTab(const ModelNode &modelNode);
};

} // namespace QmlDesigner
