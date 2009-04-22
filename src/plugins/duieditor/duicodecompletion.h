#ifndef DUICODECOMPLETION_H
#define DUICODECOMPLETION_H

#include <texteditor/icompletioncollector.h>

namespace TextEditor {
class ITextEditable;
}

namespace DuiEditor {
namespace Internal {

class DuiCodeCompletion: public TextEditor::ICompletionCollector
{
    Q_OBJECT

public:
    DuiCodeCompletion(QObject *parent = 0);
    virtual ~DuiCodeCompletion();

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
} // end of namespace DuiEditor

#endif // DUICODECOMPLETION_H
