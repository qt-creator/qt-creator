package main

type ChannelWriter struct {
	Channel chan<- []byte
	messages [][]byte
	byteSize int
}

func NewChannelWriter(channel chan<- []byte) *ChannelWriter {
	return &ChannelWriter{
		Channel: channel,
		byteSize: 0,
	}
}

func (self *ChannelWriter) Flush() () {
	for _, message := range self.messages {
		self.Channel <- message
	}
	self.messages = nil
	self.byteSize = 0
}

func (self *ChannelWriter) Write(p []byte) () {
	self.messages = append(self.messages, p)
	self.byteSize += len(p)
	if self.byteSize > 1024 {
		self.Flush()
	}
}
