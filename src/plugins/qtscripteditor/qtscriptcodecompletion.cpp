
#include "qtscriptcodecompletion.h"
#include "qtscripteditor.h"
#include <texteditor/basetexteditor.h>
#include <QtDebug>

using namespace QtScriptEditor::Internal;

QtScriptCodeCompletion::QtScriptCodeCompletion(QObject *parent)
    : TextEditor::ICompletionCollector(parent),
      m_editor(0),
      m_startPosition(0),
      m_caseSensitivity(Qt::CaseSensitive)
{ }

QtScriptCodeCompletion::~QtScriptCodeCompletion()
{ }

Qt::CaseSensitivity QtScriptCodeCompletion::caseSensitivity() const
{ return m_caseSensitivity; }

void QtScriptCodeCompletion::setCaseSensitivity(Qt::CaseSensitivity caseSensitivity)
{ m_caseSensitivity = caseSensitivity; }

bool QtScriptCodeCompletion::supportsEditor(TextEditor::ITextEditable *editor)
{
#if 0 // ### disable JS Completion. It's pretty buggy and it can crash Creator.
    if (qobject_cast<ScriptEditor *>(editor->widget()))
        return true;
#else
    Q_UNUSED(editor)
#endif

    return false;
}

bool QtScriptCodeCompletion::triggersCompletion(TextEditor::ITextEditable *)
{ return false; }

int QtScriptCodeCompletion::startCompletion(TextEditor::ITextEditable *editor)
{
    m_editor = editor;

    ScriptEditor *edit = qobject_cast<ScriptEditor *>(m_editor->widget());
    if (! edit)
        return -1;

    int pos = editor->position();

    while (editor->characterAt(pos - 1).isLetterOrNumber() || editor->characterAt(pos - 1) == QLatin1Char('_'))
        --pos;

    m_startPosition = pos;
    m_completions.clear();

    foreach (const QString &word, edit->words()) {
        TextEditor::CompletionItem item(this);
        item.text = word;
        m_completions.append(item);
    }

    return pos;
}

void QtScriptCodeCompletion::completions(QList<TextEditor::CompletionItem> *completions)
{
    // ### FIXME: this code needs to be generalized.

    const int length = m_editor->position() - m_startPosition;

    if (length == 0)
        *completions = m_completions;
    else if (length > 0) {
        const QString key = m_editor->textAt(m_startPosition, length);

        /*
         * This code builds a regular expression in order to more intelligently match
         * camel-case style. This means upper-case characters will be rewritten as follows:
         *
         *   A => [a-z0-9_]*A (for any but the first capital letter)
         *
         * Meaning it allows any sequence of lower-case characters to preceed an
         * upper-case character. So for example gAC matches getActionController.
         */
        QString keyRegExp;
        keyRegExp += QLatin1Char('^');
        bool first = true;
        foreach (const QChar &c, key) {
            if (c.isUpper() && !first) {
                keyRegExp += QLatin1String("[a-z0-9_]*");
                keyRegExp += c;
            } else if (m_caseSensitivity == Qt::CaseInsensitive && c.isLower()) {
                keyRegExp += QLatin1Char('[');
                keyRegExp += c;
                keyRegExp += c.toUpper();
                keyRegExp += QLatin1Char(']');
            } else {
                keyRegExp += QRegExp::escape(c);
            }
            first = false;
        }
        const QRegExp regExp(keyRegExp, Qt::CaseSensitive);

        foreach (TextEditor::CompletionItem item, m_completions) {
            if (regExp.indexIn(item.text) == 0) {
                item.relevance = (key.length() > 0 &&
                                    item.text.startsWith(key, Qt::CaseInsensitive)) ? 1 : 0;
                (*completions) << item;
            }
        }
    }
}

void QtScriptCodeCompletion::complete(const TextEditor::CompletionItem &item)
{
    const QString toInsert = item.text;
    const int length = m_editor->position() - m_startPosition;
    m_editor->setCurPos(m_startPosition);
    m_editor->replace(length, toInsert);
}

bool QtScriptCodeCompletion::partiallyComplete(const QList<TextEditor::CompletionItem> &completionItems)
{
    if (completionItems.count() == 1) {
        complete(completionItems.first());
        return true;
    } else {
        // Compute common prefix
        QString firstKey = completionItems.first().text;
        QString lastKey = completionItems.last().text;
        const int length = qMin(firstKey.length(), lastKey.length());
        firstKey.truncate(length);
        lastKey.truncate(length);

        while (firstKey != lastKey) {
            firstKey.chop(1);
            lastKey.chop(1);
        }

        int typedLength = m_editor->position() - m_startPosition;
        if (!firstKey.isEmpty() && firstKey.length() > typedLength) {
            m_editor->setCurPos(m_startPosition);
            m_editor->replace(typedLength, firstKey);
        }
    }
    return false;
}

void QtScriptCodeCompletion::cleanup()
{
    m_editor = 0;
    m_startPosition = 0;
    m_completions.clear();
}

