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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef QDESIGNER_INTEGRATION_H
#define QDESIGNER_INTEGRATION_H

#include "shared_global_p.h"
#include <QDesignerIntegrationInterface>

#include <QObject>

QT_BEGIN_NAMESPACE

class QDesignerFormEditorInterface;
class QDesignerFormWindowInterface;
class QDesignerResourceBrowserInterface;

class QVariant;
class QWidget;

namespace qdesigner_internal {

struct Selection;
class QDesignerIntegrationPrivate;

class QDESIGNER_SHARED_EXPORT QDesignerIntegration: public QDesignerIntegrationInterface
{
    Q_OBJECT
public:
    explicit QDesignerIntegration(QDesignerFormEditorInterface *core, QObject *parent = 0);
    virtual ~QDesignerIntegration();

    static void requestHelp(const QDesignerFormEditorInterface *core, const QString &manual, const QString &document);

    virtual QWidget *containerWindow(QWidget *widget) const;

    // Load plugins into widget database and factory.
    static void initializePlugins(QDesignerFormEditorInterface *formEditor);
    void emitObjectNameChanged(QDesignerFormWindowInterface *formWindow, QObject *object,
                               const QString &newName, const QString &oldName);
    void emitNavigateToSlot(const QString &objectName, const QString &signalSignature, const QStringList &parameterNames);
    void emitNavigateToSlot(const QString &slotSignature);

    // Create a resource browser specific to integration. Language integration takes precedence
    virtual QDesignerResourceBrowserInterface *createResourceBrowser(QWidget *parent = 0);

    enum ResourceFileWatcherBehaviour {
        NoWatcher,
        ReloadSilently,
        PromptAndReload
    };

    ResourceFileWatcherBehaviour resourceFileWatcherBehaviour() const;
    bool isResourceEditingEnabled() const;
    bool isSlotNavigationEnabled() const;

    QString contextHelpId() const;

protected:

    void setResourceFileWatcherBehaviour(ResourceFileWatcherBehaviour behaviour); // PromptAndReload by default
    void setResourceEditingEnabled(bool enable); // true by default
    void setSlotNavigationEnabled(bool enable); // false by default

signals:
    void propertyChanged(QDesignerFormWindowInterface *formWindow, const QString &name, const QVariant &value);
    void objectNameChanged(QDesignerFormWindowInterface *formWindow, QObject *object, const QString &newName, const QString &oldName);
    void helpRequested(const QString &manual, const QString &document);

    void navigateToSlot(const QString &objectName, const QString &signalSignature, const QStringList &parameterNames);
    void navigateToSlot(const QString &slotSignature);

public slots:
    virtual void updateProperty(const QString &name, const QVariant &value, bool enableSubPropertyHandling);
    // Additional signals of designer property editor
    virtual void resetProperty(const QString &name);
    virtual void addDynamicProperty(const QString &name, const QVariant &value);
    virtual void removeDynamicProperty(const QString &name);

    virtual void updateActiveFormWindow(QDesignerFormWindowInterface *formWindow);
    virtual void setupFormWindow(QDesignerFormWindowInterface *formWindow);
    virtual void updateSelection();
    virtual void updateGeometry();
    virtual void activateWidget(QWidget *widget);

    void updateCustomWidgetPlugins();

private slots:
    void updatePropertyPrivate(const QString &name, const QVariant &value);

private:
    void initialize();
    void getSelection(Selection &s);
    QObject *propertyEditorObject();

    QDesignerIntegrationPrivate *m_d;
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // QDESIGNER_INTEGRATION_H
