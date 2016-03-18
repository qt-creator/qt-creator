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

#include <coreplugin/core_global.h>

#include <QDialog>

namespace Core {

class IDocument;

namespace Internal { class ReadOnlyFilesDialogPrivate; }

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
    explicit ReadOnlyFilesDialog(const QList<IDocument *> &documents,
                                 QWidget * parent = 0);

    ~ReadOnlyFilesDialog();

    void setMessage(const QString &message);
    void setShowFailWarning(bool show, const QString &warning = QString());

    int exec();

private:
    friend class Internal::ReadOnlyFilesDialogPrivate;
    Internal::ReadOnlyFilesDialogPrivate *d;
};

} // namespace Core
