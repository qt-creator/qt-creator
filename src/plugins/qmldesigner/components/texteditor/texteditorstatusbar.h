// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QToolBar>
#include <QLabel>

namespace QmlDesigner {

class TextEditorStatusBar : public QToolBar
{
    Q_OBJECT
public:
    explicit TextEditorStatusBar(QWidget *parent = nullptr);
    void clearText();
    void setText(const QString &text);
private:
    QLabel *m_label;
};

} // namespace QmlDesigner


