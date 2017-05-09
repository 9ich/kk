// Extracts times from an XML glintercept log, ready for plotting.
package main

import (
	"encoding/xml"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"text/tabwriter"
)

type glintercept struct {
	Functions []function `xml:"FUNCTIONS>FUNCTION"`
}

type function struct {
	Name string `xml:"NAME"`
	Params []value `xml:"PARAM>VALUE"`
	Functime functime `xml:"FUNCTIME"`
}

type param struct {
	//Name string `xml:"name,attr"`
	//Data string `xml:"data,attr"`
	Value string `xml:"VALUE"`
}

type value struct {
	V string `xml:"data,attr"`
}

type functime struct {
	T int `xml:"data,attr"`
}

func main() {
	b, _ := ioutil.ReadAll(os.Stdin)
	gli := new(glintercept)
	err := xml.Unmarshal(b, gli)
	if err != nil {
		log.Fatalln(err)
	}
	w := new(tabwriter.Writer)
	// Format in tab-separated columns with a tab stop of 8.
	w.Init(os.Stdout, 0, 8, 0, '\t', 0)

	fmt.Fprintf(w, "Func, Time\n")
	for _, f := range gli.Functions {
		if f.Functime.T < 5000 {
			continue
		}
		fmt.Fprintf(w, "%s\t(%v)\t%f millisec\n", f.Name, f.Params, float32(f.Functime.T)/1000.0)
	}

	w.Flush()
}
