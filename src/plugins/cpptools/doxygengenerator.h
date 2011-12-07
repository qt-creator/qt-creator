#ifndef DOXYGEGENERATOR_H
#define DOXYGEGENERATOR_H

#include "cpptools_global.h"

#include <cplusplus/Overview.h>

#include <QtCore/QLatin1String>
#include <QtGui/QTextCursor>

namespace CPlusPlus {
class DeclarationAST;
}

namespace CppTools {

class CPPTOOLS_EXPORT DoxygenGenerator
{
public:
    DoxygenGenerator();

    enum DocumentationStyle {
        JavaStyle,
        QtStyle
    };

    void setStyle(DocumentationStyle style);
    void setStartComment(bool start);
    void setGenerateBrief(bool gen);
    void setAddLeadingAsterisks(bool add);

    QString generate(QTextCursor cursor);
    QString generate(QTextCursor cursor, CPlusPlus::DeclarationAST *decl);

private:
    QChar startMark() const;
    QChar styleMark() const;

    enum Command {
        BriefCommand,
        ParamCommand,
        ReturnCommand
    };
    static QString commandSpelling(Command command);

    void writeStart(QString *comment) const;
    void writeEnd(QString *comment) const;
    void writeContinuation(QString *comment) const;
    void writeNewLine(QString *comment) const;
    void writeCommand(QString *comment,
                      Command command,
                      const QString &commandContent = QString()) const;
    void writeBrief(QString *comment,
                    const QString &brief,
                    const QString &prefix = QString(),
                    const QString &suffix = QString());

    void assignCommentOffset(QTextCursor cursor);
    QString offsetString() const;

    bool m_addLeadingAsterisks;
    bool m_generateBrief;
    bool m_startComment;
    DocumentationStyle m_style;
    CPlusPlus::Overview m_printer;
    int m_commentOffset;
};

} // CppTools

#endif // DOXYGEGENERATOR_H
