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

#include "designer_export.h"
#include <texteditor/texteditor.h>

namespace Designer {

namespace Internal { class FormWindowFile; }

// IEditor that is used for the QDesignerFormWindowInterface
// It is a read-only text editor that shows the XML of the form.
// DesignerXmlEditorWidget is the corresponding BaseTextEditorWidget,
// FormWindowFile the corresponding BaseTextDocument.
// The content from the QDesignerFormWindowInterface is synced to the
// content of the XML editor.

class DESIGNER_EXPORT FormWindowEditor : public TextEditor::BaseTextEditor
{
    Q_PROPERTY(QString contents READ contents)
    Q_OBJECT

public:
    FormWindowEditor();
    ~FormWindowEditor() override;

    QWidget *toolBar() override;
    bool isDesignModePreferred() const override;

    // For uic code model support
    QString contents() const;

    // Convenience access.
    Internal::FormWindowFile *formWindowFile() const;
};

} // namespace Designer
