/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

#include <QDialog>

QT_BEGIN_NAMESPACE
class QListWidgetItem;
QT_END_NAMESPACE

namespace Ui { class OpenSquishSuitesDialog; }

namespace Squish {
namespace Internal {

class OpenSquishSuitesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OpenSquishSuitesDialog(QWidget *parent = nullptr);
    ~OpenSquishSuitesDialog() override;
    QStringList chosenSuites() const { return m_chosenSuites; }

private:
    void onDirectoryChanged();
    void onListItemChanged(QListWidgetItem *);
    void selectAll();
    void deselectAll();
    void setChosenSuites();
    Ui::OpenSquishSuitesDialog *ui;
    QStringList m_chosenSuites;
};

} // namespace Internal
} // namespace Squish
