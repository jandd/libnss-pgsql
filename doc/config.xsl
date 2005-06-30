<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                version="1.0">

  <!-- Use ids for filenames -->
  <xsl:param name="use.id.as.filename" select="'1'"/>

  <!-- Turn on admonition graphics. -->
  <xsl:param name="admon.graphics" select="'1'"/>
  <xsl:param name="admon.graphics.path"></xsl:param>

  <!-- HTML generation toggles -->
  <xsl:param name="make.valid.html" select="1"/>
  <xsl:param name="html.cleanup" select="1"/>
</xsl:stylesheet>
