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

#pragma once

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>

#include <QSharedPointer>

namespace Squish {
namespace Internal {

class ObjectsMapDocument;
class ObjectsMapEditorWidget;
class ObjectsMapTreeItem;

class ObjectsMapEditor : public Core::IEditor
{
    Q_OBJECT
public:
    ObjectsMapEditor(QSharedPointer<ObjectsMapDocument> document);
    ~ObjectsMapEditor() override;

    Core::IDocument *document() const override;
    QWidget *toolBar() override;
    Core::IEditor *duplicate() override;

private:
    QSharedPointer<ObjectsMapDocument> m_document;
};

class ObjectsMapEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT
public:
    ObjectsMapEditorFactory();
};

} // namespace Internal
} // namespace Squish
