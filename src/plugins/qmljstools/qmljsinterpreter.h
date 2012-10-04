#ifndef QMLJSINTERPRETER_H
#define QMLJSINTERPRETER_H

#include <qmljs/parser/qmljslexer_p.h>
#include <qmljs/parser/qmljsengine_p.h>

#include <QVector>
#include <QString>
#include <QList>

namespace QmlJSTools {
namespace Internal {

class QmlJSInterpreter: QmlJS::Lexer
{
public:
    QmlJSInterpreter()
        : Lexer(&m_engine),
          m_stateStack(128)
    {

    }

    void clearText() { m_code.clear(); }
    void appendText(const QString &text) { m_code += text; }

    QString code() const { return m_code; }
    bool canEvaluate();

private:
    QmlJS::Engine m_engine;
    QVector<int> m_stateStack;
    QList<int> m_tokens;
    QString m_code;
};

} // namespace Internal
} // namespace QmlJSTools

#endif // QMLJSINTERPRETER_H
