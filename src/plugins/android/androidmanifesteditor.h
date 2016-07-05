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

#include "androidmanifestdocument.h"
#include "androidmanifesteditorwidget.h"

#include <coreplugin/editormanager/ieditor.h>
#include <texteditor/texteditor.h>

#include <QActionGroup>

QT_BEGIN_NAMESPACE
class QToolBar;
class QActionGroup;
QT_END_NAMESPACE

namespace Android {
namespace Internal {

class AndroidManifestEditor : public Core::IEditor
{
    Q_OBJECT

public:
    explicit AndroidManifestEditor(AndroidManifestEditorWidget *editorWidget);

    QWidget *toolBar() override;
    AndroidManifestEditorWidget *widget() const override;
    Core::IDocument *document() override;
    TextEditor::TextEditorWidget *textEditor() const;

    int currentLine() const override;
    int currentColumn() const override;
    void gotoLine(int line, int column = 0, bool centerLine = true)  override;

private:
    void changeEditorPage(QAction *action);

    QString m_displayName;
    QToolBar *m_toolBar;
    QActionGroup *m_actionGroup;
};

} // namespace Internal
} // namespace Android
