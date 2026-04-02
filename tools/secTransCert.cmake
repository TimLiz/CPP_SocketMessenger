find_package(OpenSSL CONFIG REQUIRED)

find_program(
        OpenSSL_bin
        openssl
        REQUIRED
)

set(SECTRANS_TLS_PATH "${CMAKE_BINARY_DIR}/TLS")

set(SECTRANS_PRIVKEY "${SECTRANS_TLS_PATH}/privateKey.pem")
set(SECTRANS_CHAIN "${SECTRANS_TLS_PATH}/chain.pem")

add_custom_command(
        OUTPUT ${SECTRANS_PRIVKEY} ${SECTRANS_CHAIN}
        MAIN_DEPENDENCY ${OpenSSL_bin}
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SECTRANS_TLS_PATH}"
        COMMAND "${OpenSSL_bin}" genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out "${SECTRANS_PRIVKEY}"
        COMMAND "${OpenSSL_bin}" req -new -x509 -key "${SECTRANS_PRIVKEY}" -out "${SECTRANS_CHAIN}" -days 9999
        COMMENT "Generating TLS chain"
        VERBATIM
)

add_custom_target(sectrans_certs ALL
        DEPENDS "${SECTRANS_PRIVKEY}" "${SECTRANS_CHAIN}"
)