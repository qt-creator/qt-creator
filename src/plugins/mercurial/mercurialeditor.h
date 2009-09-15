#ifndef MERCURIALEDITOR_H
#define MERCURIALEDITOR_H

#include <vcsbase/vcsbaseeditor.h>

#include <QtCore/QRegExp>

namespace Mercurial {
namespace Internal {

class MercurialEditor : public VCSBase::VCSBaseEditor
{
public:
    explicit MercurialEditor(const VCSBase::VCSBaseEditorParameters *type, QWidget *parent);

private:
    virtual QSet<QString> annotationChanges() const;
    virtual QString changeUnderCursor(const QTextCursor &cursor) const;
    virtual VCSBase::DiffHighlighter *createDiffHighlighter() const;
    virtual VCSBase::BaseAnnotationHighlighter *createAnnotationHighlighter(const QSet<QString> &changes) const;
    virtual QString fileNameFromDiffSpecification(const QTextBlock &diffFileSpec) const;

    const QRegExp exactIdentifier12;
    const QRegExp exactIdentifier40;
    const QRegExp changesetIdentifier12;
    const QRegExp changesetIdentifier40;
    const QRegExp diffIdentifier;
};

} // namespace Internal
} // namespace Mercurial
#endif // MERCURIALEDITOR_H
