// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QItemSelection;
class QLabel;
class QListView;
class QRadioButton;
class QSortFilterProxyModel;
class QStringListModel;
QT_END_NAMESPACE

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

    QString m_selected;
    Result m_result;
    QStringListModel *m_listModel;
    QSortFilterProxyModel *m_filterModel;

    QLabel *m_detailsLabel;
    QRadioButton *adjustReferencesRB;
    QListView *m_symbolicNamesList;
    QDialogButtonBox *m_buttonBox;
};

} // namespace Internal
} // namespace Squish
