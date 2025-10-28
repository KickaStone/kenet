// main.go
package main

import (
	"fmt"
	"log"
	"time"

	"github.com/panjf2000/gnet/v2"
)

type MyEventHandler struct{}

func (h *MyEventHandler) OnBoot(eng gnet.Engine) (action gnet.Action) {
	return gnet.None
}

func (h *MyEventHandler) OnShutdown(eng gnet.Engine) {
	// fmt.Printf("OnShutdown: %s\n", eng.Address())
}

func (h *MyEventHandler) OnOpen(c gnet.Conn) (out []byte, action gnet.Action) {
	fmt.Printf("OnOpen: %s\n", c.RemoteAddr().String())
	return nil, gnet.None
}

func (h *MyEventHandler) OnClose(c gnet.Conn, err error) (action gnet.Action) {
	fmt.Printf("OnClose: %s\n", c.RemoteAddr().String())
	return gnet.None
}

func (h *MyEventHandler) OnTraffic(c gnet.Conn) (action gnet.Action) {
	fmt.Printf("OnTraffic: %s\n", c.RemoteAddr().String())

	// echo
	buf := make([]byte, 1024)
	n, err := c.Read(buf)
	if err != nil {
		fmt.Printf("OnTraffic: read error: %v\n", err)
		return gnet.None
	}
	_, err = c.Write(buf[:n])
	if err != nil {
		fmt.Printf("OnTraffic: write error: %v\n", err)
		return gnet.None
	}
	return gnet.None
}

func (h *MyEventHandler) OnTick() (delay time.Duration, action gnet.Action) {
	fmt.Printf("OnTick: %s\n", time.Now().String())
	return 0, gnet.None
}

func main() {
	eventHandler := &MyEventHandler{}
	err := gnet.Run(eventHandler, "tcp://:9000", gnet.WithMulticore(true), gnet.WithReusePort(true))
	if err != nil {
		log.Fatalf("gnet serve error: %v", err)
	}
}
