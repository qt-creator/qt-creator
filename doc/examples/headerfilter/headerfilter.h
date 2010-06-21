#ifndef HEADERFILTER_H
#define HEADERFILTER_H


#include <find/ifindfilter.h>
#include <utils/filesearch.h>

namespace Find {
class SearchResultWindow;
struct SearchResultItem;
}

class QKeySequence;
class QWidget;



struct HeaderFilterData;
class HeaderFilter : public Find::IFindFilter
{
    Q_OBJECT

public:
    HeaderFilter();
    ~HeaderFilter();
    QString id() const;
    QString name() const;
    bool isEnabled() const;
    QKeySequence defaultShortcut() const;
    void findAll(const QString &txt,QTextDocument::FindFlags findFlags);
    QWidget *createConfigWidget();

protected slots:
    void displayResult(int index);
    void openEditor(const Find::SearchResultItem &item);

private:
    HeaderFilterData *d;
};


#endif // HEADERFILTER_H
