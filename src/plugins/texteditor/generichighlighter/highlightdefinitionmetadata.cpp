/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "highlightdefinitionmetadata.h"

using namespace TextEditor;
using namespace Internal;

const QLatin1String HighlightDefinitionMetaData::kPriority("priority");
const QLatin1String HighlightDefinitionMetaData::kName("name");
const QLatin1String HighlightDefinitionMetaData::kExtensions("extensions");
const QLatin1String HighlightDefinitionMetaData::kMimeType("mimetype");
const QLatin1String HighlightDefinitionMetaData::kVersion("version");
const QLatin1String HighlightDefinitionMetaData::kUrl("url");

HighlightDefinitionMetaData::HighlightDefinitionMetaData() : m_priority(0)
{}

void HighlightDefinitionMetaData::setPriority(const int priority)
{ m_priority = priority; }

int HighlightDefinitionMetaData::priority() const
{ return m_priority; }

void HighlightDefinitionMetaData::setId(const QString &id)
{ m_id = id; }

const QString &HighlightDefinitionMetaData::id() const
{ return m_id; }

void HighlightDefinitionMetaData::setName(const QString &name)
{ m_name = name; }

const QString &HighlightDefinitionMetaData::name() const
{ return m_name; }

void HighlightDefinitionMetaData::setVersion(const QString &version)
{ m_version = version; }

const QString &HighlightDefinitionMetaData::version() const
{ return m_version; }

void HighlightDefinitionMetaData::setPatterns(const QStringList &patterns)
{ m_patterns = patterns; }

const QStringList &HighlightDefinitionMetaData::patterns() const
{ return m_patterns; }

void HighlightDefinitionMetaData::setMimeTypes(const QStringList &mimeTypes)
{ m_mimeTypes = mimeTypes; }

const QStringList &HighlightDefinitionMetaData::mimeTypes() const
{ return m_mimeTypes; }

void HighlightDefinitionMetaData::setUrl(const QUrl &url)
{ m_url = url; }

const QUrl &HighlightDefinitionMetaData::url() const
{ return m_url; }
