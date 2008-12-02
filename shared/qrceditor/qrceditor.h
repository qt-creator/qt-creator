/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#ifndef QRCEDITOR_H
#define QRCEDITOR_H

#include "ui_qrceditor.h"
#include "resourceview.h"

#include <QtGui/QWidget>
#include <QtGui/QUndoStack>

namespace SharedTools {

class QrcEditor : public QWidget
{
    Q_OBJECT

public:
    QrcEditor(QWidget *parent = 0);
    virtual ~QrcEditor();

    bool load(const QString &fileName);
    bool save();

    bool isDirty();
    void setDirty(bool dirty);

    QString fileName() const;
    void setFileName(const QString &fileName);

    void setResourceDragEnabled(bool e);
    bool resourceDragEnabled() const;

    void setDefaultAddFileEnabled(bool enable);
    bool defaultAddFileEnabled() const;

    void addFile(const QString &prefix, const QString &file);
//    void removeFile(const QString &prefix, const QString &file);

signals:
    void dirtyChanged(bool dirty);
    void addFilesTriggered(const QString &prefix);

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

}

#endif
