// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "formwindoweditor.h"
#include "formwindowfile.h"
#include "designerconstants.h"
#include "resourcehandler.h"

#include <coreplugin/coreconstants.h>
#include <texteditor/textdocument.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

namespace Designer {

using namespace Internal;

FormWindowEditor::FormWindowEditor()
{
    addContext(Designer::Constants::K_DESIGNER_XML_EDITOR_ID);
    addContext(Designer::Constants::C_DESIGNER_XML_EDITOR);
}

FormWindowEditor::~FormWindowEditor() = default;

QWidget *FormWindowEditor::toolBar()
{
    return nullptr;
}

QString FormWindowEditor::contents() const
{
    return formWindowFile()->formWindowContents();
}

FormWindowFile *FormWindowEditor::formWindowFile() const
{
    return qobject_cast<FormWindowFile *>(textDocument());
}

bool FormWindowEditor::isDesignModePreferred() const
{
    return true;
}

} // namespace Designer

