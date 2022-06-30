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
class QItemSelection;
class QSortFilterProxyModel;
class QStringListModel;
QT_END_NAMESPACE

namespace Ui { class DeleteSymbolicNameDialog; }

namespace Squish {
namespace Internal {

class DeleteSymbolicNameDialog : public QDialog
{
    Q_OBJECT
public:
    enum Result { ResetReference, InvalidateNames, RemoveNames };

    explicit DeleteSymbolicNameDialog(const QString &symbolicName,
                                      const QStringList &names,
                                      QWidget *parent = nullptr);
    ~DeleteSymbolicNameDialog() override;

    QString selectedSymbolicName() const { return m_selected; }
    Result result() const { return m_result; }

private:
    void updateDetailsLabel(const QString &nameToDelete);
    void populateSymbolicNamesList(const QStringList &symbolicNames);
    void onAdjustReferencesToggled(bool checked);
    void onSelectionChanged(const QItemSelection &selection, const QItemSelection &);

    Ui::DeleteSymbolicNameDialog *ui;
    QString m_selected;
    Result m_result;
    QStringListModel *m_listModel;
    QSortFilterProxyModel *m_filterModel;
};

} // namespace Internal
} // namespace Squish
