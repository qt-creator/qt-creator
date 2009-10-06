#ifndef CHECKABLEMESSAGEBOX_H
#define CHECKABLEMESSAGEBOX_H

#include "utils_global.h"

#include <QtGui/QDialogButtonBox>
#include <QtGui/QMessageBox>
#include <QtGui/QDialog>

namespace Utils {

struct CheckableMessageBoxPrivate;

/* A messagebox suitable for questions with a
 * "Do not ask me again" checkbox. Emulates the QMessageBox API with
 * static conveniences. */

class QTCREATOR_UTILS_EXPORT CheckableMessageBox : public QDialog
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QPixmap iconPixmap READ iconPixmap WRITE setIconPixmap)
    Q_PROPERTY(bool isChecked READ isChecked WRITE setChecked)
    Q_PROPERTY(QString checkBoxText READ checkBoxText WRITE setCheckBoxText)
    Q_PROPERTY(QDialogButtonBox::StandardButtons buttons READ standardButtons WRITE setStandardButtons)
    Q_PROPERTY(QDialogButtonBox::StandardButton defaultButton READ defaultButton WRITE setDefaultButton)
public:
    explicit CheckableMessageBox(QWidget *parent);
    virtual ~CheckableMessageBox();

    static QDialogButtonBox::StandardButton
        question(QWidget *parent,
                 const QString &title,
                 const QString &question,
                 const QString &checkBoxText,
                 bool *checkBoxSetting,
                 QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Yes|QDialogButtonBox::No,
                 QDialogButtonBox::StandardButton defaultButton = QDialogButtonBox::No);

    QString text() const;
    void setText(const QString &);

    bool isChecked() const;
    void setChecked(bool s);

    QString checkBoxText() const;
    void setCheckBoxText(const QString &);

   QDialogButtonBox::StandardButtons standardButtons() const;
   void setStandardButtons(QDialogButtonBox::StandardButtons s);

   QDialogButtonBox::StandardButton defaultButton() const;
   void setDefaultButton(QDialogButtonBox::StandardButton s);

    // see static QMessageBox::standardPixmap()
    QPixmap iconPixmap() const;
    void setIconPixmap (const QPixmap &p);

   // Query the result
   QAbstractButton *clickedButton() const;
   QDialogButtonBox::StandardButton clickedStandardButton() const;

   // Conversion convenience
   static QMessageBox::StandardButton dialogButtonBoxToMessageBoxButton(QDialogButtonBox::StandardButton);

private slots:
   void slotClicked(QAbstractButton *b);

private:
   CheckableMessageBoxPrivate *m_d;
};

} // namespace Utils

#endif // CHECKABLEMESSAGEBOX_H
