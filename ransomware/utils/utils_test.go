package utils

import (
	"os/user"
	"regexp"
	"testing"
)

func TestGenerateANString(t *testing.T) {
	sizes := []int{8, 16, 32, 64}
	for _, size := range sizes {
		key, err := GenerateRandomANString(size)
		if err != nil {
			t.Error(err)
		}

		if len(key) != size {
			t.Errorf("Expect key with %d length, but got %d", size, len(key))
		}

		re := regexp.MustCompile("^[a-zA-Z0-9_]*$")
		if !re.MatchString(key) {
			t.Errorf("Expecting alphanumeric string, but got %s", key)
		}
	}
}

func TestStringInSlice(t *testing.T) {
	tests := []struct {
		slice  []string
		search string
		result bool
	}{
		{[]string{"Hello", "World"}, "World", true},
		{[]string{"The", "Quick", "Brown", "Fox"}, "Fox", true},
		{[]string{"Hi", "Hello"}, "Welcome", false},
	}
	for _, test := range tests {
		exists := StringInSlice(test.search, test.slice)
		if exists != test.result {
			t.Errorf("Expect %t when search for %s in %#v but got %t", test.result, test.search, test.slice, exists)
		}
	}
}

func TestGetCurrentUser(t *testing.T) {
	usr := GetCurrentUser()
	if usr == (&user.User{}) {
		t.Error("Expect a valid user instance, got empty")
	}
}
