#include "commentssettings.h"

#include <QtCore/QSettings>

using namespace CppTools;

namespace {

const char kDocumentationCommentsGroup[] = "DocumentationComments";
const char kEnableDoxygenBlocks[] = "EnableDoxygenBlocks";
const char kGenerateBrief[] = "GenerateBrief";
const char kAddLeadingAsterisks[] = "AddLeadingAsterisks";

}

CommentsSettings::CommentsSettings()
    : m_enableDoxygen(true)
    , m_generateBrief(true)
    , m_leadingAsterisks(true)
{}

void CommentsSettings::toSettings(const QString &category, QSettings *s) const
{
    s->beginGroup(category + QLatin1String(kDocumentationCommentsGroup));
    s->setValue(QLatin1String(kEnableDoxygenBlocks), m_enableDoxygen);
    s->setValue(QLatin1String(kGenerateBrief), m_generateBrief);
    s->setValue(QLatin1String(kAddLeadingAsterisks), m_leadingAsterisks);
    s->endGroup();
}

void CommentsSettings::fromSettings(const QString &category, QSettings *s)
{
    s->beginGroup(category + QLatin1String(kDocumentationCommentsGroup));
    m_enableDoxygen = s->value(QLatin1String(kEnableDoxygenBlocks), true).toBool();
    m_generateBrief = m_enableDoxygen
            && s->value(QLatin1String(kGenerateBrief), true).toBool();
    m_leadingAsterisks = s->value(QLatin1String(kAddLeadingAsterisks), true).toBool();
    s->endGroup();
}

bool CommentsSettings::equals(const CommentsSettings &other) const
{
    return m_enableDoxygen == other.m_enableDoxygen
            && m_generateBrief == other.m_generateBrief
            && m_leadingAsterisks == other.m_leadingAsterisks;
}
