// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "designerconstants.h"

#include <utils/fancymainwindow.h>

QT_BEGIN_NAMESPACE
class QDesignerFormWindowInterface;
QT_END_NAMESPACE

namespace SharedTools { class WidgetHost; }
namespace Core { class IEditor; }

namespace Designer {

class FormWindowEditor;

namespace Internal {

class EditorData;
class FormEditorStack;

// Design mode main view.
class EditorWidget : public Utils::FancyMainWindow
{
    Q_OBJECT

public:
    explicit EditorWidget(QWidget *parent = nullptr);

    QDockWidget* const* designerDockWidgets() const;

    // Form editor stack API
    void add(SharedTools::WidgetHost *widgetHost, FormWindowEditor *formWindowEditor);
    void removeFormWindowEditor(Core::IEditor *xmlEditor);
    bool setVisibleEditor(Core::IEditor *xmlEditor);
    SharedTools::WidgetHost *formWindowEditorForXmlEditor(const Core::IEditor *xmlEditor) const;
    SharedTools::WidgetHost *formWindowEditorForFormWindow(const QDesignerFormWindowInterface *fw) const;

    EditorData activeEditor() const;
    void resetToDefaultLayout();

private:
    FormEditorStack *m_stack = nullptr;
    QDockWidget *m_designerDockWidgets[Designer::Constants::DesignerSubWindowCount];
};

} // namespace Internal
} // namespace Designer
