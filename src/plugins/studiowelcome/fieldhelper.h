// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStringList>

QT_FORWARD_DECLARE_CLASS(QStandardItemModel)

namespace ProjectExplorer {
class JsonFieldPage;
class CheckBoxField;
class ComboBoxField;
}

namespace StudioWelcome::FieldHelper {

class CheckBoxHelper
{
public:
    CheckBoxHelper(ProjectExplorer::JsonFieldPage *detailsPage, const QString &fieldName);

    void setChecked(bool value);
    bool isChecked() const;

private:
    ProjectExplorer::CheckBoxField *m_field = nullptr;
};

class ComboBoxHelper
{
public:
    ComboBoxHelper(ProjectExplorer::JsonFieldPage *detailsPage, const QString &fieldName);

    void selectIndex(int index);

    QString text(int index) const;
    int indexOf(const QString &text) const;
    int selectedIndex() const;
    QStandardItemModel *model() const;
    QStringList allTexts() const;

private:
    ProjectExplorer::ComboBoxField *m_field = nullptr;
};

} // namespace StudioWelcome::FieldHelper
