#ifndef HEADERFILTERPROGRESS_H
#define HEADERFILTERPROGRESS_H


#include <find/ifindfilter.h>
#include <utils/filesearch.h>
#include <QtGui/QComboBox>

namespace Find {
class SearchResultWindow;
struct SearchResultItem;
}

class QKeySequence;
class QWidget;



struct HeaderFilterProgressData;
class HeaderFilterProgress : public Find::IFindFilter
{
    Q_OBJECT

public:
    HeaderFilterProgress();
    ~HeaderFilterProgress();
    QString id() const;
    QString name() const;
    bool isEnabled() const;
    QKeySequence defaultShortcut() const;
    void findAll(const QString &txt,QTextDocument::FindFlags findFlags);
    QWidget* createConfigWidget();
    QWidget* createProgressWidget();

protected slots:
    void displayResult(int index);
    void openEditor(const Find::SearchResultItem &item);

private:
    HeaderFilterProgressData* d;

};


#endif // HEADERFILTERPROGRESS_H
