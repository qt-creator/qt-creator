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

#include "diffeditor.h"
#include "diffeditorconstants.h"
#include "diffeditordocument.h"
#include "diffeditorfactory.h"
#include "sidebysidediffeditorwidget.h"

#include "texteditor/texteditoractionhandler.h"

#include <QCoreApplication>

namespace DiffEditor {
namespace Internal {

DiffEditorFactory::DiffEditorFactory(QObject *parent)
    : IEditorFactory(parent)
{
    setId(Constants::DIFF_EDITOR_ID);
    setDisplayName(QCoreApplication::translate("DiffEditorFactory", Constants::DIFF_EDITOR_DISPLAY_NAME));
    addMimeType(Constants::DIFF_EDITOR_MIMETYPE);
    auto descriptionHandler = new TextEditor::TextEditorActionHandler(
                this, id(), Constants::C_DIFF_EDITOR_DESCRIPTION);
    descriptionHandler->setTextEditorWidgetResolver([](Core::IEditor *e) {
        return static_cast<DiffEditor *>(e)->descriptionWidget();
    });
    auto unifiedHandler = new TextEditor::TextEditorActionHandler(
                this, id(), Constants::UNIFIED_VIEW_ID);
    unifiedHandler->setTextEditorWidgetResolver([](Core::IEditor *e) {
        return static_cast<DiffEditor *>(e)->unifiedEditorWidget();
    });
    auto leftHandler = new TextEditor::TextEditorActionHandler(
                this, id(), Core::Id(Constants::SIDE_BY_SIDE_VIEW_ID).withSuffix(1));
    leftHandler->setTextEditorWidgetResolver([](Core::IEditor *e) {
        return static_cast<DiffEditor *>(e)->leftEditorWidget();
    });
    auto rightHandler = new TextEditor::TextEditorActionHandler(
                this, id(), Core::Id(Constants::SIDE_BY_SIDE_VIEW_ID).withSuffix(2));
    rightHandler->setTextEditorWidgetResolver([](Core::IEditor *e) {
        return static_cast<DiffEditor *>(e)->rightEditorWidget();
    });
}

Core::IEditor *DiffEditorFactory::createEditor()
{
    return new DiffEditor(new DiffEditorDocument);
}

} // namespace Internal
} // namespace DiffEditor
