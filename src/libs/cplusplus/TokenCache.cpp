#include "SimpleLexer.h"
#include "TokenCache.h"

#include <QtCore/QDebug>

using namespace CPlusPlus;

TokenCache::TokenCache()
    : m_doc(0)
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

    const int documentRevision = m_doc->revision();

    if (documentRevision != m_revision) {
        m_tokensByBlock.clear();
        m_revision = documentRevision;
//        qDebug() << "** revision changed to" << documentRevision;
    }

    const int blockNr = block.blockNumber();

    if (m_tokensByBlock.contains(blockNr)) {
//        qDebug()<<"Cache hit on line" << line;
        return m_tokensByBlock.value(blockNr);
    } else {
//        qDebug()<<"Cache miss on line" << line;

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
