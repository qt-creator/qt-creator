/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "test-utilities.h"
#include "stylemodel.h"

using namespace StudioWelcome;

namespace {

std::unique_ptr<QStandardItemModel> createBackendModel(const QList<QString> &itemNames)
{
    auto model = std::make_unique<QStandardItemModel>();

    for (const QString &name : itemNames)
        model->appendRow(new QStandardItem{name});

    return model;
}

QList<QString> displayValues(const QAbstractListModel *model)
{
    QList<QString> rows;
    for (int rowIndex = 0; rowIndex < model->rowCount(); ++rowIndex) {
        auto data = model->data(model->index(rowIndex, 0), Qt::DisplayRole);
        rows.push_back(data.value<QString>());
    }
    return rows;
}

} // namespace

class QdsStyleModel : public ::testing::Test
{
protected:
    StyleModel *aStyleModel(const QStringList &items)
    {
        m_styleModel = std::make_unique<StyleModel>();
        m_backendModel = createBackendModel(items);
        m_styleModel->setBackendModel(m_backendModel.get());
        return m_styleModel.get();
    }

private:
    std::unique_ptr<StyleModel> m_styleModel;
    std::unique_ptr<QStandardItemModel> m_backendModel;
};

/******************* TESTS *******************/

TEST_F(QdsStyleModel, emptyFilterReturnsAllItems)
{
    auto model = aStyleModel({"a", "b", "c", "d"});

    model->filter("");

    ASSERT_THAT(displayValues(model), ElementsAreArray({"a", "b", "c", "d"}));
}

TEST_F(QdsStyleModel, unknownFilterReturnsEmpty)
{
    auto model = aStyleModel({"a", "b", "c", "d"});

    model->filter("Unknown Filter");

    ASSERT_THAT(displayValues(model), IsEmpty());
}

TEST_F(QdsStyleModel, filterAllOnEmptyList)
{
    auto model = aStyleModel({});

    model->filter("all");

    ASSERT_THAT(displayValues(model), IsEmpty());
}

TEST_F(QdsStyleModel, filterLightOnEmptyList)
{
    auto model = aStyleModel({});

    model->filter("light");

    ASSERT_THAT(displayValues(model), IsEmpty());
}

TEST_F(QdsStyleModel, filterDarkOnEmptyList)
{
    auto model = aStyleModel({});

    model->filter("dark");

    ASSERT_THAT(displayValues(model), IsEmpty());
}

TEST_F(QdsStyleModel, filterAll)
{
    auto model = aStyleModel({"item a", "b", "item c", "item-d", "e"});

    model->filter("all");

    ASSERT_THAT(displayValues(model), ElementsAreArray({"item a", "b", "item c", "item-d", "e"}));
}

TEST_F(QdsStyleModel, filterLight)
{
    auto model = aStyleModel({"Fusion", "Material Light", "Material Dark", "Universal Light"});

    model->filter("light");

    ASSERT_THAT(displayValues(model), ElementsAreArray({"Material Light", "Universal Light"}));
}

TEST_F(QdsStyleModel, filterDark)
{
    auto model = aStyleModel({"Fusion", "Material Light", "Material Dark", "Universal Light"});

    model->filter("dark");

    ASSERT_THAT(displayValues(model), ElementsAreArray({"Material Dark"}));
}

TEST_F(QdsStyleModel, filterLightCaseInsensitive)
{
    auto model = aStyleModel({"Fusion", "Material Light", "Material Dark", "Universal Light"});

    model->filter("LIGHT");

    ASSERT_THAT(displayValues(model), ElementsAreArray({"Material Light", "Universal Light"}));
}

TEST_F(QdsStyleModel, filterDarkCaseInsensitive)
{
    auto model = aStyleModel({"Fusion", "Material Light", "Material Dark", "Universal Light"});

    model->filter("DARK");

    ASSERT_THAT(displayValues(model), ElementsAreArray({"Material Dark"}));
}
