// Copyright (C) 2019 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcheckmanualrundialog.h"
#include "cppcheckoptions.h"

#include <projectexplorer/selectablefilesmodel.h>

#include <cppeditor/projectinfo.h>

#include <utils/qtcassert.h>

#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

namespace Cppcheck {
namespace Internal {

ManualRunDialog::ManualRunDialog(const CppcheckOptions &options,
                                 const ProjectExplorer::Project *project)
    : QDialog(),
    m_options(new OptionsWidget(this)),
    m_model(new ProjectExplorer::SelectableFilesFromDirModel(this))
{
    QTC_ASSERT(project, return );

    setWindowTitle(tr("Cppcheck Run Configuration"));

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

    auto analyzeButton = new QPushButton(tr("Analyze"));
    buttons->addButton(analyzeButton, QDialogButtonBox::AcceptRole);
    analyzeButton->setEnabled(m_model->hasCheckedFiles());
    connect(m_model, &QAbstractItemModel::dataChanged,
            analyzeButton, [this, analyzeButton]() {
        analyzeButton->setEnabled(m_model->hasCheckedFiles());
    });

    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_options);
    layout->addWidget(view);
    layout->addWidget(buttons);

    if (auto layout = m_options->layout())
        layout->setContentsMargins(0, 0, 0, 0);

    m_options->load(options);
}

CppcheckOptions ManualRunDialog::options() const
{
    CppcheckOptions result;
    m_options->save(result);
    return result;
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

} // namespace Internal
} // namespace Cppcheck
