/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "nimcompletionassistprovider.h"
#include "suggest/nimsuggestcache.h"
#include "suggest/nimsuggest.h"

#include <projectexplorer/session.h>
#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>
#include <utils/textutils.h>
#include <utils/algorithm.h>

#include <QApplication>
#include <QDebug>
#include <QSet>
#include <QTemporaryFile>
#include <QTextStream>
#include <QTextDocument>
#include <QThread>

using namespace ProjectExplorer;
using namespace TextEditor;

namespace Nim {

bool isIdentifierChar(QChar c)
{
    return c.isLetterOrNumber() || c == QLatin1Char('_');
}

bool isActivationChar(QChar c)
{
    static const QSet<QChar> chars {QLatin1Char('.'), QLatin1Char('(')};
    return chars.contains(c);
}

class NimCompletionAssistProcessor : public QObject, public TextEditor::IAssistProcessor
{
    Q_OBJECT

public:
    TextEditor::IAssistProposal *perform(const TextEditor::AssistInterface *interface) final
    {
        QTC_ASSERT(this->thread() == qApp->thread(), return nullptr);

        if (interface->reason() == IdleEditor && !acceptsIdleEditor(interface))
            return nullptr;

        Suggest::NimSuggest *suggest = nimSuggestInstance(interface);
        QTC_ASSERT(suggest, return nullptr);

        if (suggest->executablePath().isEmpty() || suggest->projectFile().isEmpty())
            return nullptr;

        if (!suggest->isReady()) {
            m_interface = interface;
            QObject::connect(suggest, &Suggest::NimSuggest::readyChanged,
                             this, &NimCompletionAssistProcessor::onNimSuggestReady);
        } else {
            doPerform(interface, suggest);
        }

        m_running = true;
        return nullptr;
    }

    bool running()
    {
        return m_running;
    }

private:
    void onNimSuggestReady(bool ready)
    {
        auto suggest = dynamic_cast<Suggest::NimSuggest *>(sender());
        QTC_ASSERT(suggest, return);
        QTC_ASSERT(m_interface, return);

        if (!ready || !suggest) {
            m_running = false;
            setAsyncProposalAvailable(nullptr);
        } else {
            doPerform(m_interface, suggest);
        }
    }

    void doPerform(const TextEditor::AssistInterface *interface,
                   Suggest::NimSuggest *suggest)
    {
        int pos = findCompletionPos(interface);

        std::unique_ptr<QTemporaryFile> dirtyFile = writeDirtyFile(interface);
        QTC_ASSERT(dirtyFile, return);

        std::shared_ptr<Suggest::NimSuggestClientRequest> request = sendRequest(interface,
                                                                   suggest,
                                                                   dirtyFile->fileName(),
                                                                   pos);
        QTC_ASSERT(request, return);
        connect(request.get(), &Suggest::NimSuggestClientRequest::finished, this,
                &NimCompletionAssistProcessor::onRequestFinished);

        m_pos = pos;
        m_dirtyFile = std::move(dirtyFile);
        m_request = std::move(request);
    }

    void onRequestFinished()
    {
        auto items = Utils::transform<QList>(m_request->lines(), &createProposal);
        setAsyncProposalAvailable(new GenericProposal(m_pos, items));
        m_running = false;
        m_dirtyFile.reset();
        m_request.reset();
    }

    static bool acceptsIdleEditor(const AssistInterface *interface)
    {
        int pos = interface->position();
        QChar c = interface->textDocument()->characterAt(pos - 1);
        return isIdentifierChar(c) || isActivationChar(c);
    }

    static int findCompletionPos(const AssistInterface *interface)
    {
        int pos = interface->position();
        while (isIdentifierChar(interface->textDocument()->characterAt(pos - 1)))
            pos--;
        return pos;
    }

    static Suggest::NimSuggest *nimSuggestInstance(const AssistInterface *interface)
    {
        auto filename = Utils::FilePath::fromString(interface->fileName());
        return Nim::Suggest::NimSuggestCache::instance().get(filename);
    }

    static std::shared_ptr<Suggest::NimSuggestClientRequest> sendRequest(const AssistInterface *interface,
                                                                         Suggest::NimSuggest *suggest,
                                                                         QString dirtyFile,
                                                                         int pos)
    {
        int line = 0, column = 0;
        Utils::Text::convertPosition(interface->textDocument(), pos, &line, &column);
        QTC_ASSERT(column >= 1, return nullptr);
        auto filename = Utils::FilePath::fromString(interface->fileName());
        return suggest->sug(filename.toString(), line, column - 1, dirtyFile);
    }

    static std::unique_ptr<QTemporaryFile> writeDirtyFile(const TextEditor::AssistInterface *interface)
    {
        auto result = std::make_unique<QTemporaryFile>("qtcnim.XXXXXX.nim");
        QTC_ASSERT(result->open(), return nullptr);
        QTextStream stream(result.get());
        stream << interface->textDocument()->toPlainText();
        result->close();
        return result;
    }

    static AssistProposalItemInterface *createProposal(const Nim::Suggest::Line &line)
    {
        auto item = new AssistProposalItem();
        item->setIcon(Utils::CodeModelIcon::iconForType(symbolIcon(line.symbol_kind)));
        item->setText(line.data.back());
        item->setDetail(line.symbol_type);
        item->setOrder(symbolOrder(line.symbol_kind));
        return item;
    }

    static int symbolOrder(Suggest::Line::SymbolKind kind)
    {
        switch (kind) {
        case Suggest::Line::SymbolKind::skField:
            return 2;
        case Suggest::Line::SymbolKind::skVar:
        case Suggest::Line::SymbolKind::skLet:
        case Suggest::Line::SymbolKind::skEnumField:
        case Suggest::Line::SymbolKind::skResult:
        case Suggest::Line::SymbolKind::skForVar:
        case Suggest::Line::SymbolKind::skParam:
        case Suggest::Line::SymbolKind::skLabel:
        case Suggest::Line::SymbolKind::skGenericParam:
            return 1;
        default:
            return 0;
        }
    }

    static Utils::CodeModelIcon::Type symbolIcon(Suggest::Line::SymbolKind kind)
    {
        switch (kind) {
        case Suggest::Line::SymbolKind::skField:
            return Utils::CodeModelIcon::Property;
        case Suggest::Line::SymbolKind::skVar:
        case Suggest::Line::SymbolKind::skLet:
        case Suggest::Line::SymbolKind::skEnumField:
        case Suggest::Line::SymbolKind::skResult:
        case Suggest::Line::SymbolKind::skForVar:
        case Suggest::Line::SymbolKind::skParam:
        case Suggest::Line::SymbolKind::skLabel:
        case Suggest::Line::SymbolKind::skGenericParam:
            return Utils::CodeModelIcon::VarPublic;
        case Suggest::Line::SymbolKind::skProc:
        case Suggest::Line::SymbolKind::skFunc:
        case Suggest::Line::SymbolKind::skMethod:
        case Suggest::Line::SymbolKind::skIterator:
        case Suggest::Line::SymbolKind::skConverter:
            return Utils::CodeModelIcon::FuncPublic;
        case Suggest::Line::SymbolKind::skType:
            return Utils::CodeModelIcon::Struct;
        case Suggest::Line::SymbolKind::skMacro:
        case Suggest::Line::SymbolKind::skTemplate:
            return Utils::CodeModelIcon::Macro;
        case Suggest::Line::SymbolKind::skModule:
        case Suggest::Line::SymbolKind::skPackage:
            return Utils::CodeModelIcon::Namespace;
        case Suggest::Line::SymbolKind::skUnknown:
        case Suggest::Line::SymbolKind::skConditional:
        case Suggest::Line::SymbolKind::skDynLib:
        case Suggest::Line::SymbolKind::skTemp:
        case Suggest::Line::SymbolKind::skConst:
        case Suggest::Line::SymbolKind::skStub:
        case Suggest::Line::SymbolKind::skAlias:
        default:
            return Utils::CodeModelIcon::Type::Unknown;
        }
    }

    bool m_running = false;
    int m_pos = -1;
    std::weak_ptr<Suggest::NimSuggest> m_suggest;
    std::shared_ptr<Suggest::NimSuggestClientRequest> m_request;
    std::unique_ptr<QTemporaryFile> m_dirtyFile;
    const TextEditor::AssistInterface *m_interface = nullptr;
};


TextEditor::IAssistProcessor *NimCompletionAssistProvider::createProcessor() const
{
    return new NimCompletionAssistProcessor();
}

int NimCompletionAssistProvider::activationCharSequenceLength() const
{
    return 1;
}

bool NimCompletionAssistProvider::isActivationCharSequence(const QString &sequence) const
{
    return !sequence.isEmpty() && isActivationChar(sequence.at(0));
}

IAssistProvider::RunType NimCompletionAssistProvider::runType() const
{
    return RunType::Asynchronous;
}

}

#include "nimcompletionassistprovider.moc"
