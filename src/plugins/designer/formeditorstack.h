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

QT_BEGIN_NAMESPACE
class QDesignerFormWindowInterface;
class QDesignerFormEditorInterface;
QT_END_NAMESPACE

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
    explicit FormEditorStack(QWidget *parent = 0);

    Designer::FormWindowEditor *createFormWindowEditor(DesignerXmlEditorEditable *xmlEditor);
    bool removeFormWindowEditor(Core::IEditor *xmlEditor);
    bool setVisibleEditor(Core::IEditor *xmlEditor);
    Designer::FormWindowEditor *formWindowEditorForXmlEditor(const Core::IEditor *xmlEditor) const;
    Designer::FormWindowEditor *formWindowEditorForFormWindow(const QDesignerFormWindowInterface *fw) const;
    FormWindowEditor *activeFormWindow() const;

private slots:
    void formChanged();
    void reloadDocument();
    void updateFormWindowSelectionHandles();

private:
    inline int indexOf(const QDesignerFormWindowInterface *) const;
    inline int indexOf(const Core::IEditor *xmlEditor) const;

    void setFormEditorData(Designer::FormWindowEditor *formEditor, const QString &contents);
    struct FormXmlData {
        FormXmlData();
        DesignerXmlEditorEditable *xmlEditor;
        Designer::FormWindowEditor *formEditor;
    };
    QList<FormXmlData> m_formEditors;
    QDesignerFormEditorInterface *m_designerCore;
};

} // namespace Internal
} // namespace Designer

#endif // FORMEDITORSTACK_H
