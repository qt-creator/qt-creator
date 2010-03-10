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

#ifndef DESIGNERXMLEDITOR_H
#define DESIGNERXMLEDITOR_H

#include <texteditor/plaintexteditor.h>

QT_BEGIN_NAMESPACE
class QDesignerFormWindowInterface;
QT_END_NAMESPACE

namespace Designer {
class FormWindowEditor;

namespace Internal {

/* A stub-like, read-only text editor which displays UI files as text. Could be used as a
  * read/write editor too, but due to lack of XML editor, highlighting and other such
  * functionality, editing is disabled.
  * Provides an informational title bar containing a button triggering a
  * switch to design mode.
  * Internally manages a FormWindowEditor and uses the plain text
  * editable embedded in it.  */

class DesignerXmlEditor : public TextEditor::PlainTextEditor
{
    Q_OBJECT
public:
    explicit DesignerXmlEditor(QDesignerFormWindowInterface *form,
                               QWidget *parent = 0);

    FormWindowEditor *designerEditor() const;

private slots:
    void designerModeClicked();
    void updateEditorInfoBar(Core::IEditor *editor);

protected:
    virtual TextEditor::BaseTextEditorEditable *createEditableInterface();

private:
    FormWindowEditor *m_designerEditor;
};

} // Internal
} // Designer

#endif // DESIGNERXMLEDITOR_H
