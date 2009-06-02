
find_file ( JAVA_DOCBOOK_XSL_SAXON_LIBRARY
  NAMES docbook-xsl-saxon_1.00.jar
  PATH_SUFFIXES share/java
  DOC "location of saxon 6.5.x DocBook XSL extension JAR file"
  CMAKE_FIND_ROOT_PATH_BOTH
)
mark_as_advanced ( JAVA_DOCBOOK_XSL_SAXON_LIBRARY )
set ( Xslt_SAXON_EXTENSIONS "${JAVA_DOCBOOK_XSL_SAXON_LIBRARY}" )

find_package ( Xslt )
if ( Xslt_FOUND )
  if ( NOT DOCBOOK_XSLT_PROCESSOR )
    if ( XSLT_XSLTPROC_EXECUTABLE )
      set ( DOCBOOK_XSLT_PROCESSOR "xsltproc" )
    elseif ( XSLT_SAXON_COMMAND AND JAVA_DOCBOOK_XSL_SAXON_LIBRARY )
      set ( DOCBOOK_XSLT_PROCESSOR "saxon" )
    elseif ( XSLT_XALAN2_COMMAND )
      set ( DOCBOOK_XSLT_PROCESSOR "xalan2" )
    endif ( XSLT_XSLTPROC_EXECUTABLE )
  endif ( NOT DOCBOOK_XSLT_PROCESSOR )

  set ( DOCBOOK_XSLT_PROCESSOR "${DOCBOOK_XSLT_PROCESSOR}"
        CACHE STRING "Docbook XSLT processor select (xsltproc, xalan2 or saxon)" )
  set ( XSLT_PROCESSOR  "${DOCBOOK_XSLT_PROCESSOR}" )

  if ( DOCBOOK_XSLT_PROCESSOR )
    set ( Docbook_FOUND true )
    set ( Docbook_USE_FILE UseDocbook )
  endif ( DOCBOOK_XSLT_PROCESSOR )
endif ( Xslt_FOUND )

if ( NOT Docbook_FOUND )
  if ( NOT Docbook_FIND_QUIETLY )
    message ( STATUS "No supported Docbook XSLT found." )
  endif ( NOT Docbook_FIND_QUIETLY )
  if ( Docbook_FIND_REQUIRED )
    message ( FATAL_ERROR "No supported Docbook XSLT found but it is required." )
  endif ( Docbook_FIND_REQUIRED )
endif ( NOT Docbook_FOUND )
