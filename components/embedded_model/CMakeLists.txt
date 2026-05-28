idf_component_register(
    SRCS
        "xgboost_model_m2cgen.c"
        "xgboost_error_postprocess.c"
    INCLUDE_DIRS
        "include"
)

target_compile_options(
    ${COMPONENT_LIB}
    PRIVATE
        -Os
        -mtext-section-literals
        -fno-var-tracking
        -fno-var-tracking-assignments
)
