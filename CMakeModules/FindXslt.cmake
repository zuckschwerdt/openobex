#
# xsltproc is preferred because it creates better output and is faster.
#
# xalan2 can also be used by adding xalan2.jar, xml-apis.jar and xercesImpl.jar
# to the classpath, then runinng
#   org.apache.xalan.xslt.Process
# There is also a xalan (1.10) binary of that name that did not work.
#
# Saxon-6.5.5 can also be used by adding saxon.jar and maybe docbook saxon
# extension jar fileto classpath, then running
#   com.icl.saxon.StyleSheet
# Debian has wrapper shell scripts for saxon-6.5.5, called saxon-xslt.
# Saxon-B 9.1 did not work, Saxon-SA was not tested.
#
# Sablotron (1.0.3) does not work.
#
find_package ( Java )
if ( JAVA_RUNTIME )
  if ( NOT JAVA_CLASSPATH )
    set ( JAVA_CLASSPATH $ENV{CLASSPATH} CACHE STRING "java classpath" )
  endif ( NOT JAVA_CLASSPATH )
  set ( CLASSPATH ${JAVA_CLASSPATH} )

  find_file ( JAVA_RESOLVER_LIBRARY
    NAMES resolver.jar xml-commons-resolver-1.1.jar
    PATH_SUFFIXES share/java
    DOC "location of the XML commons resolver java library from the apache project"
    CMAKE_FIND_ROOT_PATH_BOTH
  )
  mark_as_advanced ( JAVA_RESOLVER_LIBRARY )
  if ( JAVA_RESOLVER_LIBRARY )
    if ( CLASSPATH )
      set ( CLASSPATH "${CLASSPATH}:${JAVA_RESOLVER_LIBRARY}" )
    else ( CLASSPATH )
      set ( CLASSPATH "${JAVA_RESOLVER_LIBRARY}" )
    endif ( CLASSPATH )
  endif ( JAVA_RESOLVER_LIBRARY )

  find_path ( JAVA_PROPERTIES_CATALOGMANAGER
    NAMES CatalogManager.properties
    PATHS /etc
    PATH_SUFFIXES xml/resolver share/java share/xml
    DOC "location of the catalog manager properties file from the XML commons resolver"
    CMAKE_FIND_ROOT_PATH_BOTH
  )
  if ( JAVA_PROPERTIES_CATALOGMANAGER )
    if ( CLASSPATH )
      set ( CLASSPATH "${CLASSPATH}:${JAVA_PROPERTIES_CATALOGMANAGER}" )
    else ( CLASSPATH )
      set ( CLASSPATH "${JAVA_PROPERTIES_CATALOGMANAGER}" )
    endif ( CLASSPATH )
  endif ( JAVA_PROPERTIES_CATALOGMANAGER )

  #
  # Find Xalan 2
  #
  find_file ( XALAN2
    NAMES xalan2.jar
    PATH_SUFFIXES share/java
    DOC "location of Xalan2 JAR file"
    CMAKE_FIND_ROOT_PATH_BOTH
  )
  mark_as_advanced ( XALAN2 )
  find_file ( JAVA_XML_APIS_LIBRARY
    NAMES xml-apis.jar
    PATH_SUFFIXES share/java
    DOC "location of Xalan2 XML-API JAR file"
    CMAKE_FIND_ROOT_PATH_BOTH
  )
  mark_as_advanced ( JAVA_XML_APIS_LIBRARY )
  find_file ( JAVA_XERCES_IMPL_LIBRARY
    NAMES xercesImpl.jar
    PATH_SUFFIXES share/java
    DOC "location of Xalan2 Xerces Implementation JAR file"
    CMAKE_FIND_ROOT_PATH_BOTH
  )
  mark_as_advanced ( JAVA_XERCES_IMPL_LIBRARY )
  if ( XALAN2 AND JAVA_XML_APIS_LIBRARY AND JAVA_XERCES_IMPL_LIBRARY )
    set ( XALAN2_COMMAND
      ${JAVA_RUNTIME}
      -cp "${CLASSPATH}:${XALAN2}:${JAVA_XML_APIS_LIBRARY}:${JAVA_XERCES_IMPL_LIBRARY}"
      org.apache.xalan.xslt.Process
    )
    if ( JAVA_RESOLVER_LIBRARY AND JAVA_PROPERTIES_CATALOGMANAGER )
      list ( APPEND XALAN2_COMMAND
	-ENTITYRESOLVER  org.apache.xml.resolver.tools.CatalogResolver
	-URIRESOLVER  org.apache.xml.resolver.tools.CatalogResolver
      )
    endif ( JAVA_RESOLVER_LIBRARY AND JAVA_PROPERTIES_CATALOGMANAGER )
  endif ( XALAN2 AND JAVA_XML_APIS_LIBRARY AND JAVA_XERCES_IMPL_LIBRARY )

  #
  # Find Saxon 6.5.x
  #
  find_file ( SAXON
    NAMES saxon.jar saxon-6.5.5.jar
    PATH_SUFFIXES share/java
    DOC "location of saxon 6.5.x JAR file"
    CMAKE_FIND_ROOT_PATH_BOTH
  )
  mark_as_advanced ( SAXON )
  find_file ( JAVA_DOCBOOK_XSL_SAXON_LIBRARY
    NAMES docbook-xsl-saxon_1.00.jar
    PATH_SUFFIXES share/java
    DOC "location of saxon 6.5.x DocBook XSL extension JAR file"
    CMAKE_FIND_ROOT_PATH_BOTH
  )
  mark_as_advanced ( JAVA_DOCBOOK_XSL_SAXON_LIBRARY )
  if ( SAXON AND JAVA_DOCBOOK_XSL_SAXON_LIBRARY )
    set ( SAXON_COMMAND
      ${JAVA_RUNTIME}
      -cp "${CLASSPATH}:${SAXON}:${JAVA_DOCBOOK_XSL_SAXON_LIBRARY}"
      com.icl.saxon.StyleSheet
    )
    if ( JAVA_RESOLVER_LIBRARY )
      list ( APPEND SAXON_COMMAND
	-x org.apache.xml.resolver.tools.ResolvingXMLReader
	-y org.apache.xml.resolver.tools.ResolvingXMLReader
	-u
      )
      if ( JAVA_PROPERTIES_CATALOGMANAGER )
	list ( APPEND SAXON_COMMAND
	  -r org.apache.xml.resolver.tools.CatalogResolver
	)
      endif ( JAVA_PROPERTIES_CATALOGMANAGER )
    endif ( JAVA_RESOLVER_LIBRARY )
  endif ( SAXON AND JAVA_DOCBOOK_XSL_SAXON_LIBRARY )
endif ( JAVA_RUNTIME )

find_program ( XSLTPROC
  NAMES xsltproc
  DOC   "path to the libxslt XSLT processor xsltproc"
)
mark_as_advanced ( XSLTPROC )

set ( Xslt_USE_FILE UseXslt )

if ( XSLTPROC OR SAXON OR XALAN2 )
  set ( Xslt_FOUND true )
endif ( XSLTPROC OR SAXON OR XALAN2 )

if ( NOT Xslt_FOUND )
  if ( NOT Xslt_FIND_QUIETLY )
    message ( STATUS "No supported XSLT processor found. Supported XSLT processors are: xsltproc, saxon-6.5.x, xalan-2.x" )
  endif ( NOT Xslt_FIND_QUIETLY )
  if ( Xslt_FIND_REQUIRED )
    message ( FATAL_ERROR "No supported XSLT processor found but it is required." )
  endif ( Xslt_FIND_REQUIRED )
endif ( NOT Xslt_FOUND )
