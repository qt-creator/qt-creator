/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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

class FormTemplateWizardPagePage : public QWizardPage
{
    Q_DISABLE_COPY(FormTemplateWizardPagePage)
    Q_OBJECT
public:
    explicit FormTemplateWizardPagePage(QWidget * parent = 0);

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
