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

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace DiffEditor {
namespace Internal {

DiffEditorFactory::DiffEditorFactory() :
    descriptionHandler {
        Constants::DIFF_EDITOR_ID,
        Constants::C_DIFF_EDITOR_DESCRIPTION,
        TextEditorActionHandler::None,
        [](IEditor *e) { return static_cast<DiffEditor *>(e)->descriptionWidget(); }
    },
    unifiedHandler {
        Constants::DIFF_EDITOR_ID,
        Constants::UNIFIED_VIEW_ID,
        TextEditorActionHandler::None,
        [](IEditor *e) { return static_cast<DiffEditor *>(e)->unifiedEditorWidget(); }
    },
    leftHandler {
        Constants::DIFF_EDITOR_ID,
        Id(Constants::SIDE_BY_SIDE_VIEW_ID).withSuffix(1),
        TextEditorActionHandler::None,
        [](IEditor *e) { return static_cast<DiffEditor *>(e)->leftEditorWidget(); }
    },
    rightHandler {
        Constants::DIFF_EDITOR_ID,
        Utils::Id(Constants::SIDE_BY_SIDE_VIEW_ID).withSuffix(2),
        TextEditorActionHandler::None,
        [](Core::IEditor *e) { return static_cast<DiffEditor *>(e)->rightEditorWidget(); }
    }
{
    setId(Constants::DIFF_EDITOR_ID);
    setDisplayName(QCoreApplication::translate("DiffEditorFactory", Constants::DIFF_EDITOR_DISPLAY_NAME));
    addMimeType(Constants::DIFF_EDITOR_MIMETYPE);
    setEditorCreator([] { return new DiffEditor(new DiffEditorDocument); });
}

} // namespace Internal
} // namespace DiffEditor
