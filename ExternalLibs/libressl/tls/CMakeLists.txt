set(
	TLS_SRC
	tls.c
	tls_bio_cb.c
	tls_client.c
	tls_config.c
	tls_conninfo.c
	tls_keypair.c
	tls_server.c
	tls_ocsp.c
	tls_peer.c
	tls_util.c
	tls_verify.c
)

if(WIN32)
	set(
		TLS_SRC
		${TLS_SRC}
		compat/ftruncate.c
		compat/getuid.c
		compat/pread.c
		compat/pwrite.c
	)
endif()

if(NOT "${OPENSSLDIR}" STREQUAL "")
	add_definitions(-D_PATH_SSL_CA_FILE=\"${OPENSSLDIR}/cert.pem\")
else()
	add_definitions(-D_PATH_SSL_CA_FILE=\"${CMAKE_INSTALL_PREFIX}/etc/ssl/cert.pem\")
endif()

add_library(tls STATIC ${TLS_SRC})
set_target_properties(tls PROPERTIES FOLDER "ExternalLibs/")

target_include_directories(tls PRIVATE . ../include/compat PUBLIC ../include)
target_link_libraries(tls ssl crypto ${PLATFORM_LIBS})




