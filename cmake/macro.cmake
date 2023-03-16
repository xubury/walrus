function(file_download path url)
  get_filename_component(filename ${path} NAME)
  message(STATUS "Downloading ${filename} from ${url}")
  file(
    DOWNLOAD ${url} ${path}
    TLS_VERIFY ON
    STATUS DOWNLOAD_RESULT)
  list(GET DOWNLOAD_RESULT 0 DOWNLOAD_RESULT_CODE)
  if(NOT DOWNLOAD_RESULT_CODE EQUAL 0)
    file(REMOVE ${path})
    message(FATAL_ERROR "Download failed with error: ${DOWNLOAD_RESULT}")
  endif()
  message(STATUS "Download finished")
endfunction()
