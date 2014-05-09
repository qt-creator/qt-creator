/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef ANDROIDMANIFESTEDITOR_H
#define ANDROIDMANIFESTEDITOR_H

#include "androidmanifestdocument.h"
#include "androidmanifesteditorwidget.h"

#include <coreplugin/editormanager/ieditor.h>
#include <texteditor/basetexteditor.h>

QT_BEGIN_NAMESPACE
class QToolBar;
QT_END_NAMESPACE

namespace Android {
namespace Internal {

class AndroidManifestEditor : public Core::IEditor
{
    Q_OBJECT

public:
    explicit AndroidManifestEditor(AndroidManifestEditorWidget *editorWidget);

    bool open(QString *errorString, const QString &fileName, const QString &realFileName);
    QWidget *toolBar();
    AndroidManifestEditorWidget *widget() const;
    Core::IDocument *document();
    TextEditor::BaseTextEditorWidget *textEditor() const;

    int currentLine() const;
    int currentColumn() const;
    void gotoLine(int line, int column = 0, bool centerLine = true) { textEditor()->gotoLine(line, column, centerLine); }

private slots:
    void changeEditorPage(QAction *action);

private:
    QString m_displayName;
    QToolBar *m_toolBar;
    QActionGroup *m_actionGroup;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDMANIFESTEDITOR_H
