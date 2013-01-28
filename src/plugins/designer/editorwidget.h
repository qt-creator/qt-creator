/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DESIGNER_EDITORWIDGET_H
#define DESIGNER_EDITORWIDGET_H

#include "designerconstants.h"

#include <utils/fancymainwindow.h>

QT_BEGIN_NAMESPACE
class QDesignerFormWindowInterface;
QT_END_NAMESPACE

namespace SharedTools {
    class WidgetHost;
}
namespace Core {
    class IEditor;
}
namespace Designer {
class FormWindowEditor;

namespace Internal {
struct EditorData;
class FormEditorStack;
class FormEditorW;

// Design mode main view.
class EditorWidget : public Utils::FancyMainWindow
{
    Q_OBJECT

public:
    explicit EditorWidget(FormEditorW *fe, QWidget *parent = 0);

    QDockWidget* const* designerDockWidgets() const;

    // Form editor stack API
    void add(const EditorData &d);
    void removeFormWindowEditor(Core::IEditor *xmlEditor);
    bool setVisibleEditor(Core::IEditor *xmlEditor);
    SharedTools::WidgetHost *formWindowEditorForXmlEditor(const Core::IEditor *xmlEditor) const;
    SharedTools::WidgetHost *formWindowEditorForFormWindow(const QDesignerFormWindowInterface *fw) const;

    EditorData activeEditor() const;

public slots:
    void resetToDefaultLayout();

private:
    FormEditorStack *m_stack;
    QDockWidget *m_designerDockWidgets[Designer::Constants::DesignerSubWindowCount];
};

} // namespace Internal
} // namespace Designer

#endif // DESIGNER_EDITORWIDGET_H
