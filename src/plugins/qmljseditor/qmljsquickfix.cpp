/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmljsquickfix.h"
#include "qmljseditor.h"
#include <QtCore/QDebug>

using namespace QmlJSEditor::Internal;

QmlJSQuickFixCollector::QmlJSQuickFixCollector()
{
}

QmlJSQuickFixCollector::~QmlJSQuickFixCollector()
{
}

TextEditor::QuickFixState *QmlJSQuickFixCollector::initializeCompletion(TextEditor::ITextEditable *editable)
{
    if (QmlJSTextEditor *editor = qobject_cast<QmlJSTextEditor *>(editable->widget())) {
        const SemanticInfo info = editor->semanticInfo();

        if (editor->isOutdated()) {
            // outdated
            qWarning() << "TODO: outdated semantic info, force a reparse.";
            return 0;
        }

        // ### TODO create the quickfix state
        return 0;
    }

    return 0;
}

QList<TextEditor::QuickFixOperation::Ptr> QmlJSQuickFixCollector::quickFixOperations(TextEditor::BaseTextEditor *) const
{
    QList<TextEditor::QuickFixOperation::Ptr> quickFixOperations;

    return quickFixOperations;
}
