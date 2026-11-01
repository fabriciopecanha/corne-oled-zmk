if(NOT CONFIG_ZMK_SPLIT OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/fp_layer_state.c
    ${CMAKE_CURRENT_LIST_DIR}/src/behavior_fp_layer_hold.c
    ${CMAKE_CURRENT_LIST_DIR}/src/behavior_fp_layer_lock.c)
  target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
endif()
