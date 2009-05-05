#ifndef TEXTWRITER_H
#define TEXTWRITER_H

#include <QString>
#include <QList>
#include <QTextCursor>

namespace DuiEditor {
namespace Internal {

class TextWriter
{
    QString *string;
    QTextCursor *cursor;

    struct Replace {
        int pos;
        int length;
        QString replacement;
    };

    QList<Replace> replaceList;

    struct Move {
        int pos;
        int length;
        int to;
    };

    QList<Move> moveList;

    bool hasOverlap(int pos, int length);
    bool hasMoveInto(int pos, int length);

    void doReplace(const Replace &replace);
    void doMove(const Move &move);

    void write_helper();

public:
    TextWriter();

    void replace(int pos, int length, const QString &replacement);
    void move(int pos, int length, int to);

    void write(QString *s);
    void write(QTextCursor *textCursor);

};

} // end of namespace Internal
} // end of namespace DuiEditor

#endif // TEXTWRITER_H
