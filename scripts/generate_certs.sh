#!/bin/bash

set -e  # Exit on any error

# Set certificate directory relative to project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "${SCRIPT_DIR}")"
CERT_DIR="${PROJECT_ROOT}/certs"
SERVER_CERT_DIR="${PROJECT_ROOT}/server_certs"
CLIENT_CERT_DIR="${PROJECT_ROOT}/client_certs"

# Create fresh directories
rm -rf "${CERT_DIR}" "${SERVER_CERT_DIR}" "${CLIENT_CERT_DIR}"
mkdir -p "${CERT_DIR}" "${SERVER_CERT_DIR}" "${CLIENT_CERT_DIR}"
cd "${CERT_DIR}"

echo "Generating CA certificate..."
# Generate CA private key with stronger parameters
openssl genrsa -out ca.key 2048

# Generate CA certificate
openssl req -x509 -new -nodes -key ca.key -sha256 -days 3650 -out ca.crt \
    -subj "/C=US/ST=State/L=City/O=VPN-CA/CN=VPN Root CA"

echo "Generating server certificate..."
# Generate server private key
openssl genrsa -out server.key 2048

# Generate server CSR
openssl req -new -key server.key -out server.csr \
    -subj "/C=US/ST=State/L=City/O=VPN-Server/CN=VPN Server"

# Sign server certificate with CA
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
    -out server.crt -days 365 -sha256 \
    -extensions v3_server -extfile <(cat <<-EOF
[v3_server]
basicConstraints=CA:FALSE
keyUsage = digitalSignature, keyEncipherment
extendedKeyUsage = serverAuth
EOF
)

echo "Generating client certificate..."
# Generate client private key
openssl genrsa -out client.key 2048

# Generate client CSR
openssl req -new -key client.key -out client.csr \
    -subj "/C=US/ST=State/L=City/O=VPN-Client/CN=VPN Client"

# Sign client certificate with CA
openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
    -out client.crt -days 365 -sha256 \
    -extensions v3_client -extfile <(cat <<-EOF
[v3_client]
basicConstraints=CA:FALSE
keyUsage = digitalSignature, keyEncipherment
extendedKeyUsage = clientAuth
EOF
)

# Move server certificates to server directory
mv server.* "${SERVER_CERT_DIR}/"
cp ca.crt "${SERVER_CERT_DIR}/"

# Move client certificates to client directory
mv client.* "${CLIENT_CERT_DIR}/"
cp ca.crt "${CLIENT_CERT_DIR}/"

# Clean up
rm -rf "${CERT_DIR}"

# Set permissions for server certs
chmod 644 "${SERVER_CERT_DIR}"/*.crt
chmod 600 "${SERVER_CERT_DIR}"/*.key

# Set permissions for client certs
chmod 644 "${CLIENT_CERT_DIR}"/*.crt
chmod 600 "${CLIENT_CERT_DIR}"/*.key

# Set ownership
chown $(whoami):$(whoami) "${SERVER_CERT_DIR}"/* "${CLIENT_CERT_DIR}"/*

# Verify certificates
echo "Verifying certificate content..."
for cert in "${SERVER_CERT_DIR}/server.crt" "${CLIENT_CERT_DIR}/client.crt"; do
    echo "Checking $cert:"
    openssl x509 -in $cert -text -noout | grep "Subject:" || exit 1
done

echo "Verifying certificate chain..."
openssl verify -CAfile "${SERVER_CERT_DIR}/ca.crt" "${SERVER_CERT_DIR}/server.crt"
openssl verify -CAfile "${CLIENT_CERT_DIR}/ca.crt" "${CLIENT_CERT_DIR}/client.crt"

# Display file sizes to confirm content
echo -e "\nCertificate sizes:"
ls -l "${SERVER_CERT_DIR}" "${CLIENT_CERT_DIR}"

echo -e "\nCertificate paths:"
echo "Server certs: ${SERVER_CERT_DIR}"
echo "Client certs: ${CLIENT_CERT_DIR}"
