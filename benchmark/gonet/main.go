package main

import (
	"io"
	"log"
	"net"
)

func echoHandler(conn net.Conn) {
	for {
		buf := make([]byte, 1024)
		n, err := conn.Read(buf)
		if err != nil {
			// if the connection is closed, return
			if err == io.EOF {
				return
			}
			log.Printf("read error: %v", err)
			return
		}
		conn.Write(buf[:n])
	}
}

func main() {
	ln, err := net.Listen("tcp", ":9000")
	log.Println("listening on :9000")
	if err != nil {
		log.Fatalf("listen error: %v", err)
	}
	defer ln.Close()

	for {
		conn, err := ln.Accept()
		if err != nil {
			log.Fatalf("accept error: %v", err)
		}
		go echoHandler(conn)
	}
}
