package cmd

import (
	"os"

	"github.com/fatih/color"
)

var (
	banner = `
            __________
            \______   \_____    ____   __________   ______  _  _______ _______   ____
             |       _/\__  \  /    \ /  ___/  _ \ /    \ \/ \/ /\__  \\_  __ \_/ __ \
             |    |   \ / __ \|   |  \\___ (  <_> )   |  \     /  / __ \|  | \/\  ___/
             |____|_  /(____  /___|  /____  >____/|___|  /\/\_/  (____  /__|    \___  >
                    \/      \/     \/     \/           \/             \/            \/
    `
	delimiter = `
    ===============================================================================================
    `
)

// Print banner
func PrintBanner() {
	color.New(color.FgCyan).Add(color.Bold).Println(delimiter)
	color.New(color.FgRed).Add(color.Bold).Println(banner)
	color.New(color.FgCyan).Add(color.Bold).Println(delimiter)
}

// Print script usage
func Usage(errmsg string) {
	color.New(color.FgWhite).Add(color.Bold).Printf(
		"%s\n\n"+
			"usage: %s\n"+
			"       %s decrypt key\n"+
			"      Will try to decrypt your files using the given key\n",
		errmsg, os.Args[0], os.Args[0])
	os.Exit(2)
}
