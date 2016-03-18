/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include <texteditor/codeassist/iassistprovider.h>

#include <cplusplus/CppDocument.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/TypeOfExpression.h>

#include <QSharedPointer>
#include <QTextCursor>

namespace CppEditor {
namespace Internal {

class VirtualFunctionAssistProvider : public TextEditor::IAssistProvider
{
public:
    VirtualFunctionAssistProvider();

    struct Parameters {
        Parameters() : function(0), staticClass(0), cursorPosition(-1), openInNextSplit(false) {}

        CPlusPlus::Function *function;
        CPlusPlus::Class *staticClass;
        QSharedPointer<CPlusPlus::TypeOfExpression> typeOfExpression; // Keeps instantiated symbols.
        CPlusPlus::Snapshot snapshot;
        int cursorPosition;
        bool openInNextSplit;
    };

    virtual bool configure(const Parameters &parameters);
    Parameters params() const { return m_params; }
    void clearParams() { m_params = Parameters(); }

    IAssistProvider::RunType runType() const override;
    bool supportsEditor(Core::Id editorId) const override;
    TextEditor::IAssistProcessor *createProcessor() const override;

private:
    Parameters m_params;
};

} // namespace Internal
} // namespace CppEditor
