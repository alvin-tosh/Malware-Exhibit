package repository

import (
	"fmt"
	"os"
	"testing"
)

func TestRepositoryOpen(t *testing.T) {
	db := Open("test.db")
	defer db.Close()
	defer os.Remove("test.db")

	if fmt.Sprintf("%T", db) != "*repository.BoltDB" {
		t.Errorf("Expect *repository.BoltDB but got %T\n", db)
	}
}

func TestRepositoryCreateOrUpdate(t *testing.T) {
	db := Open("test.db")
	defer db.Close()
	defer os.Remove("test.db")

	err := db.CreateOrUpdate("test", "Hello World", "testBucket")
	handleErr(err, t)
}

func TestRepositoryFind(t *testing.T) {
	db := Open("test.db")
	defer db.Close()
	defer os.Remove("test.db")

	err := db.CreateOrUpdate("test", "Hello World", "testBucket")
	handleErr(err, t)

	value, err := db.Find("test", "testBucket")
	handleErr(err, t)

	if value != "Hello World" {
		t.Errorf("Expect Hello World but got %s\n", value)
	}
}

func TestRepositoryFindOnNotExistentBucket(t *testing.T) {
	db := Open("test.db")
	defer db.Close()
	defer os.Remove("test.db")

	err := db.CreateOrUpdate("test", "Hello World", "testBucket")
	handleErr(err, t)

	_, err = db.Find("test", "notExistentBucket")

	if err != ErrorBucketNotExists {
		t.Fatalf("Expect ErrorNotExists to be thrown, got %v", err)
	}
}

func TestRepositoryIsAvailable(t *testing.T) {
	db := Open("test.db")
	defer db.Close()
	defer os.Remove("test.db")

	err := db.CreateOrUpdate("test", "Hello World", "testBucket")
	handleErr(err, t)

	available, err := db.IsAvailable("test", "testBucket")
	handleErr(err, t)
	if available != false {
		t.Fatalf("Expect test to be unavailable, got available")
	}

	available, err = db.IsAvailable("test1", "testBucket")
	handleErr(err, t)
	if available != true {
		t.Fatalf("Expect test1 to be available, got unavailable")
	}
}

func TestRepositoryIsAvailableOnNotExistentBucket(t *testing.T) {
	db := Open("test.db")
	defer db.Close()
	defer os.Remove("test.db")

	err := db.CreateOrUpdate("test", "Hello World", "testBucket")
	handleErr(err, t)

	_, err = db.IsAvailable("test", "notExistentBucket")
	if err != ErrorBucketNotExists {
		t.Fatalf("Expect ErrorNotExists to be thrown, got %v", err)
	}
}

func TestRepositoryDelete(t *testing.T) {
	db := Open("test.db")
	defer db.Close()
	defer os.Remove("test.db")

	err := db.CreateOrUpdate("test", "Hello World", "testBucket")
	handleErr(err, t)

	err = db.Delete("test", "testBucket")
	handleErr(err, t)

	available, err := db.IsAvailable("test", "testBucket")
	handleErr(err, t)
	if available != true {
		t.Fatalf("Expect the key test is available on testBucket, got unavailable")
	}
}

func TestRepositoryDeleteBucket(t *testing.T) {
	db := Open("test.db")
	defer db.Close()
	defer os.Remove("test.db")

	err := db.CreateOrUpdate("test", "Hello World", "testBucket")
	handleErr(err, t)

	err = db.DeleteBucket("testBucket")
	handleErr(err, t)

	_, err = db.IsAvailable("test", "testBucket")
	if err != ErrorBucketNotExists {
		t.Fatalf("Expect ErrorNotExists to be thrown, got %v", err)
	}
}

func handleErr(err error, t *testing.T) {
	if err != nil {
		t.Fatalf("An error ocurred: %s", err)
	}
}
