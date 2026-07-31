// Copyright (C) 2019 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcheckmanualrundialog.h"

#include "cppchecksettings.h"
#include "cppchecktr.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/selectablefilesmodel.h>

#include <utils/filepath.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QTreeView>

namespace Cppcheck::Internal {

ManualRunDialog::ManualRunDialog(const ProjectExplorer::Project *project,
                                 CppcheckSettings *settings)
    : m_model(new ProjectExplorer::SelectableFilesFromDirModel(this))
{
    QTC_ASSERT(project, return);
    QTC_ASSERT(settings, return);

    setWindowTitle(Tr::tr("Cppcheck Run Configuration"));

    // The directory-based tree below can only show files under the project
    // directory. Also pick up the project's source files that live outside it
    // (e.g. libraries pulled in from a sibling directory); the model keeps such
    // initially marked, out-of-base files in the result. (QTCREATORBUG-25416)
    const Utils::FilePath baseDir = project->rootProjectDirectory();
    static const QStringList sourceSuffixes = {"cpp", "cxx", "cc", "c", "C"};
    const Utils::FilePaths externalFiles
        = project->files([&baseDir](const ProjectExplorer::Node *node) {
              if (!ProjectExplorer::Project::SourceFiles(node))
                  return false;
              const Utils::FilePath path = node->filePath();
              return !path.isChildOf(baseDir) && sourceSuffixes.contains(path.suffix());
          });
    m_model->setInitialMarkedFiles(externalFiles);

    auto view = new QTreeView;
    view->setHeaderHidden(true);
    view->setModel(m_model);

    connect(m_model, &ProjectExplorer::SelectableFilesFromDirModel::parsingFinished,
            view, [this, view] {
                m_model->applyFilter("*.cpp;*.cxx;*.c;*.cc;*.C", {});
                view->expandToDepth(0);
            });
    m_model->startParsing(baseDir);

    auto externalLabel = new QLabel;
    if (externalFiles.isEmpty()) {
        externalLabel->hide();
    } else {
        externalLabel->setText(Tr::tr("Also analyzing %n source file(s) outside the project "
                                      "directory.", nullptr, int(externalFiles.size())));
        externalLabel->setWordWrap(true);
    }

    auto buttons = new QDialogButtonBox;
    buttons->setStandardButtons(QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted,
            this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    const bool hasExternalFiles = !externalFiles.isEmpty();
    auto analyzeButton = new QPushButton(Tr::tr("Analyze"));
    buttons->addButton(analyzeButton, QDialogButtonBox::AcceptRole);
    analyzeButton->setEnabled(m_model->hasCheckedFiles() || hasExternalFiles);
    connect(m_model, &QAbstractItemModel::dataChanged,
            analyzeButton, [this, analyzeButton, hasExternalFiles]() {
        analyzeButton->setEnabled(m_model->hasCheckedFiles() || hasExternalFiles);
    });

    auto optionsWidget = settings->layouter()().emerge();

    auto layout = new QVBoxLayout(this);
    layout->addWidget(optionsWidget);
    layout->addWidget(view);
    layout->addWidget(externalLabel);
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
