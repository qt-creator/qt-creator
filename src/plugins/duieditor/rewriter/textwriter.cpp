#include "textwriter.h"

using namespace DuiEditor::Internal;

TextWriter::TextWriter()
        :string(0), cursor(0)
{
}

static bool overlaps(int posA, int lengthA, int posB, int lengthB) {
    return (posA < posB + lengthB && posA + lengthA > posB + lengthB)
            || (posA < posB && posA + lengthA > posB);
}

bool TextWriter::hasOverlap(int pos, int length)
{
    {
        QListIterator<Replace> i(replaceList);
        while (i.hasNext()) {
            const Replace &cmd = i.next();
            if (overlaps(pos, length, cmd.pos, cmd.length))
                return true;
        }
    }
    {
        QListIterator<Move> i(moveList);
        while (i.hasNext()) {
            const Move &cmd = i.next();
            if (overlaps(pos, length, cmd.pos, cmd.length))
                return true;
        }
        return false;
    }
}

bool TextWriter::hasMoveInto(int pos, int length)
{
    QListIterator<Move> i(moveList);
    while (i.hasNext()) {
        const Move &cmd = i.next();
        if (cmd.to >= pos && cmd.to < pos + length)
            return true;
    }
    return false;
}

void TextWriter::replace(int pos, int length, const QString &replacement)
{
    Q_ASSERT(!hasOverlap(pos, length));
    Q_ASSERT(!hasMoveInto(pos, length));

    Replace cmd;
    cmd.pos = pos;
    cmd.length = length;
    cmd.replacement = replacement;
    replaceList += cmd;
}

void TextWriter::move(int pos, int length, int to)
{
    Q_ASSERT(!hasOverlap(pos, length));

    Move cmd;
    cmd.pos = pos;
    cmd.length = length;
    cmd.to = to;
    moveList += cmd;
}

void TextWriter::doReplace(const Replace &replace)
{
    int diff = replace.replacement.size() - replace.length;
    {
        QMutableListIterator<Replace> i(replaceList);
        while (i.hasNext()) {
            Replace &c = i.next();
            if (replace.pos < c.pos)
                c.pos += diff;
            else if (replace.pos + replace.length < c.pos + c.length)
                c.length += diff;
        }
    }
    {
        QMutableListIterator<Move> i(moveList);
        while (i.hasNext()) {
            Move &c = i.next();
            if (replace.pos < c.pos)
                c.pos += diff;
            else if (replace.pos + replace.length < c.pos + c.length)
                c.length += diff;

            if (replace.pos < c.to)
                c.to += diff;
        }
    }

    if (string) {
        string->replace(replace.pos, replace.length, replace.replacement);
    } else if (cursor) {
        cursor->setPosition(replace.pos);
        cursor->setPosition(replace.pos + replace.length, QTextCursor::KeepAnchor);
        cursor->insertText(replace.replacement);
    }
}

void TextWriter::doMove(const Move &move)
{
    QString text;
    if (string) {
        text = string->mid(move.pos, move.length);
    } else if (cursor) {
        cursor->setPosition(move.pos);
        cursor->setPosition(move.pos + move.length, QTextCursor::KeepAnchor);
        text = cursor->selectedText();
    }

    Replace cut;
    cut.pos = move.pos;
    cut.length = move.length;
    Replace paste;
    paste.pos = move.to;
    paste.length = 0;
    paste.replacement = text;

    replaceList.append(cut);
    replaceList.append(paste);

    Replace cmd;
    while (!replaceList.isEmpty()) {
        cmd = replaceList.first();
        replaceList.removeFirst();
        doReplace(cmd);
    }
}

void TextWriter::write(QString *s)
{
    string = s;
    write_helper();
    string = 0;
}

void TextWriter::write(QTextCursor *textCursor)
{
    cursor = textCursor;
    write_helper();
    cursor = 0;
}

void TextWriter::write_helper()
{
    if (cursor)
        cursor->beginEditBlock();
    {
        Replace cmd;
        while (!replaceList.isEmpty()) {
            cmd = replaceList.first();
            replaceList.removeFirst();
            doReplace(cmd);
        }
    }
    {
        Move cmd;
        while (!moveList.isEmpty()) {
            cmd = moveList.first();
            moveList.removeFirst();
            doMove(cmd);
        }
    }
    if (cursor)
        cursor->endEditBlock();
}
