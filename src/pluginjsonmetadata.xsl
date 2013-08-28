<?xml version="1.0"?>
<!-- XSL sheet to transform Qt Creator's pluginspec files into json files required
     for the new Qt 5 plugin system. -->
<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:strip-space elements="plugin vendor category description url"/>
<xsl:template match="/">
{
    <xsl:apply-templates/>
}
</xsl:template>
<xsl:template match="license"/>
<xsl:template match="dependencyList"/>
<xsl:template match="copyright"/>
<xsl:template match="argumentList"/>
<xsl:template match="vendor">
"Vendor" : "<xsl:apply-templates/>",
</xsl:template>
<xsl:template match="platform">
"Platform" : "<xsl:apply-templates/>",
</xsl:template>
<xsl:template match="category">
"Category" : "<xsl:apply-templates/>",
</xsl:template>
<xsl:template match="description">
"Description" : "<xsl:apply-templates/>",
</xsl:template>
<xsl:template match="url">
"Url" : "<xsl:apply-templates/>"
</xsl:template>
</xsl:stylesheet>
