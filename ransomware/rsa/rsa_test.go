package rsa

import (
	"strconv"
	"testing"
)

var (
	PRIV_KEY = []byte(`
-----BEGIN RSA PRIVATE KEY-----
MIIEpQIBAAKCAQEAxa7DJ2kw+JOICpeJ7lDMksQ1ZIVxb4npjskM6TyP5pAgN5Cd
x7WcCVa0bHoVgnRI7WTZmVBLA9QssqAJHav6kroo/0X7dibhghG1mUujgPxq//j4
pPvq0qIMBeQbOB3sryLB/1ScgQzByJBuEBgmFHWfLq7LJUyKmZd1uzKAEtJsrY3f
7WhcWfiqMemMeJl7zvMvx79bqNe9wEs5AGb5ZCM2fQxjCLBmMzK4gu1lZXh8wFl3
3fcDKFRGJ7gVvJSagvIe/KHA4Vydwl/Ofo23nD8qEkppqLCuNUmpRRErI136Ku0m
D8DSQncAKd2AAblPU0qrK2mVk48WVPqEQgHLmwIDAQABAoIBAESTptGyNTHWhDgg
b9IDp6Q2ess/W6W08xf8eQUkZEs4rmR4P7P7NYEr2fVTuMiDrJTmCnKcHxnZ7hOd
XuwzL/7co8JCtcQ8IrjridL/IV5qsnfQF/msBr9BReh5RFQIVYZACYqZAJ3oHgWE
zRm6NuOcFjesnX53+hDkMQxarYFZb4k7Ghxgi35a3GUe1DNcJxUYjRviW73gVg15
wT/6kfNWOStn8NVTw1MU4gklsH6JOl2IEmwcCOw+wfOXnwvN0QHewXnZYvfOKtK9
wgTC4RPIjA/6M662yqGFiY8xhEBwf5dPF+9/+N42vJTSLkta5DL+hGFGA6q4Zeih
xyMTCMkCgYEA9HbIoZ41dc+aiunEb7sCQXU2iFiubJlDn3q5L4AufezhHH2md3Co
mN1JUqcMXj84H1/LmFM+VtyEzu5qwrsZEWhyqf6mKEbDHjjXyPbjFT8sCsvlQdCk
02aE52x9ZDfEvyFfrxAEGSW7LLEFx65RaOYdtOzIJ8uKf4rnYbUt0SUCgYEAzwLX
t61nwdsiBKm5HMLPvN1ISgO6mzcIibfrb0jYWuFFUPZ00JbGHTpvPwiS4Advennl
Wp/xEEiGqp9hXDNYGhfPMphoBblcYdV8ZH4NL2DlaFj7kqXJbVUwzDJ3Ype5bzJU
SG51oixVXz/hT/jFSSVLPHzq22nQiZSxnPvObb8CgYEApZrCFxoBxSk529i4haf9
wzIQGxVYM6Evuh18zbzbwdpyNMa5ujfLPqLJRQB81GunLTnLxgi+NkF0hmnkUL5G
IRDMfHRRQv+MtjBznWQCOSZuQ3IUgB1DSyIr7koEN5u/4GpPU1xaKl7xCTlyXO6t
n44jmai9fpfX3sbOL9Z4jzECgYEAnBYxny4hyNq4yLlMeXIufuJ+qkgrgPM6/dRu
sedEMyoeQNDD/a9hzBIOZYHKdR9GIBwfInjso/F7kNVB7OpN6MbBFQ4ziPVdwerd
s0wUFwBBma9WaRmWSljsxVrcB7wNNtnFESQwkEpLSNl6wvj5kJCNLRunXi9n7QTv
80UuPjMCgYEA8Z7U/aLEjfsK60RfpCpFJJfSUreK15fE+jJoZ2qWaEezqT3USneA
KUFCqqGfWUlOyYYTeNj+ufyEqqT7e1UD7qD8I+/6igoWRWnnK6R6PaMDMOrBoNRr
msVqvZigM4ZkphoeEsj+pI9cK1CEWW6EWZz2vgKYzEqREaI05UnU6Dg=
-----END RSA PRIVATE KEY-----`)
	PUB_KEY = []byte(`
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAxa7DJ2kw+JOICpeJ7lDM
ksQ1ZIVxb4npjskM6TyP5pAgN5Cdx7WcCVa0bHoVgnRI7WTZmVBLA9QssqAJHav6
kroo/0X7dibhghG1mUujgPxq//j4pPvq0qIMBeQbOB3sryLB/1ScgQzByJBuEBgm
FHWfLq7LJUyKmZd1uzKAEtJsrY3f7WhcWfiqMemMeJl7zvMvx79bqNe9wEs5AGb5
ZCM2fQxjCLBmMzK4gu1lZXh8wFl33fcDKFRGJ7gVvJSagvIe/KHA4Vydwl/Ofo23
nD8qEkppqLCuNUmpRRErI136Ku0mD8DSQncAKd2AAblPU0qrK2mVk48WVPqEQgHL
mwIDAQAB
-----END PUBLIC KEY-----`)

	Phrases = []string{"Hello World!", "The Quick Brown Fox Jumps Over The Lazy Dog", "Testing RSA Encryption"}
)

func TestEncryptDecryptRSA(t *testing.T) {
	for _, phrase := range Phrases {
		ciphertext, err := Encrypt(PUB_KEY, []byte(phrase))
		if err != nil {
			t.Error(err)
		}
		decryptedText, err := Decrypt(PRIV_KEY, ciphertext)
		if err != nil {
			t.Error(err)
		}

		if string(decryptedText) != phrase {
			t.Errorf("Expected %s but got %s", phrase, string(decryptedText))
		}
	}
}

func BenchmarkEncryptDecryptRSA(b *testing.B) {
	for n := 0; n < b.N; n++ {
		ciphertext, _ := Encrypt(PUB_KEY, []byte("bench"+strconv.Itoa(n)))
		Decrypt(PRIV_KEY, ciphertext)
	}
}
