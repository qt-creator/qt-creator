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

#include "editordata.h"

#include <QtGui/QStackedWidget>
#include <QtCore/QList>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QDesignerFormWindowInterface;
class QDesignerFormEditorInterface;
QT_END_NAMESPACE

namespace Core {
    class IEditor;
    class IMode;
}

namespace Designer {
namespace Internal {

/* FormEditorStack: Maintains a stack of Qt Designer form windows embedded
 * into a scrollarea and their associated XML editors.
 * Takes care of updating the XML editor once design mode is left.
 * Also updates the maincontainer resize handles when the active form
 * window changes. */
class FormEditorStack : public QStackedWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(FormEditorStack);
public:
    explicit FormEditorStack(QWidget *parent = 0);

    void add(const EditorData &d);
    bool removeFormWindowEditor(Core::IEditor *xmlEditor);

    bool setVisibleEditor(Core::IEditor *xmlEditor);
    SharedTools::WidgetHost *formWindowEditorForXmlEditor(const Core::IEditor *xmlEditor) const;
    SharedTools::WidgetHost *formWindowEditorForFormWindow(const QDesignerFormWindowInterface *fw) const;

    EditorData activeEditor() const;

private slots:
    void updateFormWindowSelectionHandles();
    void modeAboutToChange(Core::IMode *);
    void formSizeChanged(int w, int h);

private:
    inline int indexOf(const QDesignerFormWindowInterface *) const;
    inline int indexOf(const Core::IEditor *xmlEditor) const;

    QList<EditorData> m_formEditors;
    QDesignerFormEditorInterface *m_designerCore;
};

} // namespace Internal
} // namespace Designer

#endif // FORMEDITORSTACK_H
