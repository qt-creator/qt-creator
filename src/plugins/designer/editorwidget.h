/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
    explicit EditorWidget(QWidget *parent = 0);

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
    FormEditorStack *m_stack;
    QDockWidget *m_designerDockWidgets[Designer::Constants::DesignerSubWindowCount];
};

} // namespace Internal
} // namespace Designer
