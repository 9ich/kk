/*
 usage: go run mkpak.go [dir] [pakname]
*/

package main

import (
	"archive/zip"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"path/filepath"
	"strings"
)

const ext = ".pk3"

var whitelist = []string{
	"*.cfg",
	"*.qvm",
	"*.font",
	"*.tga",
	"*.png",
	"*.jpg",
	"*.md3",
	"*.bsp",
	"*.wav",
	"*.ogg",
	"*.shader",
	"*.mtr",
	"shaderlist.txt",
	"*.c",     // botfiles
	"*.h",     // botfiles
	"*.arena", // botfiles
}

var blacklist = []string{
	"*/*/*_ao.*", // Ambient occlusion (already blended into diffuse).
	"*/*/*_d.*",  // Displacement map (already copied into _n alpha).
	"*/*/*/*_ao.*",
	"*/*/*/*_d.*",
	"*/*test*",
	"*/*/*test*",
	"*!", // Test dirs.
	"*/*!/*",
	"*/*/*!/*",
}

var (
	root    string   // Directory to be packed.
	pakname string   // e.g. "pak0".
	paths   []string // Filtered files to include in the pak.
)

func readable(n int64) string {
	u := "B"
	f := float64(n)
	if f > 1024 {
		u = "KiB"
		f /= 1024
	}
	if f > 1024 {
		u = "MiB"
		f /= 1024
	}
	return fmt.Sprintf("%.2f %s", f, u)
}

func writepak(fname string, paths []string, pakpaths []string) (int64, int64, error) {
	f, err := os.Create(fname)
	if err != nil {
		return 0, 0, err
	}
	defer f.Close()
	w := zip.NewWriter(f)
	var nwrite int64
	for i := 0; i < len(paths); i++ {
		log.Println("add", pakpaths[i])
		f, err := w.Create(pakpaths[i])
		if err != nil {
			return 0, 0, err
		}

		b, err := ioutil.ReadFile(paths[i])
		if err != nil {
			return 0, 0, err
		}

		n, err := f.Write(b)
		if err != nil {
			return 0, 0, err
		}
		nwrite += int64(n)
	}
	err = w.Close()
	if err != nil {
		return 0, 0, err
	}

	stat, err := f.Stat()
	if err != nil {
		return 0, 0, err
	}
	return nwrite, stat.Size(), nil
}

func visit(path string, f os.FileInfo, err error) error {
	if f.IsDir() {
		return nil
	}

	pakpath := strings.TrimPrefix(path, root)
	pakpath = strings.Replace(pakpath, "\\", "/", -1)

	ok := false
	for i := 0; i < len(whitelist); i++ {
		match, _ := filepath.Match(whitelist[i], filepath.Base(path))
		if match {
			ok = true
			break
		}
		match, _ = filepath.Match(whitelist[i], pakpath)
		if match {
			ok = true
			break
		}
	}
	for i := 0; i < len(blacklist); i++ {
		match, _ := filepath.Match(blacklist[i], filepath.Base(path))
		if match {
			ok = false
			break
		}
		match, _ = filepath.Match(blacklist[i], pakpath)
		if match {
			ok = false
			break
		}
	}

	if ok {
		paths = append(paths, path)
	}
	return nil
}

func init() {
	flag.Parse()
	root = flag.Arg(0)
	pakname = flag.Arg(1)
}

func main() {
	log.SetFlags(0)
	log.SetOutput(os.Stdout)

	if flag.NArg() < 2 {
		log.Fatalln("usage: mkpak [dir] [pakname]")
	}

	err := filepath.Walk(root, visit)
	if err != nil {
		log.Fatalln(err)
	}

	var pakpaths []string
	for i := 0; i < len(paths); i++ {
		pp := strings.TrimPrefix(paths[i], root)
		pp = strings.Replace(pp, "\\", "/", -1)
		pakpaths = append(pakpaths, pp)
	}
	nin, nout, err := writepak(pakname+ext, paths, pakpaths)
	if err != nil {
		log.Fatalln(err)
	}
	log.Printf("\n%s: %d files, %s in, %s out\n",
		pakname+ext, len(paths), readable(nin), readable(nout))
}
