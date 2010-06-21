#ifndef MODELNAMEPAGE_H
#define MODELNAMEPAGE_H

#include <QWizardPage>
#include "ui_ModelNamePage.h"
struct ModelClassParameters
 {
     QString className;
     QString headerFile;
     QString sourceFile;
     QString baseClass;
     QString path;
 };

class ModelNamePage :  public QWizardPage
{
    Q_OBJECT

public:
    ModelNamePage(QWidget *parent = 0);
    ~ModelNamePage();
    void setPath(const QString& path);
    ModelClassParameters parameters() const;
    
private slots:
    void on_txtModelClass_textEdited(const QString& txt);


private:
    Ui::ModelNamePage ui;
    QString path;
};

#endif // MODELNAMEPAGE_H
