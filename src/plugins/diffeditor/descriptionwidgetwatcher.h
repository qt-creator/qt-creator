/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "diffeditor_global.h"

#include <QObject>

namespace Core {
class IDocument;
class IEditor;
}
namespace TextEditor { class TextEditorWidget; }

namespace DiffEditor {

class DiffEditorController;

class DIFFEDITOR_EXPORT DescriptionWidgetWatcher : public QObject
{
    Q_OBJECT
public:
    explicit DescriptionWidgetWatcher(DiffEditorController *controller);
    QList<TextEditor::TextEditorWidget *> descriptionWidgets() const;

signals:
    void descriptionWidgetAdded(TextEditor::TextEditorWidget *editor);
    void descriptionWidgetRemoved(TextEditor::TextEditorWidget *editor);

private:
    TextEditor::TextEditorWidget *descriptionWidget(Core::IEditor *editor) const;

    QList<TextEditor::TextEditorWidget *> m_widgets;
    Core::IDocument *m_document = nullptr;
};

} // namespace DiffEditor
