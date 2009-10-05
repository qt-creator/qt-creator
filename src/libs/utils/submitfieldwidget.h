#ifndef SUBMITFIELDWIDGET_H
#define SUBMITFIELDWIDGET_H

#include "utils_global.h"

#include  <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QCompleter;
QT_END_NAMESPACE

namespace Utils {

struct SubmitFieldWidgetPrivate;

/* A widget for editing submit message fields like "reviewed-by:",
 * "signed-off-by:". It displays them in a vertical row of combo/line edit fields
 * that is modeled after the target address controls of mail clients.
 * When choosing a different field in the combo, a new row is opened if text
 * has been entered for the current field. Optionally, a "Browse..." button and
 * completer can be added. */
class QTCREATOR_UTILS_EXPORT SubmitFieldWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QStringList fields READ fields WRITE setFields DESIGNABLE true)
    Q_PROPERTY(bool hasBrowseButton READ hasBrowseButton WRITE setHasBrowseButton DESIGNABLE true)
    Q_PROPERTY(bool allowDuplicateFields READ allowDuplicateFields WRITE setAllowDuplicateFields DESIGNABLE true)

public:
    explicit SubmitFieldWidget(QWidget *parent = 0);
    virtual ~SubmitFieldWidget();

    QStringList fields() const;
    void setFields(const QStringList&);

    bool hasBrowseButton() const;
    void setHasBrowseButton(bool d);

    // Allow several entries for fields ("reviewed-by: a", "reviewed-by: b")
    bool allowDuplicateFields() const;
    void setAllowDuplicateFields(bool);

    QCompleter *completer() const;
    void setCompleter(QCompleter *c);

    QString fieldValue(int pos) const;
    void setFieldValue(int pos, const QString &value);

    QString fieldValues() const;

signals:
    void browseButtonClicked(int pos, const QString &field);

private slots:
    void slotRemove();
    void slotComboIndexChanged(int);
    void slotBrowseButtonClicked();

private:
    void removeField(int index);
    bool comboIndexChange(int fieldNumber, int index);
    void createField(const QString &f);

    SubmitFieldWidgetPrivate *m_d;
};

}

#endif // SUBMITFIELDWIDGET_H
