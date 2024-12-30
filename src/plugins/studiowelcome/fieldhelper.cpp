// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fieldhelper.h"

#include <projectexplorer/jsonwizard/jsonfieldpage.h>
#include <projectexplorer/jsonwizard/jsonfieldpage_p.h>

#include <utils/qtcassert.h>

using namespace StudioWelcome::FieldHelper;

CheckBoxHelper::CheckBoxHelper(ProjectExplorer::JsonFieldPage *detailsPage, const QString &fieldName)
    : m_field(dynamic_cast<ProjectExplorer::CheckBoxField *>(detailsPage->jsonField(fieldName)))
{}

void CheckBoxHelper::setChecked(bool value)
{
    QTC_ASSERT(m_field, return);

    m_field->setChecked(value);
}

bool CheckBoxHelper::isChecked() const
{
    QTC_ASSERT(m_field, return false);
    return m_field->isChecked();
}

ComboBoxHelper::ComboBoxHelper(ProjectExplorer::JsonFieldPage *detailsPage, const QString &fieldName)
    : m_field(dynamic_cast<ProjectExplorer::ComboBoxField *>(detailsPage->jsonField(fieldName)))
{}

void ComboBoxHelper::selectIndex(int index)
{
    QTC_ASSERT(m_field, return);

    m_field->selectRow(index);
}

QString ComboBoxHelper::text(int index) const
{
    QTC_ASSERT(m_field, return {});

    QStandardItemModel *model = m_field->model();
    if (index < 0 || index >= model->rowCount())
        return {};

    return model->item(index)->text();
}

int ComboBoxHelper::indexOf(const QString &text) const
{
    QTC_ASSERT(m_field, return -1);

    const QStandardItemModel *model = m_field->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        const QStandardItem *item = model->item(i, 0);
        if (text == item->text())
            return i;
    }

    return -1;
}

int ComboBoxHelper::selectedIndex() const
{
    QTC_ASSERT(m_field, return -1);

    return m_field->selectedRow();
}

QStandardItemModel *ComboBoxHelper::model() const
{
    QTC_ASSERT(m_field, return {});

    return m_field->model();
}

QStringList ComboBoxHelper::allTexts() const
{
    QTC_ASSERT(m_field, return {});

    QStandardItemModel *model = m_field->model();
    const int rows = model->rowCount();
    QStringList result;
    result.reserve(rows);

    for (int i = 0; i < rows; ++i)
        result.append(model->item(i)->text());

    return result;
}
