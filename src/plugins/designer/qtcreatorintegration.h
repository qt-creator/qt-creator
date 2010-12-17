/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QTCREATORINTEGRATION_H
#define QTCREATORINTEGRATION_H

#include <cpptools/cppmodelmanagerinterface.h>

#include "qt_private/qdesigner_integration_p.h"

QT_FORWARD_DECLARE_CLASS(QUrl)

namespace Designer {
namespace Internal {

class FormEditorW;

class QtCreatorIntegration : public qdesigner_internal::QDesignerIntegration {
    Q_OBJECT
public:
    explicit QtCreatorIntegration(QDesignerFormEditorInterface *core, FormEditorW *parent = 0);

    QWidget *containerWindow(QWidget *widget) const;

    bool supportsToSlotNavigation() { return true; }

signals:
    void creatorHelpRequested(const QUrl &url);

public slots:
    void updateSelection();

private slots:
    void slotNavigateToSlot(const QString &objectName, const QString &signalSignature, const QStringList &parameterNames);
    void slotDesignerHelpRequested(const QString &manual, const QString &document);
    void slotSyncSettingsToDesigner();

private:
    bool navigateToSlot(const QString &objectName,
                        const QString &signalSignature,
                        const QStringList &parameterNames,
                        QString *errorMessage);
    FormEditorW *m_few;
};

} // namespace Internal
} // namespace Designer

#endif // QTCREATORINTEGRATION_H
