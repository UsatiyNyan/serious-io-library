cpmaddpackage(
        NAME serious-execution-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-execution-library.git"
        GIT_TAG main)

sl_target_link_system_libraries(${PROJECT_NAME}
        PUBLIC
        sl::exec
)
