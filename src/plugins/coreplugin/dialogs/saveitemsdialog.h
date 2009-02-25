/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef SAVEITEMSDIALOG_H
#define SAVEITEMSDIALOG_H

#include <QtCore/QMap>
#include <QtGui/QDialog>

#include "ui_saveitemsdialog.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace Core {

class IFile;
class EditorManager;

namespace Internal {

class MainWindow;

class FileItem : public QObject, public QTreeWidgetItem
{
    Q_OBJECT

public:
    FileItem(QTreeWidget *tree, bool supportOpen,
             bool open, const QString &text);
    bool shouldBeSaved() const;
    void setShouldBeSaved(bool s);
    bool shouldBeOpened() const;

private slots:
    void updateSCCCheckBox();

private:
    QCheckBox *createCheckBox(QTreeWidget *tree, int column);
    QCheckBox *m_saveCheckBox;
    QCheckBox *m_sccCheckBox;
};

class SaveItemsDialog : public QDialog
{
    Q_OBJECT

public:
    SaveItemsDialog(MainWindow *mainWindow,
        QMap<Core::IFile*, QString> items);

    void setMessage(const QString &msg);

    QList<Core::IFile*> itemsToSave() const;
    QSet<Core::IFile*> itemsToOpen() const;

private slots:
    void collectItemsToSave();
    void uncheckAll();
    void discardAll();

private:
    Ui::SaveItemsDialog m_ui;
    QMap<FileItem*, Core::IFile*> m_itemMap;
    QList<Core::IFile*> m_itemsToSave;
    QSet<Core::IFile*> m_itemsToOpen;
};

} // namespace Internal
} // namespace Core

#endif // SAVEITEMSDIALOG_H
