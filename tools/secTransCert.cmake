find_package(OpenSSL CONFIG REQUIRED)

find_program(
        OpenSSL_bin
        openssl
        REQUIRED
)

set(SECTRANS_RSA_PATH "${CMAKE_BINARY_DIR}/RSA")

set(SECTRANS_PRIVKEY "${SECTRANS_RSA_PATH}/privateKey.pem")
set(SECTRANS_PUBKEY "${SECTRANS_RSA_PATH}/publicKey.pem")

add_custom_command(
        OUTPUT ${SECTRANS_PRIVKEY}
        MAIN_DEPENDENCY ${OpenSSL_bin}
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SECTRANS_RSA_PATH}"
        COMMAND "${OpenSSL_bin}" genrsa -out "${SECTRANS_PRIVKEY}" 2048
        COMMAND "${OpenSSL_bin}" rsa -in "${SECTRANS_PRIVKEY}" -pubout -out "${SECTRANS_PUBKEY}"
        COMMENT "Generating server RSA keys"
        VERBATIM
)

add_custom_target(sectrans_certs ALL
        DEPENDS "${SECTRANS_PRIVKEY}" "${SECTRANS_PUBKEY}"
)