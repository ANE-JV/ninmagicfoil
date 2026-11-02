# FetchContent may repeat PATCH_COMMAND when reconfiguring an existing cache.
# Accept only an applicable patch or the exact patch already applied.
if(NOT DEFINED GIT_EXECUTABLE OR NOT DEFINED PATCH_FILE)
    message(FATAL_ERROR "GIT_EXECUTABLE and PATCH_FILE are required")
endif()
execute_process(COMMAND "${GIT_EXECUTABLE}" apply --check --ignore-whitespace "${PATCH_FILE}"
    RESULT_VARIABLE can_apply OUTPUT_QUIET ERROR_QUIET)
if(can_apply EQUAL 0)
    execute_process(COMMAND "${GIT_EXECUTABLE}" apply --ignore-whitespace "${PATCH_FILE}"
        RESULT_VARIABLE applied)
    if(NOT applied EQUAL 0)
        message(FATAL_ERROR "Failed to apply ${PATCH_FILE}")
    endif()
else()
    execute_process(COMMAND "${GIT_EXECUTABLE}" apply --reverse --check --ignore-whitespace "${PATCH_FILE}"
        RESULT_VARIABLE already_applied OUTPUT_QUIET ERROR_QUIET)
    if(NOT already_applied EQUAL 0)
        message(FATAL_ERROR "Patch is neither applicable nor already applied: ${PATCH_FILE}")
    endif()
endif()
