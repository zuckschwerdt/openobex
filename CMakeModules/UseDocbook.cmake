
find_package ( Xslt )
if ( NOT DOCBOOK_XSLT_PROCESSOR )
  if ( XSLTPROC )
    set ( DOCBOOK_XSLT_PROCESSOR "xsltproc" )
  elseif ( SAXON_COMMAND )
    set ( DOCBOOK_XSLT_PROCESSOR "saxon" )
  elseif ( XALAN2_COMMAND )
    set ( DOCBOOK_XSLT_PROCESSOR "xalan2" )
  endif ( XSLTPROC )
endif ( NOT DOCBOOK_XSLT_PROCESSOR )

set ( DOCBOOK_XSLT_PROCESSOR "${DOCBOOK_XSLT_PROCESSOR}"
      CACHE STRING "Docbook XSLT processor select (xsltproc, xalan2 or saxon)" )
set ( XSLT_PROCESSOR  "${DOCBOOK_XSLT_PROCESSOR}" )
include ( ${Xslt_USE_FILE} )

set ( DOCBOOK_XSL_PREFIX "http://docbook.sourceforge.net/release/xsl/current"
      CACHE STRING "prefix to locate the docbook-XSL release files" )
mark_as_advanced ( DOCBOOK_XSL_PREFIX )

function ( _DOCBOOK_GET_REFENTRY_IDS inFile outIdList )
  # This assumes that every refentry has an unique id attribute
  file ( READ "${inFile}" XML_FILE_CONTENTS )
  string ( REGEX MATCHALL "<refentry[ ]+[^>]*" XML_REFENTRYTITLE "${XML_FILE_CONTENTS}" )
  foreach ( id ${XML_REFENTRYTITLE} )
    string ( REGEX REPLACE ".*id=\"([^\"]*)\".*" "\\1" id "${id}" )
    list ( APPEND FILES ${id} )
  endforeach ( id )
  set ( ${outIdList} ${FILES} PARENT_SCOPE )
endfunction ( )

function ( _DOCBOOK_GET_MANPAGE_IDS inFile outNameList )
  file ( READ "${inFile}" XML_FILE_CONTENTS )
  string ( REPLACE ";" "" XML_FILE_CONTENTS "${XML_FILE_CONTENTS}" )
  string ( REGEX MATCHALL "<refentry[ ]+.*</refentry>" ENTRIES "${XML_FILE_CONTENTS}" )
  string ( REPLACE "</refentry>" ";" ENTRIES "${ENTRIES}" )
  list ( LENGTH ENTRIES COUNT )
  foreach ( index RANGE ${COUNT} )
    if ( index LESS COUNT )
      list ( GET ENTRIES ${index} entry )
      string ( REGEX MATCH "<refentry[ ]+[^>]*" MANPAGE_NAME "${entry}" )
      string ( REGEX REPLACE ".*id=\"([^\"]*)\".*" "\\1" MANPAGE_NAME "${MANPAGE_NAME}" )

      string ( REGEX MATCH "<manvolnum>[^<]*" MANPAGE_VOLUME "${entry}" )
      string ( REGEX REPLACE "^<manvolnum>" "" MANPAGE_VOLUME "${MANPAGE_VOLUME}" )
      string ( REGEX REPLACE "[[:space:]]" "" MANPAGE_VOLUME "${MANPAGE_VOLUME}" )
      if ( MANPAGE_NAME AND MANPAGE_VOLUME )
        list ( APPEND FILES "${MANPAGE_NAME}.${MANPAGE_VOLUME}" )
      endif ( MANPAGE_NAME AND MANPAGE_VOLUME )
    endif ( index LESS COUNT )
  endforeach ( index )
  set ( ${outNameList} ${FILES} PARENT_SCOPE )
endfunction ( )

function ( _DOCBOOK_MANPAGE inFile outList )
  _docbook_get_manpage_ids ( "${inFile}" FILES )

  xsl_transform (
    "${DOCBOOK_XSL_PREFIX}/manpages/docbook.xsl"
    "${inFile}"
    ${FILES}
  )

  set ( ${outList} ${FILES} PARENT_SCOPE )
endfunction ( )

function ( _DOCBOOK_HTML inFile outList )
  _docbook_get_refentry_ids ( "${inFile}" REFTITLES )
  foreach ( file  ${REFTITLES} )
    list ( APPEND FILES ${file}.html )
  endforeach ( file )
  # This assumes that the entry point is 'reference' with
  # one or more refentries.
  list ( APPEND FILES index.html )

  xsl_transform (
    "${DOCBOOK_XSL_PREFIX}/xhtml/chunk.xsl"
    "${inFile}"
    ${FILES}
  )

  set ( ${outList} ${FILES} PARENT_SCOPE )
endfunction ( )

function ( DOCBOOK_GENERATE format inFile outList )
  if ( format STREQUAL "manpage" )
    _docbook_manpage ( "${inFile}" ${outList} )

  elseif ( format STREQUAL "html" )
    _docbook_html ( "${inFile}" ${outList} )

  else ( )
    message ( FATAL_ERROR "Unsupported docbook output format." )

  endif ( )
  set ( ${outList} ${${outList}} PARENT_SCOPE )
endfunction ( )
