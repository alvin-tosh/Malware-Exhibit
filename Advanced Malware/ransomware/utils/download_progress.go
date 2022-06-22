package utils

import (
	"fmt"
	"io"
	"os"
)

// DownloadProgressReader wraps an existing io.Reader and show the copy progress
//
// It simply forwards the Read() call, while displaying
// the results from individual calls to it.
type DownloadProgressReader struct {
	io.Reader
	Total    int64 // Total # of bytes transferred
	Lenght   int64 // Expected length
	Progress float64
}

// Read 'overrides' the underlying io.Reader's Read method.
// This is the one that will be called by io.Copy(). We simply
// use it to keep track of byte counts and then forward the call.
func (pt *DownloadProgressReader) Read(p []byte) (int, error) {
	n, err := pt.Reader.Read(p)
	if n > 0 {
		pt.Total += int64(n)
		percentage := float64(pt.Total) / float64(pt.Lenght) * float64(100)
		if percentage-pt.Progress > 2 {
			pt.Progress = percentage
			fmt.Fprintf(os.Stderr, "\r%.2f%%", pt.Progress)
			if pt.Progress > 98.0 {
				pt.Progress = 100
				fmt.Fprintf(os.Stderr, "\r%.2f%%\n", pt.Progress)
			}
		}
	}

	return n, err
}
