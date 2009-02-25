/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef FORMWIZARDPAGE_H
#define FORMWIZARDPAGE_H

#include <QtGui/QWizardPage>

QT_BEGIN_NAMESPACE
class QDesignerNewFormWidgetInterface;
QT_END_NAMESPACE

namespace Designer {
namespace Internal {

// A wizard page embedding Qt Designer's QDesignerNewFormWidgetInterface
// widget.

class FormTemplateWizardPage : public QWizardPage
{
    Q_DISABLE_COPY(FormTemplateWizardPage)
    Q_OBJECT
public:
    explicit FormTemplateWizardPage(QWidget * parent = 0);

    virtual bool isComplete () const;
    virtual bool validatePage();

    QString templateContents() const { return  m_templateContents; }

    static bool getUIXmlData(const QString &uiXml, QString *formBaseClass, QString *uiClassName);
    static QString changeUiClassName(const QString &uiXml, const QString &newUiClassName);
    static QString stripNamespaces(const QString &className);

signals:
    void templateActivated();

private slots:
    void slotCurrentTemplateChanged(bool);

private:
    QString m_templateContents;
    QDesignerNewFormWidgetInterface *m_newFormWidget;
    bool m_templateSelected;
};

} // namespace Internal
} // namespace Designer

#endif // FORMTEMPLATEWIZARDPAGE_H
