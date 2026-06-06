project(PosiStageNet)

set(POSISTAGENET_DEPS)
if(WIN32)
	list(APPEND POSISTAGENET_DEPS ws2_32)
endif()

bbb_add_external(
	DEPS ${POSISTAGENET_DEPS}
	INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/psn-cpp/include
)
