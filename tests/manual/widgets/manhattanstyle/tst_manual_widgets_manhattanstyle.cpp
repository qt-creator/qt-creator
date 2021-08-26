/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QApplication>
#include <QPushButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QFormLayout>
#include <QLayout>
#include <QListWidget>
#include <QMenuBar>
#include <QMenu>
#include <QMainWindow>
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif // Qt < 6

    QApplication app(argc, argv);

    QMainWindow window;
    window.setCentralWidget(centralWidget());
    addMenu(window.menuBar());

    window.resize(QSize());
    window.show();

    return app.exec();
}
