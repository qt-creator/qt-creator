/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FORMWINDOWEDITOR_H
#define FORMWINDOWEDITOR_H

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

#endif // FORMWINDOWEDITOR_H
