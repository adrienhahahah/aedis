if(NOT "${CONAN_BOOST_ROOT}" STREQUAL "")
	set(Boost_FOUND TRUE)
	foreach(library ${CONAN_PKG_LIBS_BOOST})
		string(SUBSTRING "${library}" 6 -1 simple_lib_name)
		if(NOT TARGET Boost::${simple_lib_name})
			add_library(Boost::${simple_lib_name} INTERFACE IMPORTED)
			target_link_libraries(Boost::${simple_lib_name} INTERFACE CONAN_LIB::boost_${library})
			target_include_directories(Boost::${simple_lib_name} INTERFACE $<TARGET_PROPERTY:CONAN_PKG::boost,INTERFACE_INCLUDE_DIRECTORIES>)
			target_compile_definitions(Boost::${simple_lib_name} INTERFACE $<TARGET_PROPERTY:CONAN_PKG::boost,INTERFACE_COMPILE_DEFINITIONS>)
			target_compile_options(Boost::${simple_lib_name} INTERFACE $<TARGET_PROPERTY:CONAN_PKG::boost,INTERFACE_COMPILE_OPTIONS>)
			target_compile_features(Boost::${simple_lib_name} INTERFACE $<TARGET_PROPERTY:CONAN_PKG::boost,INTERFACE_COMPILE_FEATURES>)
		endif()
	endforeach()
else()
	set(Boost_FOUND FALSE)
endif()