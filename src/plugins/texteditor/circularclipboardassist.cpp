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

#include "circularclipboardassist.h"
#include "codeassist/iassistprovider.h"
#include "codeassist/iassistinterface.h"
#include "codeassist/iassistprocessor.h"
#include "codeassist/iassistproposal.h"
#include "codeassist/basicproposalitem.h"
#include "codeassist/basicproposalitemlistmodel.h"
#include "codeassist/genericproposal.h"
#include "basetexteditor.h"
#include "circularclipboard.h"

#include <coreplugin/coreconstants.h>

#include <QApplication>
#include <QClipboard>

using namespace TextEditor;
using namespace TextEditor::Internal;

namespace TextEditor {
namespace Internal {

class ClipboardProposalItem: public BasicProposalItem {
public:
    enum { maxLen = 80 };

    ClipboardProposalItem(QSharedPointer<const QMimeData> mimeData): m_mimeData(mimeData)
    {
        QString text = mimeData->text().simplified();
        if (text.length() > maxLen) {
            text.truncate(maxLen);
            text.append(QLatin1String("..."));
        }
        setText(text);
    }

    virtual void apply(BaseTextEditor *editor, int /*basePosition*/) const
    {
        BaseTextEditorWidget *editwidget = editor->editorWidget();

        //Move to last in circular clipboard
        if (CircularClipboard * clipboard = CircularClipboard::instance()) {
            clipboard->collect(m_mimeData);
            clipboard->toLastCollect();
        }

        //Copy the selected item
        QApplication::clipboard()->setMimeData(editwidget->duplicateMimeData(m_mimeData.data()));

        //Paste
        editwidget->paste();
    }

private:
    QSharedPointer<const QMimeData> m_mimeData;
};

class ClipboardAssistProcessor: public IAssistProcessor
{
public:
    IAssistProposal *perform(const IAssistInterface *interface)
    {
        if (!interface)
            return 0;

        QScopedPointer<const IAssistInterface> assistInterface(interface);

        QIcon icon = QIcon::fromTheme(QLatin1String("edit-paste"), QIcon(QLatin1String(Core::Constants::ICON_PASTE))).pixmap(16);
        CircularClipboard * clipboard = CircularClipboard::instance();
        QList<BasicProposalItem *> items;
        for (int i = 0; i < clipboard->size(); ++i) {
            QSharedPointer<const QMimeData> data = clipboard->next();

            BasicProposalItem *item = new ClipboardProposalItem(data);
            item->setIcon(icon);
            item->setOrder(clipboard->size() - 1 - i);
            items.append(item);
        }

        return new GenericProposal(interface->position(), new BasicProposalItemListModel(items));
    }
};

} // namespace Internal
} // namespace TextEditor

bool ClipboardAssistProvider::supportsEditor(const Core::Id &/*editorId*/) const
{
    return true;
}

IAssistProcessor *ClipboardAssistProvider::createProcessor() const
{
    return new ClipboardAssistProcessor;
}
