// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QListWidgetItem;
class QListWidget;
class QDialogButtonBox;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace Squish {
namespace Internal {

class OpenSquishSuitesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OpenSquishSuitesDialog(QWidget *parent = nullptr);
    ~OpenSquishSuitesDialog() override;
    Utils::FilePaths chosenSuites() const { return m_chosenSuites; }

private:
    void onDirectoryChanged();
    void onListItemChanged(QListWidgetItem *);
    void selectAll();
    void deselectAll();
    void setChosenSuites();

    Utils::FilePaths m_chosenSuites;

    Utils::PathChooser *m_directoryLineEdit;
    QListWidget *m_suitesListWidget;
    QDialogButtonBox *m_buttonBox;
};

} // namespace Internal
} // namespace Squish
