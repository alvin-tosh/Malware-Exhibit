package utils

import (
	"crypto/rand"
	"encoding/hex"
	"io"
	"os"
	"os/user"
	"strings"
)

// Generate a random alphanumeric string with the given size
func GenerateRandomANString(size int) (string, error) {
	key := make([]byte, size)
	_, err := rand.Read(key)
	if err != nil {
		return "", err
	}

	return hex.EncodeToString(key)[:size], nil
}

// Check if a value exists on slice
func StringInSlice(search string, slice []string) bool {
	for _, v := range slice {
		if v == search {
			return true
		}
	}
	return false
}

// SliceContainsSubstring check if a substring exists on a slice item
func SliceContainsSubstring(search string, slice []string) bool {
	for _, v := range slice {
		if strings.Contains(search, v) {
			return true
		}
	}
	return false
}

// Return a list containing the letters used by the current drives
func GetDrives() (letters []string) {
	for _, drive := range "ABCDEFGHIJKLMNOPQRSTUVWXYZ" {
		_, err := os.Open(string(drive) + ":\\")
		if err == nil {
			letters = append(letters, string(drive)+":\\")
		}
	}
	return
}

// Return an *os.User instance representing the current user
func GetCurrentUser() *user.User {
	usr, err := user.Current()
	if err != nil {
		return &user.User{}
	}
	return usr
}

// RenameFile move the file from src to the file on dst
// This is a workaround to os.Rename across drives on windows
func RenameFile(src, dst string) error {
	srcFile, err := os.Open(src)
	if err != nil {
		return err
	}
	defer srcFile.Close()

	dstfile, err := os.Create(dst)
	if err != nil {
		return err
	}
	defer dstfile.Close()

	_, err = io.Copy(dstfile, srcFile)
	if err != nil {
		return err
	}

	srcFile.Close()

	if err = os.Remove(src); err != nil {
		return err
	}

	return nil
}

// FileExists returns whether the given file or directory exists or not
func FileExists(path string) bool {
	_, err := os.Stat(path)
	if err == nil {
		return true
	}

	return false
}
