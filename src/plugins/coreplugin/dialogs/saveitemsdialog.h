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

#include <QList>
#include <QDialog>

#include "ui_saveitemsdialog.h"

namespace Core {

class IDocument;

namespace Internal {


class SaveItemsDialog : public QDialog
{
    Q_OBJECT

public:
    SaveItemsDialog(QWidget *parent, QList<IDocument *> items);

    void setMessage(const QString &msg);
    void setAlwaysSaveMessage(const QString &msg);
    bool alwaysSaveChecked();
    QList<IDocument *> itemsToSave() const;
    QStringList filesToDiff() const;

private:
    void collectItemsToSave();
    void collectFilesToDiff();
    void discardAll();
    void updateButtons();
    void adjustButtonWidths();

    Ui::SaveItemsDialog m_ui;
    QList<IDocument*> m_itemsToSave;
    QStringList m_filesToDiff;
    QPushButton *m_diffButton = nullptr;
};

} // namespace Internal
} // namespace Core
