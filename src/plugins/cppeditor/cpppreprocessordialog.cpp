// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpppreprocessordialog.h"

#include "cppeditorconstants.h"
#include "cpptoolsreuse.h"

#include <projectexplorer/session.h>

#include <texteditor/snippets/snippeteditor.h>

#include <utils/layoutbuilder.h>

#include <QDialogButtonBox>

using namespace Utils;

namespace CppEditor::Internal {

CppPreProcessorDialog::CppPreProcessorDialog(const FilePath &filePath, QWidget *parent)
    : QDialog(parent)
    , m_filePath(filePath)
{
    resize(400, 300);
    setWindowTitle(tr("Additional C++ Preprocessor Directives"));

    const QString key = Constants::EXTRA_PREPROCESSOR_DIRECTIVES + m_filePath.toString();
    const QString directives = ProjectExplorer::SessionManager::value(key).toString();

    m_editWidget = new TextEditor::SnippetEditorWidget;
    m_editWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_editWidget->setPlainText(directives);
    decorateCppEditor(m_editWidget);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    using namespace Layouting;

    Column {
        tr("Additional C++ Preprocessor Directives for %1:").arg(m_filePath.fileName()),
        m_editWidget,
        buttonBox,
    }.attachTo(this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

CppPreProcessorDialog::~CppPreProcessorDialog() = default;

int CppPreProcessorDialog::exec()
{
    if (QDialog::exec() == Rejected)
        return Rejected;

    const QString key = Constants::EXTRA_PREPROCESSOR_DIRECTIVES + m_filePath.toString();
    ProjectExplorer::SessionManager::setValue(key, extraPreprocessorDirectives());

    return Accepted;
}

QString CppPreProcessorDialog::extraPreprocessorDirectives() const
{
    return m_editWidget->toPlainText();
}

} // CppEditor::Internal
