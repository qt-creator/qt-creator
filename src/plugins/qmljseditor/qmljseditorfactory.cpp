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

#include "qmljseditorfactory.h"
#include "qmljseditoreditable.h"
#include "qmljseditor.h"
#include "qmljseditoractionhandler.h"
#include "qmljseditorconstants.h"
#include "qmljseditorplugin.h"

#include <qmljstools/qmljstoolsconstants.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <coreplugin/icore.h>
#include <coreplugin/infobar.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>
#include <QSettings>
#include <QMessageBox>
#include <QPushButton>

using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJSEditor::Constants;

QmlJSEditorFactory::QmlJSEditorFactory(QObject *parent)
  : Core::IEditorFactory(parent)
{
    m_mimeTypes
            << QLatin1String(QmlJSTools::Constants::QML_MIMETYPE)
            << QLatin1String(QmlJSTools::Constants::QMLPROJECT_MIMETYPE)
            << QLatin1String(QmlJSTools::Constants::QBS_MIMETYPE)
            << QLatin1String(QmlJSTools::Constants::QMLTYPES_MIMETYPE)
            << QLatin1String(QmlJSTools::Constants::JS_MIMETYPE)
            << QLatin1String(QmlJSTools::Constants::JSON_MIMETYPE)
            ;
}

Core::Id QmlJSEditorFactory::id() const
{
    return Core::Id(C_QMLJSEDITOR_ID);
}

QString QmlJSEditorFactory::displayName() const
{
    return qApp->translate("OpenWith::Editors", C_QMLJSEDITOR_DISPLAY_NAME);
}

Core::IEditor *QmlJSEditorFactory::createEditor(QWidget *parent)
{
    QmlJSEditor::QmlJSTextEditorWidget *rc = new QmlJSEditor::QmlJSTextEditorWidget(parent);
    QmlJSEditorPlugin::instance()->initializeEditor(rc);
    return rc->editor();
}

QStringList QmlJSEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}
