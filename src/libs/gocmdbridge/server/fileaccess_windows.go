package main

import (
	"encoding/hex"
	"fmt"
	"os"
	"unsafe"

	"golang.org/x/sys/windows"
)

func isWritable(path string) bool {
	return true
}

func isReadable(path string) bool {
	return true
}

func isExecutable(path string) bool {
	return true
}

func numberOfHardLinks(path string) int {
	return 0
}

// File ID for Windows up to version 7 and FAT32 drives
func fileIdFromHandle(handle windows.Handle) string {
	var info windows.ByHandleFileInformation
	if err := windows.GetFileInformationByHandle(handle, &info); err == nil {
		buffer := fmt.Sprintf("%lx:%08lx%08lx", info.VolumeSerialNumber, info.FileIndexHigh, info.FileIndexLow)
		return buffer
	}
	return ""
}

type FILE_ID_INFO struct {
	VolumeSerialNumber uint64
	FileId             [16]byte
}

// File ID for Windows starting from version 8.
func fileIdWin8(handle windows.Handle) string {
	infoEx := &FILE_ID_INFO{}

	if err := windows.GetFileInformationByHandleEx(
		handle,
		windows.FileIdInfo,
		(*byte)(unsafe.Pointer(infoEx)),
		uint32(unsafe.Sizeof(*infoEx))); err == nil {
		result := fmt.Sprintf("0x%x:%s", infoEx.VolumeSerialNumber, hex.EncodeToString(infoEx.FileId[:]))
		return result
	}

	// GetFileInformationByHandleEx() is observed to fail for FAT32, QTBUG-74759
	return fileIdFromHandle(handle)
}

func fileId(path string) string {
	f, err := os.Open(path)
	defer f.Close()

	if err != nil {
		return ""
	}
	return fileIdWin8(windows.Handle(f.Fd()));
}

func freeSpace(path string) uint64 {
	var freeBytes uint64
	windows.GetDiskFreeSpaceEx(windows.StringToUTF16Ptr(path), &freeBytes, nil, nil)
	return freeBytes
}
