#ifndef QTSCRIPTCODECOMPLETION_H
#define QTSCRIPTCODECOMPLETION_H

#include <texteditor/icompletioncollector.h>

namespace TextEditor {
class ITextEditable;
}

namespace QtScriptEditor {
namespace Internal {

class QtScriptCodeCompletion: public TextEditor::ICompletionCollector
{
    Q_OBJECT

public:
    QtScriptCodeCompletion(QObject *parent = 0);
    virtual ~QtScriptCodeCompletion();

    Qt::CaseSensitivity caseSensitivity() const;
    void setCaseSensitivity(Qt::CaseSensitivity caseSensitivity);

    virtual bool supportsEditor(TextEditor::ITextEditable *editor);
    virtual bool triggersCompletion(TextEditor::ITextEditable *editor);
    virtual int startCompletion(TextEditor::ITextEditable *editor);
    virtual void completions(QList<TextEditor::CompletionItem> *completions);
    virtual void complete(const TextEditor::CompletionItem &item);
    virtual bool partiallyComplete(const QList<TextEditor::CompletionItem> &completionItems);
    virtual void cleanup();

private:
    TextEditor::ITextEditable *m_editor;
    int m_startPosition;
    QList<TextEditor::CompletionItem> m_completions;
    Qt::CaseSensitivity m_caseSensitivity;
};


} // end of namespace Internal
} // end of namespace QtScriptEditor

#endif // QTSCRIPTCODECOMPLETION_H
