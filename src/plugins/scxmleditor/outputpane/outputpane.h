// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QFrame>

namespace ScxmlEditor {

namespace OutputPane {

class OutputPane : public QFrame
{
    Q_OBJECT

public:
    explicit OutputPane(QWidget *parent = nullptr)
        : QFrame(parent)
    {
    }

    virtual QString title() const = 0;
    virtual QIcon icon() const = 0;
    virtual void setPaneFocus() = 0;
    virtual QColor alertColor() const
    {
        return QColor(Qt::red);
    }

signals:
    void iconChanged();
    void titleChanged();
    void dataChanged();
};

} // namespace OutputPane
} // namespace ScxmlEditor
