
# Name of the application signing certificate
APPCERT = "\"Developer ID Application: Three Monkeys International Inc. (42EJ97Y7M3)\""

# Cert OU
CERT_OU = "\"42EJ97Y7M3\""

# Sha1 of the siging certificate
CERTSHA1 = 5A8C36F1EC34CD831DA770E24C5E32447CCF517D

DEFINES += kSigningCertCommonName=\\\"$${APPCERT}\\\"
