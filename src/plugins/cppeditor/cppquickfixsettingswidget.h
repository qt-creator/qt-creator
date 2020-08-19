/****************************************************************************
**
** Copyright (C) 2020 Leander Schulten <Leander.Schulten@rwth-aachen.de>
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
