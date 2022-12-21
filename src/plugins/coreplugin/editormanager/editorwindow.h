// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

namespace Core {
namespace Internal {

class EditorArea;

class EditorWindow : public QWidget
{
    Q_OBJECT
public:
    explicit EditorWindow(QWidget *parent = nullptr);
    ~EditorWindow() override;

    EditorArea *editorArea() const;

    QVariantHash saveState() const;
    void restoreState(const QVariantHash &state);
private:
    void updateWindowTitle();

    EditorArea *m_area;
};

} // Internal
} // Core
