// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>
#include <QPushButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QLayout>
#include <QListWidget>
#include <QMenuBar>
#include <QMenu>
#include <QMainWindow>
#include <QMessageBox>
#include <QScrollBar>
#include <QSpinBox>

#include <utils/utilsicons.h>
#include <utils/fancylineedit.h>
#include <utils/styledbar.h>

#include "../common/themeselector.h"

static void addMenu(QMenuBar *root)
{
    QMenu *fileMenu = root->addMenu("&File");
    fileMenu->addAction(Utils::Icons::NEWFILE.icon(), "&New File or Project...");
    QMenu *recentFiles = fileMenu->addMenu("Recent &Files");
    recentFiles->addActions({new QAction("File One"), new QAction("File Two")});
    fileMenu->addAction(Utils::Icons::SAVEFILE.icon(), "&Save File...")->setDisabled(true);
    root->addAction("&Edit")->setDisabled(true);
}

static QWidget *centralWidget()
{
    auto widget = new QWidget;
    auto rootLayout = new QVBoxLayout(widget);
    rootLayout->addWidget(new ManualTest::ThemeSelector);
    auto enabledDisabledLayout = new QHBoxLayout(widget);
    rootLayout->addLayout(enabledDisabledLayout);
    for (bool enabled : {true, false}) {
        auto groupBox = new QGroupBox("GroupBox");
        enabledDisabledLayout->addWidget(groupBox);
        auto vBoxLayout = new QVBoxLayout;

        auto lineEdit = new Utils::FancyLineEdit;
        lineEdit->setText("FancyLineEdit");
        lineEdit->setFiltering(true);
        vBoxLayout->addWidget(lineEdit);

        vBoxLayout->addWidget(new QCheckBox("CheckBox"));
        auto checkedCheckButton = new QCheckBox("CheckBox (checked)");
        checkedCheckButton->setChecked(true);
        vBoxLayout->addWidget(checkedCheckButton);
        auto partiallyCheckButton = new QCheckBox("CheckBox (partially)");
        partiallyCheckButton->setTristate(true);
        partiallyCheckButton->setCheckState(Qt::PartiallyChecked);
        vBoxLayout->addWidget(partiallyCheckButton);

        vBoxLayout->addWidget(new QRadioButton("RadioButton"));
        auto checkedRadioButton = new QRadioButton("RadioButton (checked)");
        checkedRadioButton->setChecked(true);
        vBoxLayout->addWidget(checkedRadioButton);

        auto comboBox = new QComboBox;
        comboBox->addItems({"ComboBox", "Foo", "Bar"});
        comboBox->setEditable(true);
        vBoxLayout->addWidget(comboBox);

        auto spinBox = new QSpinBox;
        vBoxLayout->addWidget(spinBox);

        auto listWidget = new QListWidget;
        listWidget->addItems({"Item 1", "Item 2"});
        listWidget->setMinimumHeight(44);
        vBoxLayout->addWidget(listWidget);

        auto tabWidget = new QTabWidget;
        tabWidget->addTab(new QWidget, "T1");
        tabWidget->tabBar()->setTabsClosable(true);
        tabWidget->addTab(new QWidget, "T2");
        vBoxLayout->addWidget(tabWidget);

        auto hScrollBar = new QScrollBar(Qt::Horizontal);
        vBoxLayout->addWidget(hScrollBar);

        auto groupBoxLayout = new QHBoxLayout(groupBox);
        groupBoxLayout->addLayout(vBoxLayout);

        auto vScrollBar = new QScrollBar(Qt::Vertical);
        groupBoxLayout->addWidget(vScrollBar);

        groupBox->setEnabled(enabled);
    }
    return widget;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QMainWindow window;
    window.setCentralWidget(centralWidget());
    addMenu(window.menuBar());

    window.resize(QSize());
    window.show();

    return app.exec();
}
