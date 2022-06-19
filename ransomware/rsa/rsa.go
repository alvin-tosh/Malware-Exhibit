package rsa

import (
	"crypto/rand"
	"crypto/rsa"
	"crypto/sha256"
	"crypto/x509"
	"encoding/pem"
	"fmt"
)

// Encrypt content with a public key
func Encrypt(publicKey []byte, text []byte) ([]byte, error) {
	block, _ := pem.Decode(publicKey)
	pubkeyInterface, err := x509.ParsePKIXPublicKey(block.Bytes)
	pubkey, _ := pubkeyInterface.(*rsa.PublicKey)
	var out []byte

	out, err = rsa.EncryptOAEP(sha256.New(), rand.Reader, pubkey, text, []byte(""))
	if err != nil {
		return []byte{}, err
	}

	return out, nil
}

// Decrypt content using a Private Key
func Decrypt(privateKey []byte, ciphertext []byte) ([]byte, error) {
	// Extract the PEM-encoded data block
	block, _ := pem.Decode(privateKey)
	if block == nil {
		return []byte{}, fmt.Errorf("bad key data: %s", "not PEM-encoded")
	}
	if block.Headers["Proc-Type"] == "4,ENCRYPTED" {
		return []byte{}, fmt.Errorf(
			"Failed to read private key: password protected keys are\n" +
				"not supported. Please decrypt the key prior to use.")
	}

	if got, want := block.Type, "RSA PRIVATE KEY"; got != want {
		return []byte{}, fmt.Errorf("Unknown key type %q, want %q", got, want)
	}

	// Decode the RSA private key
	priv, err := x509.ParsePKCS1PrivateKey(block.Bytes)
	if err != nil {
		return []byte{}, fmt.Errorf("Bad private key: %s", err)
	}

	var out []byte

	// Decrypt the data
	out, err = rsa.DecryptOAEP(sha256.New(), rand.Reader, priv, ciphertext, []byte(""))
	if err != nil {
		return []byte{}, err
	}

	return out, nil
}
