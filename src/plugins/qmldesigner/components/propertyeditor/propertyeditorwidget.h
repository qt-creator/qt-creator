// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStackedWidget>

namespace QmlDesigner {

class PropertyEditorWidget : public QStackedWidget
{

Q_OBJECT

public:
    PropertyEditorWidget(QWidget *parent = nullptr);

signals:
    void resized();

protected:
    void resizeEvent(QResizeEvent * event) override;
};

} //QmlDesigner
