// Generates a charmap for cgame & ui from an XML .fnt file
// exported by BMFont.  Font must cover ' ' through '~'.
//
// http://www.angelcode.com/products/bmfont/
package main

import (
	"encoding/xml"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"sort"
	"text/tabwriter"
)

type info struct {
	Face    string `xml:"face,attr"`
	Size    int    `xml:"size,attr"`
	Spacing string `xml:"spacing,attr"`
}

type common struct {
	ScaleW int `xml:"scaleW,attr"`
	ScaleH int `xml:"scaleH,attr"`
}

type page struct {
	File string `xml:"file,attr"`
}

type char struct {
	ID       int `xml:"id,attr"`
	X        int `xml:"x,attr"`
	Y        int `xml:"y,attr"`
	Width    int `xml:"width,attr"`
	Xoffset  int `xml:"xoffset,attr"`
	Yoffset  int `xml:"yoffset,attr"`
	Xadvance int `xml:"xadvance,attr"`
}

type kerning struct {
	First  int `xml:"first,attr"`
	Second int `xml:"second,attr"`
	Amount int `xml:"amount,attr"`
}

type kerningsort []kerning

func (k kerningsort) Len() int      { return len(k) }
func (k kerningsort) Swap(i, j int) { k[i], k[j] = k[j], k[i] }
func (k kerningsort) Less(i, j int) bool {
	if k[i].First == k[j].First {
		return k[i].Second < k[j].Second
	}
	return k[i].First < k[j].First
}

type font struct {
	Info     info      `xml:"info"`
	Common   common    `xml:"common"`
	Page     page      `xml:"pages>page"`
	Chars    []char    `xml:"chars>char"`
	Kernings []kerning `xml:"kernings>kerning"`
}

var pathprefix string

func init() {
	flag.StringVar(&pathprefix, "p", "", "image filename prefix")
	flag.Parse()
}

func main() {
	b, _ := ioutil.ReadAll(os.Stdin)
	font := new(font)
	err := xml.Unmarshal(b, font)
	if err != nil {
		log.Fatalln(err)
	}
	w := new(tabwriter.Writer)
	// Format in tab-separated columns with a tab stop of 8.
	w.Init(os.Stdout, 0, 8, 0, '\t', 0)

	fmt.Fprintf(w, "// %s\nfont\t%s%s \t%d\t%d\t%d\n\n", font.Info.Face,
		pathprefix, font.Page.File, font.Info.Size, font.Common.ScaleW,
		font.Common.ScaleH)
	fmt.Fprintln(w, "//\tx\ty\twidth\txofs\tyofs\txadv")

	// Skip control chars.
	for c := 0; c < int(' '); c++ {
		fmt.Fprintf(w, "char\t0\t0\t-1\t0\t0\t0\t// \\x%02x\n", c)
	}
	for _, c := range font.Chars {
		fmt.Fprintf(w, "char\t%d\t%d\t%d\t%d\t%d\t%d\t// '%c'\n", c.X, c.Y,
			c.Width, c.Xoffset, c.Yoffset, c.Xadvance, c.ID)
	}
	fmt.Fprintln(w, "char\t0\t0\t-1\t0\t0\t0\t// DEL\n")

	if len(font.Kernings) > 0 {
		sort.Sort(kerningsort(font.Kernings))
		fmt.Fprintln(w, "//\tfirst\tsecond\tamount")
		for _, k := range font.Kernings {
			fmt.Fprintf(w, "kerning\t%d\t%d\t%d\t// '%c%c'\n", k.First,
				k.Second, k.Amount, k.First, k.Second)
		}
		fmt.Fprintln(w, "")
	}
	w.Flush()
}
