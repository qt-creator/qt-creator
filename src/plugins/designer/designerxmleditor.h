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

class DesignerXmlEditor : public TextEditor::PlainTextEditorWidget
{
    Q_OBJECT
public:
    explicit DesignerXmlEditor(QDesignerFormWindowInterface *form,
                               QWidget *parent = 0);

    FormWindowEditor *designerEditor() const;

protected:
    virtual TextEditor::BaseTextEditor *createEditor();

private:
    FormWindowEditor *m_designerEditor;
};

} // Internal
} // Designer

#endif // DESIGNERXMLEDITOR_H
