/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "quickfix.h"
#include "basetexteditor.h"

#include <coreplugin/ifile.h>
#include <extensionsystem/pluginmanager.h>
#include <QtGui/QApplication>
#include <QtGui/QTextBlock>

#include <QtCore/QDebug>

using namespace TextEditor;

QuickFixState::QuickFixState(TextEditor::BaseTextEditor *editor)
    : _editor(editor)
{
}

QuickFixState::~QuickFixState()
{
}

TextEditor::BaseTextEditor *QuickFixState::editor() const
{
    return _editor;
}


QuickFixOperation::QuickFixOperation(int priority)
{
    setPriority(priority);
}

QuickFixOperation::~QuickFixOperation()
{
}

int QuickFixOperation::priority() const
{
    return _priority;
}

void QuickFixOperation::setPriority(int priority)
{
    _priority = priority;
}

QString QuickFixOperation::description() const
{
    return _description;
}

void QuickFixOperation::setDescription(const QString &description)
{
    _description = description;
}



QuickFixFactory::QuickFixFactory(QObject *parent)
    : QObject(parent)
{
}

QuickFixFactory::~QuickFixFactory()
{
}

QuickFixCollector::QuickFixCollector()
    : _editable(0)
{
}

QuickFixCollector::~QuickFixCollector()
{
}

TextEditor::ITextEditable *QuickFixCollector::editor() const
{
    return _editable;
}

int QuickFixCollector::startPosition() const
{
    return _editable->position();
}

bool QuickFixCollector::triggersCompletion(TextEditor::ITextEditable *)
{
    return false;
}

int QuickFixCollector::startCompletion(TextEditor::ITextEditable *editable)
{
    Q_ASSERT(editable != 0);

    _editable = editable;
    TextEditor::BaseTextEditor *editor = qobject_cast<TextEditor::BaseTextEditor *>(editable->widget());
    Q_ASSERT(editor != 0);

    if (TextEditor::QuickFixState *state = initializeCompletion(editor)) {
        QMap<int, QList<QuickFixOperation::Ptr> > matchedOps;

        foreach (QuickFixFactory *factory, quickFixFactories()) {
            QList<QuickFixOperation::Ptr> ops = factory->matchingOperations(state);

            foreach (QuickFixOperation::Ptr op, ops) {
                const int priority = op->priority();
                if (priority != -1)
                    matchedOps[priority].append(op);
            }
        }

        QMapIterator<int, QList<TextEditor::QuickFixOperation::Ptr> > it(matchedOps);
        it.toBack();
        if (it.hasPrevious()) {
            it.previous();
            _quickFixes = it.value();
        }

        delete state;

        if (! _quickFixes.isEmpty())
            return editable->position();
    }

    return -1;
}

void QuickFixCollector::completions(QList<TextEditor::CompletionItem> *quickFixItems)
{
    for (int i = 0; i < _quickFixes.size(); ++i) {
        TextEditor::QuickFixOperation::Ptr op = _quickFixes.at(i);

        TextEditor::CompletionItem item(this);
        item.text = op->description();
        item.data = QVariant::fromValue(i);
        quickFixItems->append(item);
    }
}

void QuickFixCollector::fix(const TextEditor::CompletionItem &item)
{
    const int index = item.data.toInt();

    if (index < _quickFixes.size()) {
        TextEditor::QuickFixOperation::Ptr quickFix = _quickFixes.at(index);
        quickFix->perform();
    }
}

void QuickFixCollector::cleanup()
{
    _quickFixes.clear();
}
