package web

import (
	"net/http"
	"strings"

	"github.com/labstack/echo"
)

var (
	ApiResponseForbidden        = SimpleResponse{Status: http.StatusForbidden, Message: "Seems like you are not welcome here... Bye"}
	ApiResponseBadJson          = SimpleResponse{Status: http.StatusBadRequest, Message: "Expect valid json payload"}
	ApiResponseInternalError    = SimpleResponse{Status: http.StatusInternalServerError, Message: "Internal Server Error"}
	ApiResponseDuplicatedId     = SimpleResponse{Status: http.StatusConflict, Message: "Duplicated Id"}
	ApiResponseBadRSAEncryption = SimpleResponse{Status: http.StatusUnprocessableEntity, Message: "Error validating payload, bad public key"}
	ApiResponseNoPayload        = SimpleResponse{Status: http.StatusUnprocessableEntity, Message: "No payload"}
	ApiResponseBadRequest       = SimpleResponse{Status: http.StatusBadRequest, Message: "Bad Request"}
	ApiResponseResourceNotFound = SimpleResponse{Status: http.StatusTeapot, Message: "Resource Not Found"}
	ApiResponseNotFound         = SimpleResponse{Status: http.StatusNotFound, Message: "Not Found"}
)

type SimpleResponse struct {
	Status  int
	Message string
}

func CustomHTTPErrorHandler(err error, c echo.Context) {
	httpError, ok := err.(*echo.HTTPError)
	if ok {
		// If is an API call return a JSON response
		path := c.Request().URL().Path()
		if !strings.HasSuffix(path, "/") {
			path += "/"
		}

		if strings.Contains(path, "/api/") {
			c.JSON(httpError.Code, SimpleResponse{Status: httpError.Code, Message: httpError.Message})
			return
		}

		// Otherwise return the normal response
		c.String(httpError.Code, httpError.Message)
	}
}
