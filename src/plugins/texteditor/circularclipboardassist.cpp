// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "circularclipboardassist.h"
#include "codeassist/assistinterface.h"
#include "codeassist/iassistprocessor.h"
#include "codeassist/iassistproposal.h"
#include "codeassist/assistproposalitem.h"
#include "codeassist/genericproposal.h"
#include "texteditor.h"
#include "circularclipboard.h"

#include <coreplugin/coreconstants.h>

#include <utils/icon.h>

#include <QApplication>
#include <QClipboard>

using namespace Utils;

namespace TextEditor::Internal {

class ClipboardProposalItem final : public AssistProposalItem
{
public:
    enum { maxLen = 80 };

    ClipboardProposalItem(std::shared_ptr<const QMimeData> mimeData)
        : m_mimeData(mimeData)
    {
        QString text = mimeData->text().simplified();
        if (text.length() > maxLen) {
            text.truncate(maxLen);
            text.append(QLatin1String("..."));
        }
        setText(text);
    }

    void apply(TextEditorWidget *editorWidget, int /*basePosition*/) const final
    {
        QTC_ASSERT(editorWidget, return);

        //Move to last in circular clipboard
        if (CircularClipboard * clipboard = CircularClipboard::instance()) {
            clipboard->collect(m_mimeData);
            clipboard->toLastCollect();
        }

        //Copy the selected item
        QApplication::clipboard()->setMimeData(
                    TextEditorWidget::duplicateMimeData(m_mimeData.get()));

        //Paste
        editorWidget->paste();
    }

private:
    std::shared_ptr<const QMimeData> m_mimeData;
};

class ClipboardAssistProcessor final : public IAssistProcessor
{
public:
    IAssistProposal *perform() final
    {
        QIcon icon = Icon::fromTheme("edit-paste");
        CircularClipboard * clipboard = CircularClipboard::instance();
        QList<AssistProposalItemInterface *> items;
        items.reserve(clipboard->size());
        for (int i = 0; i < clipboard->size(); ++i) {
            std::shared_ptr<const QMimeData> data = clipboard->next();

            AssistProposalItem *item = new ClipboardProposalItem(data);
            item->setIcon(icon);
            item->setOrder(clipboard->size() - 1 - i);
            items.append(item);
        }

        return new GenericProposal(interface()->position(), items);
    }
};

class ClipboardAssistProvider final : public IAssistProvider
{
public:
    IAssistProcessor *createProcessor(const AssistInterface *) const final
    {
        return new ClipboardAssistProcessor;
    }
};

IAssistProvider &clipboardAssistProvider()
{
    static ClipboardAssistProvider theClipboardAssistProvider;
    return theClipboardAssistProvider;
}

} // TextEditor::Internal
