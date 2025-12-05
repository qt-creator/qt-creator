// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../common/themeselector.h"

#include <projectexplorer/runconfigurationaspects.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/layoutbuilder.h>

#include <QApplication>
#include <QCheckBox>
#include <QPushButton>
#include <QToolButton>

using namespace Layouting;

struct AspectUI : public Column
{
    AspectUI(Utils::BaseAspect *aspect, bool multipleInstances = false)
    {
        QLabel *volatileLabel = new QLabel;
        QLabel *valueLabel = new QLabel;
        QLabel *storeLabel = new QLabel;
        QCheckBox *autoApplyCheck = new QCheckBox();

        autoApplyCheck->setText("Auto Apply:");
        autoApplyCheck->setChecked(aspect->isAutoApply());
        QObject::connect(
            autoApplyCheck, &QCheckBox::toggled, aspect, &Utils::BaseAspect::setAutoApply);

        QPushButton *applyBtn = new QPushButton();
        applyBtn->setText("Apply");

        auto undoStack = new QUndoStack(aspect);
        aspect->setUndoStack(undoStack);

        QToolButton *undoBtn = new QToolButton();
        undoBtn->setDefaultAction(undoStack->createUndoAction(undoBtn));

        QToolButton *redoBtn = new QToolButton();
        redoBtn->setDefaultAction(undoStack->createRedoAction(redoBtn));

        const auto updateWidgets = [aspect, applyBtn, volatileLabel, valueLabel, storeLabel]() {
            applyBtn->setEnabled(aspect->isDirty());
            volatileLabel->setText(QString("%1").arg(aspect->volatileVariantValue().toString()));
            valueLabel->setText(QString("%1").arg(aspect->variantValue().toString()));
            Utils::Store map;
            aspect->toMap(map);
            storeLabel->setText(QString::fromUtf8(jsonFromStore(map)));
        };

        QObject::connect(
            aspect, &Utils::BaseAspect::volatileValueChanged, volatileLabel, updateWidgets);
        QObject::connect(aspect, &Utils::BaseAspect::changed, valueLabel, updateWidgets);

        QObject::connect(applyBtn, &QPushButton::clicked, aspect, [updateWidgets, aspect]() {
            aspect->apply();
            updateWidgets();
        });

        // clang-format off
        this->addItems({
            [multipleInstances, aspect] {
                if (multipleInstances) {
                    return Column {
                        noMargin,
                        Group {
                            sizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Maximum}),
                            Column {
                                "First instance:", aspect
                            }
                        },
                        Group {
                            sizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Maximum}),
                            Column {
                                "Second instance:", aspect
                            }
                        }
                    }.emerge();
                }
                return Group {
                    sizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Maximum}),
                    Column {
                        "Single instance:",
                        aspect
                    }
                }.emerge();
            },
            Widget {
                sizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed}),
                Column {
                    noMargin,
                    Group { Column {  "Volatile Value:", volatileLabel } },
                    Group { Column { "Value:", valueLabel } },
                }
            },
            Group {
                sizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed}),
                Column {
                    noMargin,
                    "Store Representation:",
                    storeLabel,
                },
            },
            st,
            Row {
                undoBtn,
                redoBtn,
                autoApplyCheck,
                applyBtn,
                st
            },
        });
        // clang-format on

        updateWidgets();
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    auto selectionAspect = [] {
        auto selectionAspect = new Utils::SelectionAspect();
        selectionAspect->setSettingsKey("SelectionAspectId");
        selectionAspect->addOption("Option 1", "This is option 1");
        selectionAspect->addOption("Option 2", "This is option 2");
        selectionAspect->addOption("Option 3", "This is option 3");
        return selectionAspect;
    };

    auto comboSelectionAspect = [] {
        auto selectionAspect = new Utils::SelectionAspect();
        selectionAspect->setSettingsKey("SelectionAspectId");
        selectionAspect->setDisplayStyle(Utils::SelectionAspect::DisplayStyle::ComboBox);
        selectionAspect->addOption("Option 1", "This is option 1");
        selectionAspect->addOption("Option 2", "This is option 2");
        selectionAspect->addOption("Option 3", "This is option 3");
        return selectionAspect;
    };

    auto stringAspect = [] {
        auto stringAspect = new Utils::StringAspect();
        stringAspect->setSettingsKey("StringAspectId");
        stringAspect->setDisplayStyle(Utils::StringAspect::DisplayStyle::LineEditDisplay);
        stringAspect->setDefaultValue("Default Value");
        stringAspect->setPlaceHolderText("Placeholder text");
        return stringAspect;
    };

    auto integerAspect = [] {
        auto integerAspect = new Utils::IntegerAspect();
        integerAspect->setSettingsKey("IntegerAspectId");
        integerAspect->setRange(-10, 10);
        integerAspect->setDefaultValue(0);
        return integerAspect;
    };

    auto runAsAspect = [] {
        auto runAsAspect = new ProjectExplorer::RunAsAspect();
        runAsAspect->setSettingsKey("RunAsAspectId");
        return runAsAspect;
    };

    auto themeSelector = new ManualTest::ThemeSelector;

    // clang-format off
    TabWidget {
        Tab {
            "Selection Aspect",
            AspectUI(selectionAspect(), true)
        },
        Tab {
            "Combo Selection Aspect",
            AspectUI(comboSelectionAspect(), true)
        },
        Tab {
            "String Aspect",
            AspectUI(stringAspect(), true)
        },
        Tab {
            "Integer Aspect",
            AspectUI(integerAspect(), false)
        },
        Tab {
            "Run As Aspect",
            AspectUI(runAsAspect(), true)
        },
        Tab {
            "Theme",
            Column {
                themeSelector
            }
        }
    }.show();
    // clang-format on

    return app.exec();
}
