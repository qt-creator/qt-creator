/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#ifndef QRCEDITOR_H
#define QRCEDITOR_H

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
    QrcEditor(QWidget *parent = 0);
    virtual ~QrcEditor();

    bool load(const QString &fileName);
    bool save();
    QTreeView *treeView() { return m_treeview; }
    QString errorMessage() const { return m_treeview->errorMessage(); }

    bool isDirty();
    void setDirty(bool dirty);

    QString fileName() const;
    void setFileName(const QString &fileName);

    void setResourceDragEnabled(bool e);
    bool resourceDragEnabled() const;

    void addFile(const QString &prefix, const QString &file);

    const QUndoStack *commandHistory() const { return &m_history; }

    void refresh();
    void editCurrentItem();

    QString currentResourcePath() const;

signals:
    void dirtyChanged(bool dirty);
    void itemActivated(const QString &fileName);
    void showContextMenu(const QPoint &globalPos, const QString &fileName);

private slots:
    void updateCurrent();
    void updateHistoryControls();

private:
    void resolveLocationIssues(QStringList &files);

private slots:
    void onAliasChanged(const QString &alias);
    void onPrefixChanged(const QString &prefix);
    void onLanguageChanged(const QString &language);
    void onRemove();
    void onAddFiles();
    void onAddPrefix();

signals:
    void undoStackChanged(bool canUndo, bool canRedo);

public slots:
    void onUndo();
    void onRedo();

private:
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

#endif
