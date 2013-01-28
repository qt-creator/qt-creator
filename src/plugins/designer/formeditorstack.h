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

#ifndef FORMEDITORSTACK_H
#define FORMEDITORSTACK_H

#include "editordata.h"

#include <QStackedWidget>
#include <QList>
#include <QString>

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

public:
    explicit FormEditorStack(QWidget *parent = 0);

    void add(const EditorData &d);

    bool setVisibleEditor(Core::IEditor *xmlEditor);
    SharedTools::WidgetHost *formWindowEditorForXmlEditor(const Core::IEditor *xmlEditor) const;
    SharedTools::WidgetHost *formWindowEditorForFormWindow(const QDesignerFormWindowInterface *fw) const;

    EditorData activeEditor() const;

public slots:
    void removeFormWindowEditor(QObject *);

private slots:
    void updateFormWindowSelectionHandles();
    void modeAboutToChange(Core::IMode *);
    void formSizeChanged(int w, int h);

private:
    inline int indexOfFormWindow(const QDesignerFormWindowInterface *) const;
    inline int indexOfFormEditor(const QObject *xmlEditor) const;

    QList<EditorData> m_formEditors;
    QDesignerFormEditorInterface *m_designerCore;
};

} // namespace Internal
} // namespace Designer

#endif // FORMEDITORSTACK_H
