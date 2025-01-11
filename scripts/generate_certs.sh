#!/bin/bash

# Set certificate directory relative to project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "${SCRIPT_DIR}")"
CERT_DIR="${PROJECT_ROOT}/certs"

# Create certificates directory if it doesn't exist
mkdir -p "${CERT_DIR}"

# Generate private key
openssl genpkey -algorithm RSA -out "${CERT_DIR}/server.key"

# Generate self-signed certificate
openssl req -new -x509 -key "${CERT_DIR}/server.key" -out "${CERT_DIR}/server.crt" -days 365 \
    -subj "/C=US/ST=State/L=City/O=Organization/CN=VPN Server"

# Set permissions
chmod 600 "${CERT_DIR}/server.key"
chmod 644 "${CERT_DIR}/server.crt"

# After generating certificates, make them readable by all users
chmod 644 "${CERT_DIR}/server.crt"
chmod 644 "${CERT_DIR}/server.key"  # Note: In production, keep this restricted

echo "Certificate absolute paths:"
echo "Certificate: $(realpath ${CERT_DIR}/server.crt)"
echo "Private key: $(realpath ${CERT_DIR}/server.key)"

echo "Certificates generated in: ${CERT_DIR}"
echo "Please ensure the application has read access to these files"
ls -l "${CERT_DIR}"
