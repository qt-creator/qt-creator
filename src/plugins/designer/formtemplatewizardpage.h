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

#ifndef FORMWIZARDPAGE_H
#define FORMWIZARDPAGE_H

#include <QWizardPage>

QT_BEGIN_NAMESPACE
class QDesignerNewFormWidgetInterface;
QT_END_NAMESPACE

namespace Designer {
namespace Internal {

// A wizard page embedding Qt Designer's QDesignerNewFormWidgetInterface
// widget.

class FormTemplateWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit FormTemplateWizardPage(QWidget * parent = 0);

    virtual bool isComplete () const;
    virtual bool validatePage();

    QString templateContents() const { return  m_templateContents; }

    // Parse UI XML forms to determine:
    // 1) The ui class name from "<class>Designer::Internal::FormClassWizardPage</class>"
    // 2) the base class from: widget class="QWizardPage"...
    static bool getUIXmlData(const QString &uiXml, QString *formBaseClass, QString *uiClassName);
    // Change the class name in a UI XML form
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
