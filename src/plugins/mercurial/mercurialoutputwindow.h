#ifndef MERCURIALOUTPUTWINDOW_H
#define MERCURIALOUTPUTWINDOW_H

#include <coreplugin/ioutputpane.h>

QT_BEGIN_NAMESPACE
class QListWidget;
QT_END_NAMESPACE

#include <QtCore/QByteArray>

namespace Mercurial {
namespace Internal {

class MercurialOutputWindow: public Core::IOutputPane
{
    Q_OBJECT

public:
    MercurialOutputWindow();
    ~MercurialOutputWindow();

    QWidget *outputWidget(QWidget *parent);
    QList<QWidget*> toolBarWidgets() const;
    QString name() const;
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool visible);
    void setFocus();
    bool hasFocus();
    bool canFocus();
    bool canNavigate();
    bool canNext();
    bool canPrevious();
    void goToNext();
    void goToPrev();

public slots:
    void append(const QString &text);
    void append(const QByteArray &array);

private:
    QListWidget *outputListWidgets;
};

} //namespace Internal
} //namespace Mercurial

#endif // MERCURIALOUTPUTWINDOW_H
