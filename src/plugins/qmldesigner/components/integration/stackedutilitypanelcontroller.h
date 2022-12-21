// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utilitypanelcontroller.h"

QT_BEGIN_NAMESPACE
class QStackedWidget;
QT_END_NAMESPACE

namespace QmlDesigner {

class DesignDocument;

class StackedUtilityPanelController : public UtilityPanelController
{
    Q_OBJECT

public:
    StackedUtilityPanelController(QObject* parent = nullptr);

    void show(DesignDocument* DesignDocument);
    void close(DesignDocument* DesignDocument);

protected:
    QWidget* contentWidget() const override;
    virtual QWidget* stackedPageWidget(DesignDocument* DesignDocument) const = 0;

private:
    class QStackedWidget* m_stackedWidget;
};

} // namespace QmlDesigner
