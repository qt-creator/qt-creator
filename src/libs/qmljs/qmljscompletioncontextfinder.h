#ifndef QMLJSCOMPLETIONCONTEXTFINDER_H
#define QMLJSCOMPLETIONCONTEXTFINDER_H

#include "qmljs_global.h"
#include <qmljs/qmljslineinfo.h>

#include <QtCore/QStringList>
#include <QtGui/QTextCursor>

namespace QmlJS {

class QMLJS_EXPORT CompletionContextFinder : public LineInfo
{
public:
    CompletionContextFinder(const QTextCursor &cursor);

    //bool inQmlObjectDefinition();
    bool inQmlBindingRhs();

    QStringList qmlObjectTypeName() const;
    bool isInQmlContext() const;

    bool isInLhsOfBinding() const;
    bool isInRhsOfBinding() const;

private:
    int findOpeningBrace(int startTokenIndex);
    void getQmlObjectTypeName(int startTokenIndex);
    void checkBinding();

    QTextCursor m_cursor;
    QStringList m_qmlObjectTypeName;

    int m_startTokenIndex;
    int m_colonCount;
};

} // namespace QmlJS

#endif // QMLJSCOMPLETIONCONTEXTFINDER_H
