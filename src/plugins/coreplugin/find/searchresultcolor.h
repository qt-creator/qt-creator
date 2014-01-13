#ifndef SEARCHRESULTCOLOR_H
#define SEARCHRESULTCOLOR_H

#include <QColor>

namespace Core {
namespace Internal {

class SearchResultColor{
public:
    QColor textBackground;
    QColor textForeground;
    QColor highlightBackground;
    QColor highlightForeground;
};

} // namespace Internal
} // namespace Core

#endif // SEARCHRESULTCOLOR_H
