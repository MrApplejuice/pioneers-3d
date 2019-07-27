<?xml version='1.0'?> 
<xsl:stylesheet  
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"  version="1.0"> 
<xsl:param name="generate.toc">
book  toc,title,figure,table,example
part  toc,title
</xsl:param> 
<xsl:param name="formal.title.placement">
figure after
example before
table after
procedure before
</xsl:param>
<xsl:param name="section.autolabel" selected="1"/>
<xsl:param name="section.label.includes.component.label" selected="1"/>
<xsl:param name="section.autolabel.max.depth" selected="1"/>
<xsl:param name="htmlhelp.chm">pioneers.chm</xsl:param>
</xsl:stylesheet>
