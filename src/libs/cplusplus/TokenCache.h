#ifndef TOKENCACHE_H
#define TOKENCACHE_H

#include <CPlusPlusForwardDeclarations.h>
#include <SimpleLexer.h>

#include <QtCore/QHash>
#include <QtCore/QList>

#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT TokenCache
{
public:
    TokenCache();

    void setDocument(QTextDocument *doc);

    QList<CPlusPlus::SimpleToken> tokensForBlock(const QTextBlock &block) const;
    CPlusPlus::SimpleToken tokenUnderCursor(const QTextCursor &cursor) const;

    QString text(const QTextBlock &block, int tokenIndex) const;

    static int previousBlockState(const QTextBlock &block);

private:
    QTextDocument *m_doc;

    mutable int m_revision;
    mutable QHash<int, QList<CPlusPlus::SimpleToken> > m_tokensByBlock;
};

} // namespace CPlusPlus

#endif // TOKENCACHE_H
