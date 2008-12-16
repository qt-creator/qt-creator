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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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
