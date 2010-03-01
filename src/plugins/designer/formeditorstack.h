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

#ifndef FORMEDITORSTACK_H
#define FORMEDITORSTACK_H

#include <QtGui/QStackedWidget>
#include <QtCore/QList>
#include <QtCore/QString>

namespace Core {
    class IEditor;
}

namespace Designer {
class FormWindowEditor;
class DesignerXmlEditorEditable;

namespace Internal {

/**
  * A wrapper for Qt Designer form editors, so that they can be used in Design mode.
  * FormEditorW owns an instance of this class, and creates new form editors when
  * needed.
  */
class FormEditorStack : public QStackedWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(FormEditorStack);
public:
    FormEditorStack();
    ~FormEditorStack();
    Designer::FormWindowEditor *createFormWindowEditor(DesignerXmlEditorEditable *xmlEditor);
    bool removeFormWindowEditor(Core::IEditor *xmlEditor);
    bool setVisibleEditor(Core::IEditor *xmlEditor);
    Designer::FormWindowEditor *formWindowEditorForXmlEditor(Core::IEditor *xmlEditor);

private slots:
    void formChanged();
    void reloadDocument();

private:
    void setFormEditorData(Designer::FormWindowEditor *formEditor, const QString &contents);

    struct FormXmlData;
    QList<FormXmlData*> m_formEditors;

    FormXmlData *activeEditor;

    struct FormXmlData {
        DesignerXmlEditorEditable *xmlEditor;
        Designer::FormWindowEditor *formEditor;
        int widgetIndex;
    };

};

}
}

#endif // FORMEDITORSTACK_H
