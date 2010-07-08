/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CUSTOMPROJECTWIZARDDIALOG_H
#define CUSTOMPROJECTWIZARDDIALOG_H

#include <QtGui/QComboBox>
#include <QtGui/QCheckBox>
#include <QtGui/QWizardPage>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QFormLayout;
class QLineEdit;
class QTextEdit;
QT_END_NAMESPACE

namespace Utils {
    class PathChooser;
}

namespace ProjectExplorer {
namespace Internal {

struct CustomWizardField;
struct CustomWizardParameters;
struct CustomWizardContext;

// A non-editable combo for text editing purposes that plays
// with QWizard::registerField (providing a settable 'text' property).
// Allows for a separation of values to be used for wizard fields replacement
// and display texts.
class TextFieldComboBox : public QComboBox {
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_OBJECT
public:
    explicit TextFieldComboBox(QWidget *parent = 0);

    QString text() const;
    void setText(const QString &s);

    void setItems(const QStringList &displayTexts, const QStringList &values);

signals:
    void text4Changed(const QString &); // Do not conflict with Qt 3 compat signal.

private slots:
    void slotCurrentIndexChanged(int);

private:
    inline QString valueAt(int) const;
};

// A Checkbox that plays with QWizard::registerField (providing a settable
// 'text' property containing predefined strings for 'true'/'false').
class TextFieldCheckBox : public QCheckBox {
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QString trueText READ trueText WRITE setTrueText)
    Q_PROPERTY(QString falseText READ falseText WRITE setFalseText)
    Q_OBJECT
public:
    explicit TextFieldCheckBox(const QString &text, QWidget *parent = 0);

    QString text() const;
    void setText(const QString &s);

    void setTrueText(const QString &t)  { m_trueText = t;    }
    QString trueText() const            { return m_trueText; }
    void setFalseText(const QString &t) { m_falseText = t; }
    QString falseText() const              { return m_falseText; }

signals:
    void textChanged(const QString &);

private slots:
    void slotStateChanged(int);

private:
    QString m_trueText;
    QString m_falseText;
};

// A simple custom wizard page presenting the fields to be used
// as page 2 of a BaseProjectWizardDialog if there are any fields.
// Uses the 'field' functionality of QWizard.
// Implements validatePage() as the field logic cannot be tied up
// with additional validation.
class CustomWizardFieldPage : public QWizardPage {
    Q_OBJECT
public:
    typedef QList<CustomWizardField> FieldList;

    explicit CustomWizardFieldPage(const QSharedPointer<CustomWizardContext> &ctx,
                                   const FieldList &f,
                                   QWidget *parent = 0);
    virtual ~CustomWizardFieldPage();

    virtual bool validatePage();
    virtual void initializePage();

protected:
    inline void addRow(const QString &name, QWidget *w);

private:
    struct LineEditData {
        explicit LineEditData(QLineEdit* le = 0, const QString &defText = QString());
        QLineEdit* lineEdit;
        QString defaultText;
    };
    struct TextEditData {
        explicit TextEditData(QTextEdit* le = 0, const QString &defText = QString());
        QTextEdit* textEdit;
        QString defaultText;
    };
    typedef QList<LineEditData> LineEditDataList;
    typedef QList<TextEditData> TextEditDataList;

    QWidget *registerLineEdit(const QString &fieldName, const CustomWizardField &field);
    QWidget *registerComboBox(const QString &fieldName, const CustomWizardField &field);
    QWidget *registerTextEdit(const QString &fieldName, const CustomWizardField &field);
    QWidget *registerPathChooser(const QString &fieldName, const CustomWizardField &field);
    QWidget *registerCheckBox(const QString &fieldName,
                              const QString &fieldDescription,
                              const CustomWizardField &field);
    void addField(const CustomWizardField &f);

    const QSharedPointer<CustomWizardContext> m_context;
    QFormLayout *m_formLayout;
    LineEditDataList m_lineEdits;
    TextEditDataList m_textEdits;
};

// A custom wizard page presenting the fields to be used and a path chooser
// at the bottom (for use by "class"/"file" wizards). Does validation on
// the Path chooser only (as the other fields can by validated by regexps).

class CustomWizardPage : public CustomWizardFieldPage {
    Q_OBJECT
public:
    explicit CustomWizardPage(const QSharedPointer<CustomWizardContext> &ctx,
                              const FieldList &f,
                              QWidget *parent = 0);

    QString path() const;
    void setPath(const QString &path);

    virtual bool isComplete() const;

private:
    Utils::PathChooser *m_pathChooser;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // CUSTOMPROJECTWIZARDDIALOG_H
