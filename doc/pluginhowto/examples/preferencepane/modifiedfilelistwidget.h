#ifndef MODIFIEDFILELISTWIDGET_H
#define MODIFIEDFILELISTWIDGET_H

#include <QListWidget>
class ModifiedFileListWidget: public QListWidget
{
    Q_OBJECT

public:
    ModifiedFileListWidget(QWidget* parent=0);
    ~ModifiedFileListWidget();
};

#endif // MODIFIEDFILELISTWIDGET_H
