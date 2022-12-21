// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "editordata.h"

#include <utils/id.h>

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
    explicit FormEditorStack(QWidget *parent = nullptr);
    ~FormEditorStack() override;

    void add(const EditorData &d);

    bool setVisibleEditor(Core::IEditor *xmlEditor);
    SharedTools::WidgetHost *formWindowEditorForXmlEditor(const Core::IEditor *xmlEditor) const;
    SharedTools::WidgetHost *formWindowEditorForFormWindow(const QDesignerFormWindowInterface *fw) const;

    EditorData activeEditor() const;

    void removeFormWindowEditor(QObject *);

private:
    void updateFormWindowSelectionHandles();
    void modeAboutToChange(Utils::Id mode);
    void formSizeChanged(const SharedTools::WidgetHost *widgetHost, int w, int h);

    inline int indexOfFormWindow(const QDesignerFormWindowInterface *) const;
    inline int indexOfFormEditor(const QObject *xmlEditor) const;

    QList<EditorData> m_formEditors;
    QDesignerFormEditorInterface *m_designerCore = nullptr;
};

} // namespace Internal
} // namespace Designer
