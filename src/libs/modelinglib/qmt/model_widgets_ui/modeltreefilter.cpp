// Copyright (C) 2018 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modeltreefilter.h"
#include "ui_modeltreefilter.h"

#include <QStringListModel>

namespace qmt {

class ModelTreeFilter::Private {
public:
    QStringListModel m_typeModel;
    QStringListModel m_stereotypesModel;
    QStringListModel m_directionModel;
};

ModelTreeFilter::ModelTreeFilter(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ModelTreeFilter),
    d(new Private)
{
    ui->setupUi(this);
    connect(ui->resetViewButton, &QPushButton::clicked, this, &ModelTreeFilter::resetView);
    connect(ui->relationsCheckBox, &QCheckBox::clicked, this, &ModelTreeFilter::onViewChanged);
    connect(ui->diagramElementsCheckBox, &QCheckBox::clicked, this, &ModelTreeFilter::onViewChanged);
    connect(ui->clearFilterButton, &QPushButton::clicked, this, &ModelTreeFilter::clearFilter);
    connect(ui->typeComboBox, &QComboBox::currentIndexChanged, this, [this](int index) {
        ui->directionComboBox->setDisabled(index != (int) ModelTreeFilterData::Type::Dependency);
    });
    connect(ui->typeComboBox, &QComboBox::currentIndexChanged, this, &ModelTreeFilter::onFilterChanged);
    connect(ui->stereotypesComboBox, &QComboBox::currentTextChanged, this, &ModelTreeFilter::onFilterChanged);
    connect(ui->nameLineEdit, &QLineEdit::textChanged, this, &ModelTreeFilter::onFilterChanged);
    connect(ui->directionComboBox, &QComboBox::currentTextChanged, this, &ModelTreeFilter::onFilterChanged);
    setupFilter();
    resetView();
    clearFilter();
}

ModelTreeFilter::~ModelTreeFilter()
{
    delete d;
    delete ui;
}

void ModelTreeFilter::setupFilter()
{
    QStringList types = {"Any", "Package", "Component", "Class", "Diagram", "Item",
                         "Dependency", "Association", "Inheritance", "Connection"};
    d->m_typeModel.setStringList(types);
    ui->typeComboBox->setModel(&d->m_typeModel);

    QStringList stereotypes = { };
    d->m_stereotypesModel.setStringList(stereotypes);
    ui->stereotypesComboBox->setModel(&d->m_stereotypesModel);

    QStringList directions = {"Any", "Outgoing (->)", "Incoming (<-)", "Bidirectional (<->)"};
    d->m_directionModel.setStringList(directions);
    ui->directionComboBox->setModel(&d->m_directionModel);
}

void ModelTreeFilter::resetView()
{
    ui->relationsCheckBox->setChecked(true);
    ui->diagramElementsCheckBox->setChecked(false);
    onViewChanged();
}

void ModelTreeFilter::clearFilter()
{
    ui->typeComboBox->setCurrentIndex(0);
    ui->stereotypesComboBox->clearEditText();
    ui->nameLineEdit->clear();
    ui->directionComboBox->setCurrentIndex(0);
    onFilterChanged();
}

void ModelTreeFilter::onViewChanged()
{
    ModelTreeViewData modelTreeViewData;
    modelTreeViewData.setShowRelations(ui->relationsCheckBox->isChecked());
    modelTreeViewData.setShowDiagramElements(ui->diagramElementsCheckBox->isChecked());
    emit viewDataChanged(modelTreeViewData);
}

void ModelTreeFilter::onFilterChanged()
{
    ModelTreeFilterData modelTreeFilterData;
    modelTreeFilterData.setType((ModelTreeFilterData::Type) ui->typeComboBox->currentIndex());
    modelTreeFilterData.setCustomId(QString());
    modelTreeFilterData.setStereotypes(ui->stereotypesComboBox->currentText().split(',', Qt::SkipEmptyParts));
    modelTreeFilterData.setName(ui->nameLineEdit->text().trimmed());
    if (modelTreeFilterData.type() == ModelTreeFilterData::Type::Dependency)
        modelTreeFilterData.setDirection((ModelTreeFilterData::Direction) ui->directionComboBox->currentIndex());
    else
        modelTreeFilterData.setDirection(ModelTreeFilterData::Direction::Any);
    emit filterDataChanged(modelTreeFilterData);
}

} // namespace qmt
