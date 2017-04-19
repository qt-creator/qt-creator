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

#include <utils/utilsicons.h>

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

    ~ClipboardProposalItem() Q_DECL_NOEXCEPT {}

    void apply(TextDocumentManipulatorInterface &manipulator, int /*basePosition*/) const override
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
        manipulator.paste();
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
        const QScopedPointer<const AssistInterface> AssistInterface(interface);

        QIcon icon = QIcon::fromTheme(QLatin1String("edit-paste"), Utils::Icons::PASTE.icon()).pixmap(16);
        CircularClipboard * clipboard = CircularClipboard::instance();
        QList<AssistProposalItemInterface *> items;
        items.reserve(clipboard->size());
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
