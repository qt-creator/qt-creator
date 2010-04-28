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

private:
    int findOpeningBrace(int startTokenIndex);
    void getQmlObjectTypeName(int startTokenIndex);

    QTextCursor m_cursor;
    QStringList m_qmlObjectTypeName;
};

} // namespace QmlJS

#endif // QMLJSCOMPLETIONCONTEXTFINDER_H
