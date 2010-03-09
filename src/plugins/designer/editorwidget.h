/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DESIGNER_EDITORWIDGET_H
#define DESIGNER_EDITORWIDGET_H

#include "designerconstants.h"

#include <utils/fancymainwindow.h>

#include <QtCore/QHash>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE
class QDesignerFormWindowInterface;
QT_END_NAMESPACE

namespace Core {
    class IEditor;
}
namespace Designer {
class FormWindowEditor;
class DesignerXmlEditorEditable;

namespace Internal {
class FormEditorStack;
class FormEditorW;
/* Form editor splitter used as editor window. Contains the shared designer
 * windows. */
class EditorWidget : public Utils::FancyMainWindow
{
    Q_OBJECT
    Q_DISABLE_COPY(EditorWidget)
public:
    explicit EditorWidget(FormEditorW *fe, QWidget *parent = 0);

    QDockWidget* const* designerDockWidgets() const;

    // Form editor stack API
    Designer::FormWindowEditor *createFormWindowEditor(DesignerXmlEditorEditable *xmlEditor);
    bool removeFormWindowEditor(Core::IEditor *xmlEditor);
    bool setVisibleEditor(Core::IEditor *xmlEditor);
    Designer::FormWindowEditor *formWindowEditorForXmlEditor(const Core::IEditor *xmlEditor) const;
    Designer::FormWindowEditor *formWindowEditorForFormWindow(const QDesignerFormWindowInterface *fw) const;
    FormWindowEditor *activeFormWindow() const;

public slots:
    void resetToDefaultLayout();

private:
    FormEditorStack *m_stack;
    QDockWidget *m_designerDockWidgets[Designer::Constants::DesignerSubWindowCount];
};

} // namespace Internal
} // namespace Designer

#endif // DESIGNER_EDITORWIDGET_H
