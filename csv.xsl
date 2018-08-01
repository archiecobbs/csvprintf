<?xml version="1.0" encoding="UTF-8"?>

<xsl:transform xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

    <xsl:output method="text" encoding="UTF-8" media-type="text/csv"/>

    <xsl:template match="/csv/row">
        <xsl:apply-templates select="*"/>
        <xsl:value-of select="'&#10;'"/>
    </xsl:template>

    <xsl:template match="/csv/row/*">
        <xsl:if test="position() &gt; 1">
            <xsl:value-of select="','"/>
        </xsl:if>
        <xsl:call-template name="enquote"/>
    </xsl:template>

    <xsl:template name="enquote">
        <xsl:param name="s" select="."/>
        <xsl:value-of select="'&quot;'"/>
        <xsl:call-template name="escape">
            <xsl:with-param name="s" select="$s"/>
        </xsl:call-template>
        <xsl:value-of select="'&quot;'"/>
    </xsl:template>

    <xsl:template name="escape">
        <xsl:param name="s" select="."/>
        <xsl:choose>
            <xsl:when test="contains($s, '&quot;')">
                <xsl:call-template name="escape">
                    <xsl:with-param name="s" select="substring-before($s, '&quot;')"/>
                </xsl:call-template>
                <xsl:value-of select="'&quot;&quot;'"/>
                <xsl:call-template name="escape">
                    <xsl:with-param name="s" select="substring-after($s, '&quot;')"/>
                </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="$s"/>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>

    <xsl:template match="@*|node()">
        <xsl:apply-templates select="*"/>
    </xsl:template>

</xsl:transform>
