// Generates a propMap array for cgame & ui from an XML .fnt file
// exported by BMFont.  Font must cover ' ' through '~'.
//
// http://www.angelcode.com/products/bmfont/
package main

import (
	"encoding/xml"
	"fmt"
	"io/ioutil"
	"log"
	"os"
)

type char struct {
	ID    int `xml:"id,attr"`
	X     int `xml:"x,attr"`
	Y     int `xml:"y,attr"`
	Width int `xml:"width,attr"`
}

type font struct {
	Chars []char `xml:"chars>char"`
}

// control characters get skipped
var preamble string = `
static int propMap[128][3] = {
	{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},
	{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},

	{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},
	{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},
`

func main() {
	b, _ := ioutil.ReadAll(os.Stdin)
	font := new(font)
	err := xml.Unmarshal(b, font)
	if err != nil {
		log.Fatalln(err)
	}
	fmt.Println(preamble)
	for _, c := range font.Chars {
		fmt.Printf("\t{%d, %d, %d},\t// '%c'\n", c.X, c.Y, c.Width, c.ID)
	}
	fmt.Println("\t{0, 0, -1}\t// DEL\n};")
}
