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

#ifndef QTCREATORINTEGRATION_H
#define QTCREATORINTEGRATION_H

#include <cpptools/ModelManagerInterface.h>

#if QT_VERSION >= 0x050000
#    include <QDesignerIntegration>
#else
#    include "qt_private/qdesigner_integration_p.h"
#endif

QT_FORWARD_DECLARE_CLASS(QUrl)

namespace Designer {
namespace Internal {

class FormEditorW;

class QtCreatorIntegration :
#if QT_VERSION >= 0x050000
    public QDesignerIntegration {
#else
    public qdesigner_internal::QDesignerIntegration {
#endif
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
