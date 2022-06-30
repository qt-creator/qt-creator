/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

#include "objectsmapeditor.h"
#include "objectsmapdocument.h"
#include "objectsmapeditorwidget.h"
#include "squishconstants.h"

namespace Squish {
namespace Internal {

ObjectsMapEditor::ObjectsMapEditor(QSharedPointer<ObjectsMapDocument> document)
    : m_document(document)
{
    setWidget(new ObjectsMapEditorWidget(m_document.data()));
    setDuplicateSupported(true);
}

ObjectsMapEditor::~ObjectsMapEditor()
{
    delete m_widget;
}

Core::IDocument *ObjectsMapEditor::document() const
{
    return m_document.data();
}

QWidget *ObjectsMapEditor::toolBar()
{
    return nullptr;
}

Core::IEditor *ObjectsMapEditor::duplicate()
{
    return new ObjectsMapEditor(m_document);
}

/********************************* EditorFactory ********************************************/

ObjectsMapEditorFactory::ObjectsMapEditorFactory()
{
    setId(Constants::OBJECTSMAP_EDITOR_ID);
    setDisplayName("Squish Object Map Editor");
    addMimeType(Constants::SQUISH_OBJECTSMAP_MIMETYPE);
    setEditorCreator([]() {
        return new ObjectsMapEditor(QSharedPointer<ObjectsMapDocument>(new ObjectsMapDocument));
    });
}

} // namespace Internal
} // namespace Squish
