package repository

import (
	"errors"
	"log"

	"github.com/boltdb/bolt"
)

var (
	ErrorBucketNotExists = errors.New("Bucket Not Exists")
)

type BoltDB struct {
	*bolt.DB
}

// Open or create a BoltDB database
func Open(name string) *BoltDB {
	db, err := bolt.Open(name, 0600, nil)
	if err != nil {
		log.Fatalln(err)
	}
	return &BoltDB{db}
}

// Find by key
func (db BoltDB) Find(key, bucket string) (string, error) {
	var value string
	err := db.View(func(tx *bolt.Tx) error {
		b := tx.Bucket([]byte(bucket))
		if b == nil {
			return ErrorBucketNotExists
		}

		val := b.Get([]byte(key))
		value = string(val)
		return nil
	})
	return value, err
}

// Delete a key from the bucket
func (db BoltDB) Delete(key, bucket string) error {
	err := db.Update(func(tx *bolt.Tx) error {
		b := tx.Bucket([]byte(bucket))
		if b == nil {
			return ErrorBucketNotExists
		}
		return b.Delete([]byte(key))
	})
	return err
}

// Delete a bucket
func (db BoltDB) DeleteBucket(bucket string) error {
	err := db.Update(func(tx *bolt.Tx) error {
		return tx.DeleteBucket([]byte(bucket))
	})
	return err
}

// Create or update a value
func (db *BoltDB) CreateOrUpdate(key, value, bucket string) error {
	return db.Update(func(tx *bolt.Tx) error {
		b, err := tx.CreateBucketIfNotExists([]byte(bucket))
		if err != nil {
			return err
		}

		err = b.Put([]byte(key), []byte(value))
		return err
	})
}

// Check if a key is available
func (db BoltDB) IsAvailable(key, bucket string) (bool, error) {
	available := false
	err := db.View(func(tx *bolt.Tx) error {
		b := tx.Bucket([]byte(bucket))
		if b == nil {
			return ErrorBucketNotExists
		}

		v := b.Get([]byte(key))
		if v == nil {
			available = true
		}
		return nil
	})

	return available, err
}
