Based on WiX (http://sourceforge.net/projects/wix/) I create an installer for
Windows. This installer includes all necessary headers, libs and DLLs (e.g. zlib,
sqlite, xml2, apr). You find the XML description of the installer in jxta.wxs.
With the following commands you can create the installer:

candle jxta.wxs
light jxta.wixobj
