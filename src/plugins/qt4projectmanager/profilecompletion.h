#ifndef PROFILECOMPLETION_H
#define PROFILECOMPLETION_H

#include <texteditor/icompletioncollector.h>

namespace Qt4ProjectManager {

namespace Internal {

class ProFileCompletion : public TextEditor::ICompletionCollector
{
    Q_OBJECT
public:
    ProFileCompletion(QObject *parent = 0);

    virtual ~ProFileCompletion();

    virtual QList<TextEditor::CompletionItem> getCompletions();
    virtual bool shouldRestartCompletion();

    virtual TextEditor::ITextEditable *editor() const;
    virtual int startPosition() const;

    virtual bool supportsEditor(TextEditor::ITextEditable *editor);
    virtual bool triggersCompletion(TextEditor::ITextEditable *editor);
    virtual int startCompletion(TextEditor::ITextEditable *editor);
    virtual void completions(QList<TextEditor::CompletionItem> *completions);
    virtual bool typedCharCompletes(const TextEditor::CompletionItem &item, QChar typedChar);
    virtual void complete(const TextEditor::CompletionItem &item, QChar typedChar);
    virtual bool partiallyComplete(const QList<TextEditor::CompletionItem> &completionItems);
    virtual void cleanup();
private:
    int findStartOfName(int pos = -1) const;
    bool isInComment() const;

    TextEditor::ITextEditable *m_editor;
    int m_startPosition;
    const QIcon m_variableIcon;
    const QIcon m_functionIcon;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // PROFILECOMPLETION_H
