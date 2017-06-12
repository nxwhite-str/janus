if(NOT ${JANUS_IMPLEMENTATION} STREQUAL "")
  set(UTILS
      janus_cluster.cpp
      janus_create_gallery.cpp
      janus_create_templates.cpp
      janus_create_templates2.cpp
      janus_detect.cpp
      janus_search.cpp
      janus_verify.cpp
      )
  foreach(UTIL ${UTILS})
    get_filename_component(UTIL_NAME ${UTIL} NAME_WE)
    add_executable(${UTIL_NAME} ${UTIL} ${JANUS_HEADERS})
    target_link_libraries(${UTIL_NAME} ${JANUS_IMPLEMENTATION} ${JANUS_IO_IMPLEMENTATION})
    install(TARGETS ${UTIL_NAME} RUNTIME DESTINATION bin)
  endforeach()
endif()
