/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DESIGNMODE_H
#define DESIGNMODE_H

#include <coreplugin/imode.h>

namespace Core {
class IEditor;

namespace Internal { class DesignModeCoreListener; }

/**
  * A global mode for Design pane - used by Bauhaus (QML Designer) and
  * Qt Designer. Other plugins can register themselves by registerDesignWidget()
  * and giving a list of mimetypes that the editor understands, as well as an instance
  * to the main editor widget itself.
  */

class DesignModePrivate;

class CORE_EXPORT DesignMode : public IMode
{
    Q_OBJECT

public:
    explicit DesignMode();
    virtual ~DesignMode();

    static DesignMode *instance();

    void setDesignModeIsRequired();
    bool designModeIsRequired() const;

    void registerDesignWidget(QWidget *widget,
                              const QStringList &mimeTypes,
                              const Context &context);
    void unregisterDesignWidget(QWidget *widget);

    QStringList registeredMimeTypes() const;

signals:
    void actionsUpdated(Core::IEditor *editor);

private slots:
    void updateActions();

private:
    void currentEditorChanged(IEditor *editor);
    void updateContext(IMode *newMode, IMode *oldMode);
    void setActiveContext(const Context &context);

    DesignModePrivate *d;
    friend class Internal::DesignModeCoreListener;
};

} // namespace Core

#endif // DESIGNMODE_H
