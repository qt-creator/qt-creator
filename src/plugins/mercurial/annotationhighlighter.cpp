#include "annotationhighlighter.h"
#include "constants.h"

using namespace Mercurial::Internal;
using namespace Mercurial;

MercurialAnnotationHighlighter::MercurialAnnotationHighlighter(const ChangeNumbers &changeNumbers,
                                                               QTextDocument *document)
    : VCSBase::BaseAnnotationHighlighter(changeNumbers, document),
    changeset(Constants::CHANGESETID12)
{
}

QString MercurialAnnotationHighlighter::changeNumber(const QString &block) const
{
    if (changeset.indexIn(block) != -1)
        return changeset.cap(1);
    return QString();
}
