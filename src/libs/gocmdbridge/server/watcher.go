package main

import (
	"strings"

	"github.com/fsnotify/fsnotify"
	"github.com/fxamacker/cbor/v2"
)

type WatcherHandler struct {
	watcher   *fsnotify.Watcher
	watchList map[int]string
	watchRefs map[string]int
}

func NewWatcherHandler() *WatcherHandler {
	watcher, err := fsnotify.NewWatcher()
	if err != nil {
		panic(err)
	}
	return &WatcherHandler{
		watcher:   watcher,
		watchList: make(map[int]string),
		watchRefs: make(map[string]int),
	}
}

type watchevent struct {
	Type      string
	Id        int
	Path      string
	EventType int
}

func (watchHandler WatcherHandler) start(out chan<- []byte) {
	for {
		select {
		case event, ok := <-watchHandler.watcher.Events:
			if !ok {
				return
			}
			// Find which watchList entries correspond to the event
			for id, path := range watchHandler.watchList {
				// See if the event path is a subpath of the watch path
				if strings.HasPrefix(event.Name, path) {
					data, _ := cbor.Marshal(watchevent{
						Type:      "watchEvent",
						Id:        id,
						Path:      event.Name,
						EventType: int(event.Op),
					})
					out <- data
				}
			}
		case err, ok := <-watchHandler.watcher.Errors:
			if !ok {
				return
			}
			panic(err)
		}
	}
}

type addwatchresult struct {
	Type   string
	Id     int
	Result bool
}

func (watchHandler WatcherHandler) processAdd(cmd command, out chan<- []byte) {
	// TODO: Resolve links
	err := watchHandler.watcher.Add(cmd.Path)
	if err != nil {
		sendError(out, cmd, err)
		return
	}
	watchHandler.watchList[cmd.Id] = cmd.Path
	watchHandler.watchRefs[cmd.Path]++
	data, _ := cbor.Marshal(addwatchresult{
		Type:   "addwatchresult",
		Id:     cmd.Id,
		Result: true,
	})

	out <- data
}

func (watchHandler WatcherHandler) processStop(cmd command, out chan<- []byte) {
	path, ok := watchHandler.watchList[cmd.Id]
	if !ok {
		sendError(out, cmd, &watchnotfounderror{})
		return
	}

	if watchHandler.watchRefs[path] <= 0 {
		sendError(out, cmd, &watchnotfounderror{})
	}

	watchHandler.watchRefs[path]--
	if watchHandler.watchRefs[path] == 0 {
		err := watchHandler.watcher.Remove(path)
		if err != nil {
			sendError(out, cmd, err)
			return
		}
		delete(watchHandler.watchRefs, path)
	}

	delete(watchHandler.watchList, cmd.Id)
	data, _ := cbor.Marshal(removewatchresult{
		Type:   "removewatchresult",
		Id:     cmd.Id,
		Result: true,
	})
	out <- data
}

type removewatchresult struct {
	Type   string
	Id     int
	Result bool
}

type watchnotfounderror struct {
}

func (e *watchnotfounderror) Error() string {
	return "Watch not found"
}

func (watchHandler WatcherHandler) processRemove(cmd command, out chan<- []byte) {
	if _, ok := watchHandler.watchList[cmd.Id]; !ok {
		sendError(out, cmd, &watchnotfounderror{})
		return
	}
	watchHandler.watchRefs[cmd.Path]--
	if watchHandler.watchRefs[cmd.Path] == 0 {
		err := watchHandler.watcher.Remove(cmd.Path)
		if err != nil {
			sendError(out, cmd, err)
			return
		}
	}
	delete(watchHandler.watchList, cmd.Id)

	data, _ := cbor.Marshal(removewatchresult{
		Type:   "removewatchresult",
		Id:     cmd.Id,
		Result: true,
	})
	out <- data
}
