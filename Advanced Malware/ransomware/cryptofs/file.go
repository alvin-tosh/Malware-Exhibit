// Package cryptofs provides a basic abstraction for file encryption/decryption
package cryptofs

import (
	"crypto/aes"
	"crypto/cipher"
	"crypto/rand"
	"io"
	"os"
)

// Type File wrap an os.FileInfo
type File struct {
	os.FileInfo
	Extension string // The file extension without dot
	Path      string // The absolute path of the file
}

// Encrypt the file content with AES-CTR with the given key
// sending then to dst
func (file *File) Encrypt(enckey string, dst io.Writer) error {
	// Open the file read only
	inFile, err := os.Open(file.Path)
	if err != nil {
		return err
	}
	defer inFile.Close()

	// Create a 128 bits cipher.Block for AES-256
	block, err := aes.NewCipher([]byte(enckey))
	if err != nil {
		return err
	}

	// The IV needs to be unique, but not secure
	iv := make([]byte, aes.BlockSize)
	if _, err = io.ReadFull(rand.Reader, iv); err != nil {
		return err
	}

	// Get a stream for encrypt/decrypt in counter mode (best performance I guess)
	stream := cipher.NewCTR(block, iv)

	// Write the Initialization Vector (iv) as the first block
	// of the dst writer
	dst.Write(iv)

	// Open a stream to encrypt and write to dst
	writer := &cipher.StreamWriter{S: stream, W: dst}

	// Copy the input file to the dst writer, encrypting as we go.
	if _, err = io.Copy(writer, inFile); err != nil {
		return err
	}

	return nil
}

// Decrypt the file content with AES-CTR with the given key
// sending then to dst
func (file *File) Decrypt(key string, dst io.Writer) error {
	// Open the encrypted file
	inFile, err := os.Open(file.Path)
	if err != nil {
		return err
	}
	defer inFile.Close()

	// Create a 128 bits cipher.Block for AES-256
	block, err := aes.NewCipher([]byte(key))
	if err != nil {
		return err
	}

	// Retrieve the iv from the encrypted file
	iv := make([]byte, aes.BlockSize)
	inFile.Read(iv)

	// Get a stream for encrypt/decrypt in counter mode (best performance I guess)
	stream := cipher.NewCTR(block, iv)

	// Open a stream to decrypt and write to dst
	reader := &cipher.StreamReader{S: stream, R: inFile}

	// Copy the input file to the dst, decrypting as we go.
	if _, err = io.Copy(dst, reader); err != nil {
		return err
	}

	return nil
}

func (f *File) ReplaceBy(filename string) error {
	// Open the file
	file, err := os.OpenFile(f.Path, os.O_WRONLY|os.O_TRUNC, 0600)
	if err != nil {
		return err
	}
	defer file.Close()

	src, err := os.Open(filename)
	if err != nil {
		return err
	}
	defer src.Close()

	// Copy the reader to file
	if _, err = io.Copy(file, src); err != nil {
		return err
	}

	return nil
}
