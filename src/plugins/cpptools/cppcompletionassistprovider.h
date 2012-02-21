#ifndef CPPTOOLS_CPPCOMPLETIONASSISTPROVIDER_H
#define CPPTOOLS_CPPCOMPLETIONASSISTPROVIDER_H

#include "cppcompletionsupport.h"
#include "cpptools_global.h"

#include <texteditor/codeassist/completionassistprovider.h>

namespace CppTools {

class CPPTOOLS_EXPORT CppCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
    Q_OBJECT

public:
    virtual bool supportsEditor(const Core::Id &editorId) const;
    virtual int activationCharSequenceLength() const;
    virtual bool isActivationCharSequence(const QString &sequence) const;

    virtual CppCompletionSupport *completionSupport(TextEditor::ITextEditor *editor) = 0;

    static int activationSequenceChar(const QChar &ch, const QChar &ch2,
                                      const QChar &ch3, unsigned *kind,
                                      bool wantFunctionCall);
};

} // namespace CppTools

#endif // CPPTOOLS_CPPCOMPLETIONASSISTPROVIDER_H
