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

#include "qmljseditoreditable.h"
#include "qmljseditor.h"
#include "qmljseditorconstants.h"

#include <qmljstools/qmljstoolsconstants.h>
#include <texteditor/texteditorconstants.h>
#include <qmldesigner/qmldesignerconstants.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <coreplugin/mimedatabase.h>
#include <coreplugin/icore.h>
#include <coreplugin/designmode.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/coreconstants.h>

namespace QmlJSEditor {

QmlJSEditorEditable::QmlJSEditorEditable(QmlJSTextEditorWidget *editor)
    : BaseTextEditor(editor)
{
    m_context.add(QmlJSEditor::Constants::C_QMLJSEDITOR_ID);
    m_context.add(TextEditor::Constants::C_TEXTEDITOR);
    m_context.add(ProjectExplorer::Constants::LANG_QMLJS);
}

// Use preferred mode from Bauhaus settings
static bool openInDesignMode()
{
    static bool bauhausDetected = false;
    static bool bauhausPresent = false;
    // Check if Bauhaus is loaded, that is, a Design mode widget is
    // registered for the QML mime type.
    if (!bauhausDetected) {
        if (const Core::IMode *dm = Core::ModeManager::mode(Core::Constants::MODE_DESIGN))
            if (const Core::DesignMode *designMode = qobject_cast<const Core::DesignMode *>(dm))
                bauhausPresent = designMode->registeredMimeTypes().contains(QLatin1String(QmlJSTools::Constants::QML_MIMETYPE));
        bauhausDetected =  true;
    }
    if (!bauhausPresent)
        return false;

    return bool(QmlDesigner::Constants::QML_OPENDESIGNMODE_DEFAULT);
}

Core::Id QmlJSEditorEditable::preferredModeType() const
{
    Core::IMode *mode = Core::ModeManager::currentMode();
    if (mode && (mode->type() == Core::Constants::MODE_DESIGN_TYPE
                || mode->type() == Core::Constants::MODE_EDIT_TYPE))
    {
        return mode->type();
    }

    // if we are in other mode than edit or design, use the hard-coded default.
    // because the editor opening decision is modal, it would be confusing to
    // have the user also access to this failsafe setting.
    if (editorWidget()->mimeType() == QLatin1String(QmlJSTools::Constants::QML_MIMETYPE)
        && openInDesignMode())
        return Core::Id(Core::Constants::MODE_DESIGN_TYPE);
    return Core::Id();
}

void QmlJSEditorEditable::setTextCodec(QTextCodec *codec, TextCodecReason reason)
{
    if (reason != TextCodecOtherReason) // qml is defined to be utf8
        return;
    editorWidget()->setTextCodec(codec);
}

} // namespace QmlJSEditor
