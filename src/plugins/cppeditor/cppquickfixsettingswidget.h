// Copyright (C) 2020 Leander Schulten <Leander.Schulten@rwth-aachen.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#ifndef CppQuickFixSettingsWidget_H
#define CppQuickFixSettingsWidget_H

#include <QRegularExpression>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QListWidgetItem;
namespace Ui { class CppQuickFixSettingsWidget; }
QT_END_NAMESPACE

namespace CppEditor {
class CppQuickFixSettings;

namespace Internal {

class CppQuickFixSettingsWidget : public QWidget
{
    Q_OBJECT
    enum CustomDataRoles {
        Types = Qt::UserRole,
        Comparison,
        Assignment,
        ReturnExpression,
        ReturnType,
    };

public:
    explicit CppQuickFixSettingsWidget(QWidget *parent = nullptr);
    ~CppQuickFixSettingsWidget();
    void loadSettings(CppQuickFixSettings *settings);
    void saveSettings(CppQuickFixSettings *settings);
private slots:
    void currentCustomItemChanged(QListWidgetItem *newItem, QListWidgetItem *oldItem);
signals:
    void settingsChanged();

private:
    bool isLoadingSettings = false;
    Ui::CppQuickFixSettingsWidget *ui;
    const QRegularExpression typeSplitter;
};

} // namespace Internal
} // namespace CppEditor

#endif // CppQuickFixSettingsWidget_H
