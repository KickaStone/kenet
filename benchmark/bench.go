package main

import (
	"fmt"
	"net"
	"time"
)

func main() {
	conn, err := net.Dial("tcp", "127.0.0.1:9000")
	if err != nil {
		fmt.Println("Error connecting to server:", err)
		return
	}
	defer conn.Close()

	// single thread
	start := time.Now()

	for i := 0; i < 1000; i++ {
		msg := make([]byte, 1024)
		startTime := time.Now()
		_, err = conn.Write(msg)
		if err != nil {
			fmt.Println("Error writing to server:", err)
			return
		}

		// read response
		buf := make([]byte, 1024)
		_, err = conn.Read(buf)
		if err != nil {
			fmt.Println("Error reading from server:", err)
			return
		}
		elapsed := time.Since(startTime)
		fmt.Println("Request time:", elapsed)
	}
	elapsed := time.Since(start)
	fmt.Println("Single thread time:", elapsed)
	fmt.Println("QPS:", float64(1000)/elapsed.Seconds())
}
