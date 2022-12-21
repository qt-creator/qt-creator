// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
