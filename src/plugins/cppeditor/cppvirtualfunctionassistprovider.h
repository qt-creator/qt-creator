/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPFOLLOWVIRTUALSYMBOLS_H
#define CPPFOLLOWVIRTUALSYMBOLS_H

#include <texteditor/codeassist/iassistprovider.h>

#include <cplusplus/CppDocument.h>
#include <cplusplus/Symbols.h>

namespace CppEditor {
namespace Internal {

class VirtualFunctionAssistProvider : public TextEditor::IAssistProvider
{
public:
    VirtualFunctionAssistProvider();

    virtual bool configure(CPlusPlus::Class *startClass, CPlusPlus::Function *function,
        const CPlusPlus::Snapshot &snapshot, bool openInNextSplit);
    CPlusPlus::Class *startClass() const { return m_startClass; }
    CPlusPlus::Function *function() const { return m_function; }
    CPlusPlus::Snapshot snapshot() const { return m_snapshot; }
    bool openInNextSplit() const { return m_openInNextSplit; }

    bool isAsynchronous() const;
    bool supportsEditor(const Core::Id &editorId) const;
    TextEditor::IAssistProcessor *createProcessor() const;

private:
    CPlusPlus::Class *m_startClass;
    CPlusPlus::Function *m_function;
    CPlusPlus::Snapshot m_snapshot;
    bool m_openInNextSplit;
};

class FunctionHelper
{
public:
    static bool isVirtualFunction(CPlusPlus::Function *function,
        const CPlusPlus::Snapshot &snapshot);

    static bool isPureVirtualFunction(CPlusPlus::Function *function,
        const CPlusPlus::Snapshot &snapshot);

    static QList<CPlusPlus::Symbol *> overrides(CPlusPlus::Class *startClass,
        CPlusPlus::Function *function, const CPlusPlus::Snapshot &snapshot);
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPFOLLOWVIRTUALSYMBOLS_H
