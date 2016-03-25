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

#include "editordata.h"

#include <coreplugin/id.h>

#include <QStackedWidget>
#include <QList>

QT_BEGIN_NAMESPACE
class QDesignerFormWindowInterface;
class QDesignerFormEditorInterface;
QT_END_NAMESPACE

namespace Core { class IEditor; }

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
    void modeAboutToChange(Core::Id mode);
    void formSizeChanged(int w, int h);

private:
    inline int indexOfFormWindow(const QDesignerFormWindowInterface *) const;
    inline int indexOfFormEditor(const QObject *xmlEditor) const;

    QList<EditorData> m_formEditors;
    QDesignerFormEditorInterface *m_designerCore;
};

} // namespace Internal
} // namespace Designer
