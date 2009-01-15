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

#ifndef WORKBENCHINTEGRATION_H
#define WORKBENCHINTEGRATION_H

#include <cpptools/cppmodelmanagerinterface.h>

#include <QtDesigner/private/qdesigner_integration_p.h>

namespace Designer {
namespace Internal {

class FormEditorW;

class WorkbenchIntegration : public qdesigner_internal::QDesignerIntegration {
    Q_OBJECT
public:
    WorkbenchIntegration(QDesignerFormEditorInterface *core, FormEditorW *parent = 0);

    QWidget *containerWindow(QWidget *widget) const;

    bool supportsToSlotNavigation() { return true; };

public slots:
    void updateSelection();
private slots:
    void slotNavigateToSlot(const QString &objectName, const QString &signalSignature, const QStringList &parameterNames);
private:
    bool navigateToSlot(const QString &objectName,
                        const QString &signalSignature,
                        const QStringList &parameterNames,
                        QString *errorMessage);
    FormEditorW *m_few;
};

} // namespace Internal
} // namespace Designer

#endif // WORKBENCHINTEGRATION_H
