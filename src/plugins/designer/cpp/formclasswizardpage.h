/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#ifndef FORMCLASSWIZARDPAGE_H
#define FORMCLASSWIZARDPAGE_H

#include <QtGui/QWizardPage>

namespace Designer {
namespace Internal {

namespace Ui {
    class FormClassWizardPage;
}

struct FormClassWizardParameters;

class FormClassWizardPage : public QWizardPage
{
    Q_DISABLE_COPY(FormClassWizardPage)
    Q_OBJECT
public:
    explicit FormClassWizardPage(QWidget * parent = 0);
    ~FormClassWizardPage();

    void setSuffixes(const QString &header, const QString &source,  const QString &form);

    virtual bool isComplete () const;
    virtual bool validatePage();

    int uiClassEmbedding() const;
    bool hasRetranslationSupport() const;
    QString path() const;

    // Fill out applicable parameters
    void getParameters(FormClassWizardParameters *) const;

public slots:
    void setClassName(const QString &suggestedClassName);
    void setPath(const QString &);
    void setRetranslationSupport(bool);
    void setUiClassEmbedding(int v);

private slots:
    void slotValidChanged();

private:
    void saveSettings();
    void restoreSettings();

    Ui::FormClassWizardPage *m_ui;
    bool m_isValid;
};

} // namespace Internal
} // namespace Designer

#endif //FORMCLASSWIZARDPAGE_H
