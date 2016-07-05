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

#include "ui_qrceditor.h"
#include "resourceview.h"

#include <QWidget>
#include <QUndoStack>

namespace ResourceEditor {
namespace Internal {

class QrcEditor : public QWidget
{
    Q_OBJECT

public:
    QrcEditor(RelativeResourceModel *model, QWidget *parent = 0);
    virtual ~QrcEditor();

    void loaded(bool success);

    void setResourceDragEnabled(bool e);
    bool resourceDragEnabled() const;

    const QUndoStack *commandHistory() const { return &m_history; }

    void refresh();
    void editCurrentItem();

    QString currentResourcePath() const;

    void onUndo();
    void onRedo();

signals:
    void itemActivated(const QString &fileName);
    void showContextMenu(const QPoint &globalPos, const QString &fileName);
    void undoStackChanged(bool canUndo, bool canRedo);

private:
    void updateCurrent();
    void updateHistoryControls();

    void resolveLocationIssues(QStringList &files);

    void onAliasChanged(const QString &alias);
    void onPrefixChanged(const QString &prefix);
    void onLanguageChanged(const QString &language);
    void onRemove();
    void onRemoveNonExisting();
    void onAddFiles();
    void onAddPrefix();

    Ui::QrcEditor m_ui;
    QUndoStack m_history;
    ResourceView *m_treeview;
    QAction *m_addFileAction;

    QString m_currentAlias;
    QString m_currentPrefix;
    QString m_currentLanguage;
};

} // namespace Internal
} // namespace ResourceEditor
