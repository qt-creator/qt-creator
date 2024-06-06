// Copyright (C) 2018 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modeltreefilter.h"

#include "../../modelinglibtr.h"

#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStringListModel>

namespace qmt {

class ModelTreeFilter::Private {
public:
    QStringListModel m_typeModel;
    QStringListModel m_stereotypesModel;
    QStringListModel m_directionModel;

    QPushButton *resetViewButton;
    QCheckBox *relationsCheckBox;
    QCheckBox *diagramElementsCheckBox;
    QPushButton *clearFilterButton;
    QComboBox *typeComboBox;
    QComboBox *stereotypesComboBox;
    QLineEdit *nameLineEdit;
    QComboBox *directionComboBox;
};

ModelTreeFilter::ModelTreeFilter(QWidget *parent) :
    QWidget(parent),
    d(new Private)
{
    d->resetViewButton = new QPushButton(Tr::tr("Reset"));
    d->relationsCheckBox = new QCheckBox(Tr::tr("Relations"));
    d->diagramElementsCheckBox = new QCheckBox(Tr::tr("Diagram Elements"));
    d->diagramElementsCheckBox->setMaximumHeight(0);
    d->clearFilterButton = new QPushButton(Tr::tr("Clear"));
    d->typeComboBox = new QComboBox;
    d->stereotypesComboBox = new QComboBox;
    d->stereotypesComboBox->setEditable(true);
    d->nameLineEdit = new QLineEdit;
    d->directionComboBox = new QComboBox;

    const auto boldLabel = [] (const QString &title) -> QLabel * {
        auto label = new QLabel(title);
        QFont boldFont = label->font();
        boldFont.setWeight(QFont::Bold);
        label->setFont(boldFont);
        return label;
    };

    const auto line = [] () -> QFrame * {
        auto line = new QFrame;
        line->setFrameShadow(QFrame::Plain);
        line->setFrameShape(QFrame::HLine);
        return line;
    };

    const int margin = 9;

    using namespace Layouting;
    Column {
        Column {
            Row {
                boldLabel(Tr::tr("View")), st, d->resetViewButton
            },
            d->relationsCheckBox,
            d->diagramElementsCheckBox,
            customMargins(margin, 0, margin, 0),
        },
        Space(10),
        line(),
        Column {
            Row {
                boldLabel(Tr::tr("Filter")), st, d->clearFilterButton
            },
            Space(10),
            Form {
                Tr::tr("Type:"), d->typeComboBox, br,
                Tr::tr("Stereotypes:"), d->stereotypesComboBox, br,
                Tr::tr("Name:"), d->nameLineEdit, br,
                Tr::tr("Direction:"), d->directionComboBox, br,
            },
            customMargins(margin, 0, margin, 0),
        },
        st,
        line(),
        customMargins(0, margin, 0, 0),
    }.attachTo(this);

    connect(d->resetViewButton, &QPushButton::clicked, this, &ModelTreeFilter::resetView);
    connect(d->relationsCheckBox, &QCheckBox::clicked, this, &ModelTreeFilter::onViewChanged);
    connect(d->diagramElementsCheckBox, &QCheckBox::clicked, this, &ModelTreeFilter::onViewChanged);
    connect(d->clearFilterButton, &QPushButton::clicked, this, &ModelTreeFilter::clearFilter);
    connect(d->typeComboBox, &QComboBox::currentIndexChanged, this, [this](int index) {
        d->directionComboBox->setDisabled(index != (int) ModelTreeFilterData::Type::Dependency);
    });
    connect(d->typeComboBox, &QComboBox::currentIndexChanged, this, &ModelTreeFilter::onFilterChanged);
    connect(d->stereotypesComboBox, &QComboBox::currentTextChanged, this, &ModelTreeFilter::onFilterChanged);
    connect(d->nameLineEdit, &QLineEdit::textChanged, this, &ModelTreeFilter::onFilterChanged);
    connect(d->directionComboBox, &QComboBox::currentTextChanged, this, &ModelTreeFilter::onFilterChanged);
    setupFilter();
    resetView();
    clearFilter();
}

ModelTreeFilter::~ModelTreeFilter()
{
    delete d;
}

void ModelTreeFilter::setupFilter()
{
    QStringList types = {"Any", "Package", "Component", "Class", "Diagram", "Item",
                         "Dependency", "Association", "Inheritance", "Connection"};
    d->m_typeModel.setStringList(types);
    d->typeComboBox->setModel(&d->m_typeModel);

    QStringList stereotypes = { };
    d->m_stereotypesModel.setStringList(stereotypes);
    d->stereotypesComboBox->setModel(&d->m_stereotypesModel);

    QStringList directions = {"Any", "Outgoing (->)", "Incoming (<-)", "Bidirectional (<->)"};
    d->m_directionModel.setStringList(directions);
    d->directionComboBox->setModel(&d->m_directionModel);
}

void ModelTreeFilter::resetView()
{
    d->relationsCheckBox->setChecked(true);
    d->diagramElementsCheckBox->setChecked(false);
    onViewChanged();
}

void ModelTreeFilter::clearFilter()
{
    d->typeComboBox->setCurrentIndex(0);
    d->stereotypesComboBox->clearEditText();
    d->nameLineEdit->clear();
    d->directionComboBox->setCurrentIndex(0);
    onFilterChanged();
}

void ModelTreeFilter::onViewChanged()
{
    ModelTreeViewData modelTreeViewData;
    modelTreeViewData.setShowRelations(d->relationsCheckBox->isChecked());
    modelTreeViewData.setShowDiagramElements(d->diagramElementsCheckBox->isChecked());
    emit viewDataChanged(modelTreeViewData);
}

void ModelTreeFilter::onFilterChanged()
{
    ModelTreeFilterData modelTreeFilterData;
    modelTreeFilterData.setType((ModelTreeFilterData::Type) d->typeComboBox->currentIndex());
    modelTreeFilterData.setCustomId(QString());
    modelTreeFilterData.setStereotypes(d->stereotypesComboBox->currentText().split(',', Qt::SkipEmptyParts));
    modelTreeFilterData.setName(d->nameLineEdit->text().trimmed());
    if (modelTreeFilterData.type() == ModelTreeFilterData::Type::Dependency)
        modelTreeFilterData.setDirection((ModelTreeFilterData::Direction) d->directionComboBox->currentIndex());
    else
        modelTreeFilterData.setDirection(ModelTreeFilterData::Direction::Any);
    emit filterDataChanged(modelTreeFilterData);
}

} // namespace qmt
