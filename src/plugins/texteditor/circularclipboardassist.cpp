/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "circularclipboardassist.h"
#include "codeassist/assistinterface.h"
#include "codeassist/iassistprocessor.h"
#include "codeassist/iassistproposal.h"
#include "codeassist/assistproposalitem.h"
#include "codeassist/genericproposalmodel.h"
#include "codeassist/genericproposal.h"
#include "texteditor.h"
#include "circularclipboard.h"

#include <coreplugin/coreconstants.h>


#include <QApplication>
#include <QClipboard>

namespace TextEditor {
namespace Internal {

class ClipboardProposalItem: public AssistProposalItem
{
public:
    enum { maxLen = 80 };

    ClipboardProposalItem(QSharedPointer<const QMimeData> mimeData)
        : m_mimeData(mimeData)
    {
        QString text = mimeData->text().simplified();
        if (text.length() > maxLen) {
            text.truncate(maxLen);
            text.append(QLatin1String("..."));
        }
        setText(text);
    }

    void apply(TextEditorWidget *editorWidget, int /*basePosition*/) const override
    {

        //Move to last in circular clipboard
        if (CircularClipboard * clipboard = CircularClipboard::instance()) {
            clipboard->collect(m_mimeData);
            clipboard->toLastCollect();
        }

        //Copy the selected item
        QApplication::clipboard()->setMimeData(
                    TextEditorWidget::duplicateMimeData(m_mimeData.data()));

        //Paste
        editorWidget->paste();
    }

private:
    QSharedPointer<const QMimeData> m_mimeData;
};

class ClipboardAssistProcessor: public IAssistProcessor
{
public:
    IAssistProposal *perform(const AssistInterface *interface) override
    {
        if (!interface)
            return 0;

        QIcon icon = QIcon::fromTheme(QLatin1String("edit-paste"), QIcon(QLatin1String(Core::Constants::ICON_PASTE))).pixmap(16);
        CircularClipboard * clipboard = CircularClipboard::instance();
        QList<AssistProposalItem *> items;
        for (int i = 0; i < clipboard->size(); ++i) {
            QSharedPointer<const QMimeData> data = clipboard->next();

            AssistProposalItem *item = new ClipboardProposalItem(data);
            item->setIcon(icon);
            item->setOrder(clipboard->size() - 1 - i);
            items.append(item);
        }

        return new GenericProposal(interface->position(), items);
    }
};

IAssistProvider::RunType ClipboardAssistProvider::runType() const
{
    return Synchronous;
}

bool ClipboardAssistProvider::supportsEditor(Core::Id /*editorId*/) const
{
    return true;
}

IAssistProcessor *ClipboardAssistProvider::createProcessor() const
{
    return new ClipboardAssistProcessor;
}

} // namespace Internal
} // namespace TextEditor
