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

#include "designer_export.h"
#include "formwindowfile.h"

#include <texteditor/plaintexteditor.h>

namespace Core {
    class IMode;
}

namespace TextEditor {
    class BaseTextDocument;
}

namespace Designer {

namespace Internal {
class DesignerXmlEditor;
}

// The actual Core::IEditor belonging to Qt Designer. It internally embeds
// a TextEditor::PlainTextEditorEditable which is used to make the
// Read-only edit mode XML text editor work and delegates some functionality
// to it. However, the isDirty() handling is delegated to the FormWindowFile,
// which is the Core::IFile used for it.
class DESIGNER_EXPORT DesignerXmlEditorEditable : public Core::IEditor
{
    Q_OBJECT
public:
    explicit DesignerXmlEditorEditable(Internal::DesignerXmlEditor *editor,
                                       QDesignerFormWindowInterface *form,
                                       QObject *parent = 0);

    // IEditor
    virtual bool createNew(const QString &contents = QString());
    virtual bool open(const QString &fileName = QString());
    virtual Core::IFile *file();
    virtual QString id() const;
    virtual QString displayName() const;
    virtual void setDisplayName(const QString &title);

    virtual bool duplicateSupported() const;
    virtual IEditor *duplicate(QWidget *parent);

    virtual QByteArray saveState() const;
    virtual bool restoreState(const QByteArray &state);

    virtual bool isTemporary() const;

    virtual QWidget *toolBar();

    // IContext
    virtual QList<int> context() const;
    virtual QWidget *widget();

    // For uic code model support
    QString contents() const;

    TextEditor::BaseTextDocument *textDocument();
    TextEditor::PlainTextEditorEditable *textEditable();

public slots:
    void syncXmlEditor();

private slots:
    void slotOpen(const QString &fileName);

private:
    void syncXmlEditor(const QString &contents);

    TextEditor::PlainTextEditorEditable m_textEditable;
    Internal::FormWindowFile m_file;
    QList<int> m_context;
};

/* A stub-like, read-only text editor which displays UI files as text. Could be used as a
  * read/write editor too, but due to lack of XML editor, highlighting and other such
  * functionality, editing is disabled.
  * Provides an informational title bar containing a button triggering a
  * switch to design mode.
  * Internally manages DesignerXmlEditorEditable and uses the plain text
  * editable embedded in it.  */
namespace Internal {

class DesignerXmlEditor : public TextEditor::PlainTextEditor
{
    Q_OBJECT
public:
    explicit DesignerXmlEditor(QDesignerFormWindowInterface *form,
                               QWidget *parent = 0);

    DesignerXmlEditorEditable *designerEditable() const;

private slots:
    void designerModeClicked();
    void updateEditorInfoBar(Core::IEditor *editor);

protected:
    virtual TextEditor::BaseTextEditorEditable *createEditableInterface();

private:
    DesignerXmlEditorEditable *m_editable;
};

} // Internal
} // Designer

#endif // DESIGNERXMLEDITOR_H
