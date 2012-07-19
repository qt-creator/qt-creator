/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef HTML5APPWIZARDPAGES_H
#define HTML5APPWIZARDPAGES_H

#include <QWizardPage>
#include "html5app.h"

namespace Qt4ProjectManager {
namespace Internal {

class Html5AppWizardOptionsPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit Html5AppWizardOptionsPage(QWidget *parent = 0);
    virtual ~Html5AppWizardOptionsPage();

    Html5App::Mode mainHtmlMode() const;
    QString mainHtmlData() const;
    void setTouchOptimizationEndabled(bool enabled);
    bool touchOptimizationEndabled() const;
    virtual bool isComplete() const;

private slots:
    void setLineEditsEnabled();

private:
    class Html5AppWizardOptionsPagePrivate *d;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // HTML5APPWIZARDPAGES_H
