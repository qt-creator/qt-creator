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

#ifndef READONLYFILESDIALOG_H
#define READONLYFILESDIALOG_H

#include <coreplugin/core_global.h>

#include <QDialog>
#include <QHash>

QT_BEGIN_NAMESPACE
class QButtonGroup;
class QTreeWidgetItem;
class QRadioButton;
QT_END_NAMESPACE

namespace Core {
class IVersionControl;
class IDocument;

namespace Internal {

namespace Ui { class ReadOnlyFilesDialog; }

class CORE_EXPORT ReadOnlyFilesDialog : public QDialog
{
    Q_OBJECT

private:
    enum ReadOnlyFilesTreeColumn {
        MakeWritable = 0,
        OpenWithVCS = 1,
        SaveAs = 2,
        FileName = 3,
        Folder = 4,
        NumberOfColumns
    };

public:
    enum ReadOnlyResult {
        RO_Cancel = -1,
        RO_OpenVCS = OpenWithVCS,
        RO_MakeWritable = MakeWritable,
        RO_SaveAs = SaveAs
    };

    explicit ReadOnlyFilesDialog(const QList<QString> &fileNames,
                                 QWidget *parent = 0);
    explicit ReadOnlyFilesDialog(const QString &fileName,
                                 QWidget * parent = 0);
    explicit ReadOnlyFilesDialog(IDocument *document,
                                 QWidget * parent = 0,
                                 bool displaySaveAs = false);
    explicit ReadOnlyFilesDialog(const QList<IDocument *> documents,
                                 QWidget * parent = 0);

    ~ReadOnlyFilesDialog();

    void setMessage(const QString &message);
    void setShowFailWarning(bool show, const QString &warning = QString());

    int exec();

private:
    void initDialog(const QStringList &fileNames);
    void promptFailWarning(const QStringList &files, ReadOnlyResult type) const;
    QRadioButton *createRadioButtonForItem(QTreeWidgetItem *item, QButtonGroup *group,
                              ReadOnlyFilesDialog::ReadOnlyFilesTreeColumn type);

private slots:
    void setAll(int index);
    void updateSelectAll();

private:
    class ReadOnlyFilesDialogPrivate *d;
    Ui::ReadOnlyFilesDialog *ui;
};

} // namespace Internal
} // namespace Core

#endif // READONLYFILESDIALOG_H
