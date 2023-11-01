// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpppreprocessordialog.h"

#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cpptoolsreuse.h"

#include <coreplugin/session.h>

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
    setWindowTitle(Tr::tr("Additional C++ Preprocessor Directives"));

    const Key key = Constants::EXTRA_PREPROCESSOR_DIRECTIVES + keyFromString(m_filePath.toString());
    const QString directives = Core::SessionManager::value(key).toString();

    m_editWidget = new TextEditor::SnippetEditorWidget;
    m_editWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_editWidget->setPlainText(directives);
    decorateCppEditor(m_editWidget);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    using namespace Layouting;

    Column {
        Tr::tr("Additional C++ Preprocessor Directives for %1:").arg(m_filePath.fileName()),
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

    const Key key = Constants::EXTRA_PREPROCESSOR_DIRECTIVES + keyFromString(m_filePath.toString());
    Core::SessionManager::setValue(key, extraPreprocessorDirectives());

    return Accepted;
}

QString CppPreProcessorDialog::extraPreprocessorDirectives() const
{
    return m_editWidget->toPlainText();
}

} // CppEditor::Internal
