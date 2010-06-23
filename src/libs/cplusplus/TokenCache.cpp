#include "SimpleLexer.h"
#include "TokenCache.h"

using namespace CPlusPlus;

TokenCache::TokenCache(QTextDocument *doc)
    : m_doc(doc)
    , m_revision(-1)
{}

void TokenCache::setDocument(QTextDocument *doc)
{
    m_doc = doc;
    m_revision = -1;
}

QList<SimpleToken> TokenCache::tokensForBlock(const QTextBlock &block) const
{
    Q_ASSERT(m_doc);
    Q_ASSERT(m_doc == block.document());

    const int documentRevision = m_doc->revision();

    if (documentRevision != m_revision) {
        m_tokensByBlock.clear();
        m_revision = documentRevision;
    }

    const int blockNr = block.blockNumber();

    if (m_tokensByBlock.contains(blockNr)) {
        return m_tokensByBlock.value(blockNr);
    } else {

        SimpleLexer tokenize;
        tokenize.setObjCEnabled(true);
        tokenize.setQtMocRunEnabled(true);
        tokenize.setSkipComments(false);

        const int prevState = previousBlockState(block);
        QList<SimpleToken> tokens = tokenize(block.text(), prevState);
        m_tokensByBlock.insert(blockNr, tokens);

        return tokens;
    }
}

SimpleToken TokenCache::tokenUnderCursor(const QTextCursor &cursor) const
{
    const QTextBlock block = cursor.block();
    const QList<SimpleToken> tokens = tokensForBlock(block);
    const int column = cursor.position() - block.position();

    for (int index = tokens.size() - 1; index >= 0; --index) {
        const SimpleToken &tk = tokens.at(index);
        if (tk.position() < column)
            return tk;
    }

    return SimpleToken();
}

QString TokenCache::text(const QTextBlock &block, int tokenIndex) const
{
    SimpleToken tk = tokensForBlock(block).at(tokenIndex);
    return block.text().mid(tk.position(), tk.length());
}

int TokenCache::previousBlockState(const QTextBlock &block)
{
    const QTextBlock prevBlock = block.previous();

    if (prevBlock.isValid()) {
        int state = prevBlock.userState();

        if (state != -1)
            return state;
    }

    return 0;
}
