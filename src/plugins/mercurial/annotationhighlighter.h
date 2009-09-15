#ifndef ANNOTATIONHIGHLIGHTER_H
#define ANNOTATIONHIGHLIGHTER_H

#include <vcsbase/baseannotationhighlighter.h>

namespace Mercurial {
namespace Internal {

class MercurialAnnotationHighlighter : public VCSBase::BaseAnnotationHighlighter
{
public:
    explicit MercurialAnnotationHighlighter(const ChangeNumbers &changeNumbers,
                                            QTextDocument *document = 0);

private:
    virtual QString changeNumber(const QString &block) const;
    QRegExp changeset;
};

} //namespace Internal
}// namespace Mercurial
#endif // ANNOTATIONHIGHLIGHTER_H
