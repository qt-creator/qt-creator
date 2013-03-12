/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CUSTOMWIZARDPAGE_H
#define CUSTOMWIZARDPAGE_H

#include <QComboBox>
#include <QCheckBox>
#include <QWizardPage>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QFormLayout;
class QLineEdit;
class QTextEdit;
class QLabel;
QT_END_NAMESPACE

namespace Utils {
    class PathChooser;
}

namespace ProjectExplorer {
namespace Internal {

struct CustomWizardField;
struct CustomWizardParameters;
struct CustomWizardContext;

// Documentation inside.
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

// Documentation inside.
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

// Documentation inside.
class CustomWizardFieldPage : public QWizardPage {
    Q_OBJECT
public:
    typedef QList<CustomWizardField> FieldList;

    explicit CustomWizardFieldPage(const QSharedPointer<CustomWizardContext> &ctx,
                                   const QSharedPointer<CustomWizardParameters> &parameters,
                                   QWidget *parent = 0);
    virtual ~CustomWizardFieldPage();

    virtual bool validatePage();
    virtual void initializePage();
    virtual void cleanupPage();

    static QMap<QString, QString> replacementMap(const QWizard *w,
                                                 const QSharedPointer<CustomWizardContext> &ctx,
                                                 const FieldList &f);

protected:
    inline void addRow(const QString &name, QWidget *w);
    void showError(const QString &);
    void clearError();
private:
    struct LineEditData {
        explicit LineEditData(QLineEdit* le = 0, const QString &defText = QString());
        QLineEdit* lineEdit;
        QString defaultText;
        QString userChange;
    };
    struct TextEditData {
        explicit TextEditData(QTextEdit* le = 0, const QString &defText = QString());
        QTextEdit* textEdit;
        QString defaultText;
        QString userChange;
    };
    struct PathChooserData {
        explicit PathChooserData(Utils::PathChooser* pe = 0, const QString &defText = QString());
        Utils::PathChooser* pathChooser;
        QString defaultText;
        QString userChange;
    };

    typedef QList<LineEditData> LineEditDataList;
    typedef QList<TextEditData> TextEditDataList;
    typedef QList<PathChooserData> PathChooserDataList;

    QWidget *registerLineEdit(const QString &fieldName, const CustomWizardField &field);
    QWidget *registerComboBox(const QString &fieldName, const CustomWizardField &field);
    QWidget *registerTextEdit(const QString &fieldName, const CustomWizardField &field);
    QWidget *registerPathChooser(const QString &fieldName, const CustomWizardField &field);
    QWidget *registerCheckBox(const QString &fieldName,
                              const QString &fieldDescription,
                              const CustomWizardField &field);
    void addField(const CustomWizardField &f);

    const QSharedPointer<CustomWizardParameters> m_parameters;
    const QSharedPointer<CustomWizardContext> m_context;
    QFormLayout *m_formLayout;
    LineEditDataList m_lineEdits;
    TextEditDataList m_textEdits;
    PathChooserDataList m_pathChoosers;
    QLabel *m_errorLabel;
};

// Documentation inside.
class CustomWizardPage : public CustomWizardFieldPage {
    Q_OBJECT
public:
    explicit CustomWizardPage(const QSharedPointer<CustomWizardContext> &ctx,
                              const QSharedPointer<CustomWizardParameters> &parameters,
                              QWidget *parent = 0);

    QString path() const;
    void setPath(const QString &path);

    virtual bool isComplete() const;

private:
    Utils::PathChooser *m_pathChooser;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // CUSTOMWIZARDPAGE_H
