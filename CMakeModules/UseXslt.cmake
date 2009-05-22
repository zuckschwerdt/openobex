#
# call as XSLT_TRANSFORM(<.xsl file URL> <input file> <output files...>)
#
# The following variables are supported:
# XSLT_PROCESSOR (mandatory):
#      select the xslt processor to use (xsltproc, xalan2, saxon)
# XSLT_PARAMS (optional):
#      a list which may contain param=value entries.
# XSLT_(XSLTPROC|XALAN2|SAXON)_OPTIONS (optional):
#      a list with extra options for those xslt processors.
#
function ( XSL_TRANSFORM xslurl infile )
  if ( XSLT_PARAMS )
    foreach ( param ${XSLT_PARAMS} )
      set ( param_name )
      string ( REGEX MATCH "[^=]+" param_name "${param}" )
      if ( param_name )
	set ( param_value )
	string ( REGEX REPLACE "[^=]+=(.*)" "\\1" param_value "${param}" )
	set ( XSLT_XSLTPROC_OPTIONS ${XSLT_XSLTPROC_OPTIONS} --param ${param_name} ${param_value} )
	set ( XSLT_XALAN2_OPTIONS ${XSLT_XALAN2_OPTIONS} -param ${param_name} ${param_value} )
      endif ( param_name )
    endforeach ( param )
  endif ( XSLT_PARAMS )

  if ( XSLT_PROCESSOR STREQUAL "xsltproc" )
    if ( XSLT_XSLTPROC_EXECUTABLE )
      set ( XSL_TRANSFORM_COMMAND
	${XSLT_XSLTPROC_EXECUTABLE}
	${XSLT_XSLTPROC_OPTIONS}
	"${xslurl}" "${infile}"
      )
    else ( XSLT_XSLTPROC_EXECUTABLE )
      message ( FATAL_ERROR "xsltproc not found" )
    endif ( XSLT_XSLTPROC_EXECUTABLE )

  elseif ( XSLT_PROCESSOR STREQUAL "saxon" )
    if ( XSLT_SAXON_COMMAND )
      set ( XSL_TRANSFORM_COMMAND
	${XSLT_SAXON_COMMAND}
	${XSLT_SAXON_OPTIONS}
	"${infile}" "${xslurl}"
	${Xslt_PARAMS}
      )
    else ( XSLT_SAXON_COMMAND )
      message ( FATAL_ERROR "Saxon-6.5.x not found" )
    endif ( XSLT_SAXON_COMMAND )

  elseif ( XSLT_PROCESSOR STREQUAL "xalan2" )
    # Xalan places the output in the source file's directory :-(
    get_filename_component ( infile_name "${infile}" NAME )
    add_custom_command (
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${infile_name}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${infile}" "${CMAKE_CURRENT_BINARY_DIR}/${infile_name}"
      DEPENDS ${infile}
      VERBATIM
    )
    set ( infile "${CMAKE_CURRENT_BINARY_DIR}/${infile_name}" )
    if ( XSLT_XALAN2_COMMAND )
      set ( XSL_TRANSFORM_COMMAND
	${XSLT_XALAN2_COMMAND}
	${XSLT_XALAN2_OPTIONS}
	-in "${infile}" -xsl "${xslurl}"
      )
    else ( XSLT_XALAN2_COMMAND )
      message ( FATAL_ERROR " Xalan 2.x not found" )
    endif ( XSLT_XALAN2_COMMAND )

  else ( XSLT_PROCESSOR STREQUAL "xsltproc" )
    message ( FATAL_ERROR "Unsupported XSLT processor" )
  endif ( XSLT_PROCESSOR STREQUAL "xsltproc" )

  add_custom_command (
    OUTPUT ${ARGN}
    COMMAND ${XSL_TRANSFORM_COMMAND}
    DEPENDS "${infile}"
    VERBATIM
  )
  set ( XSL_TRANSFORM_COMMAND )
endfunction ( XSL_TRANSFORM )
