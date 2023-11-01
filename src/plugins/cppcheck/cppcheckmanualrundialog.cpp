// Copyright (C) 2019 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcheckmanualrundialog.h"

#include "cppchecksettings.h"
#include "cppchecktr.h"

#include <projectexplorer/selectablefilesmodel.h>

#include <cppeditor/projectinfo.h>

#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

namespace Cppcheck::Internal {

ManualRunDialog::ManualRunDialog(const ProjectExplorer::Project *project)
    : m_model(new ProjectExplorer::SelectableFilesFromDirModel(this))
{
    QTC_ASSERT(project, return );

    setWindowTitle(Tr::tr("Cppcheck Run Configuration"));

    auto view = new QTreeView;
    view->setHeaderHidden(true);
    view->setModel(m_model);

    connect(m_model, &ProjectExplorer::SelectableFilesFromDirModel::parsingFinished,
            view, [this, view] {
                m_model->applyFilter("*.cpp;*.cxx;*.c;*.cc;*.C", {});
                view->expandToDepth(0);
            });
    m_model->startParsing(project->rootProjectDirectory());

    auto buttons = new QDialogButtonBox;
    buttons->setStandardButtons(QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted,
            this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    auto analyzeButton = new QPushButton(Tr::tr("Analyze"));
    buttons->addButton(analyzeButton, QDialogButtonBox::AcceptRole);
    analyzeButton->setEnabled(m_model->hasCheckedFiles());
    connect(m_model, &QAbstractItemModel::dataChanged,
            analyzeButton, [this, analyzeButton]() {
        analyzeButton->setEnabled(m_model->hasCheckedFiles());
    });

    auto optionsWidget = settings().layouter()().emerge();

    auto layout = new QVBoxLayout(this);
    layout->addWidget(optionsWidget);
    layout->addWidget(view);
    layout->addWidget(buttons);

    if (auto layout = optionsWidget->layout())
        layout->setContentsMargins(0, 0, 0, 0);
}

Utils::FilePaths ManualRunDialog::filePaths() const
{
    return m_model->selectedFiles();
}

QSize ManualRunDialog::sizeHint() const
{
    const auto original = QDialog::sizeHint();
    return {original.width() * 2, original.height()};
}

} // Cppcheck::Internal
