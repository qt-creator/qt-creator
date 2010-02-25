/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QMLDESIGNERPLUGIN_H
#define QMLDESIGNERPLUGIN_H

#include <qmldesigner/designersettings.h>

#include <extensionsystem/iplugin.h>

namespace Core {
    class IWizard;
    class ICore;
    class IEditorFactory;
    class IEditor;
}

namespace QmlDesigner {
    class IntegrationCore;
}

namespace QmlDesigner {
namespace Internal {

class BauhausPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    BauhausPlugin();
    virtual ~BauhausPlugin();

    //Plugin
    virtual bool initialize(const QStringList &arguments, QString *error_message = 0);
    virtual void extensionsInitialized();

    static BauhausPlugin *pluginInstance();

    DesignerSettings settings() const;
    void setSettings(const DesignerSettings &s);

private slots:
    void switchTextDesign();

private:
    QmlDesigner::IntegrationCore *m_designerCore;
    static BauhausPlugin *m_pluginInstance;
    DesignerSettings m_settings;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // QMLDESIGNERPLUGIN_H
