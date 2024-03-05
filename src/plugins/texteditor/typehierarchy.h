// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor_global.h>

#include <QWidget>

namespace Core { class IEditor; }

namespace TextEditor {
namespace Internal {
void setupTypeHierarchyFactory();
void updateTypeHierarchy(QWidget *widget);
}

class TEXTEDITOR_EXPORT TypeHierarchyWidget : public QWidget
{
    Q_OBJECT
public:
    virtual void reload() = 0;
};

class TEXTEDITOR_EXPORT TypeHierarchyWidgetFactory : public QObject
{
    Q_OBJECT

public:
    virtual TypeHierarchyWidget *createWidget(Core::IEditor *editor) = 0;

protected:
    TypeHierarchyWidgetFactory();
    ~TypeHierarchyWidgetFactory() override;
};

TEXTEDITOR_EXPORT void openTypeHierarchy();

} // namespace TextEditor
