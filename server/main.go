package main

import (
	"fmt"
	"io"
	"net/http"
)

func main() {

	http.HandleFunc(
		"GET /hello",
		func(w http.ResponseWriter, r *http.Request) {
			fmt.Println(r)
			fmt.Println("Content Length is:", r.ContentLength)
			fmt.Println("Request body:", r.Body)
			io.WriteString(w, "Hello, from the server!\n")
		},
	)

	// ln, err := net.Listen("tcp", ":8080")
	// if err != nil {
	// 	panic(err)
	// }
	//
	// for {
	// 	conn, err := ln.Accept()
	// 	if err != nil {
	// 		panic(err)
	// 	}
	// 	b, err := io.ReadAll(conn)
	// 	if err != nil {
	// 		panic(err)
	// 	}
	// 	fmt.Println("Received:", string(b))
	// }
	http.ListenAndServe("127.0.0.1:8080", nil)
}
