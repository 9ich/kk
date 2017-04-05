/*
 usage: go run mkpak.go baseq3 0
*/

package main

import (
	"archive/zip"
	"bytes"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"path/filepath"
	"strconv"
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
	"*/*/*_h.*",  // Height map (same as displacement maps).
	"*/*/*/*_ao.*",
	"*/*/*/*_d.*",
	"*/*test*",
	"*/*/*test*",
	"*!", // Test dirs.
	"*/*!/*",
	"*/*/*!/*",
}

var (
	root     string            // Directory to be packed.
	paknum   int               // e.g. 0 for pak0.
	paths    []string          // Filtered files to be included in the pak.
	pakpaths []string          // Same as paths but relative to pak's root.
	paks     []*zip.ReadCloser // Preceding pak files (pak0 to pakN-1) opened for fast access.
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

func delta(buf []byte, name string) bool {
	var occurrences int

	for i := paknum - 1; i >= 0; i-- {
		// Search for filename in previous paks.
		var j int
		for j = 0; j < len(paks[i].File); j++ {
			if paks[i].File[j].Name == name {
				break
			}
		}
		if j == len(paks[i].File) {
			continue	// Not found.
		}

		occurrences++

		rc, err := paks[i].File[j].Open()
		if err != nil {
			log.Fatalln(err)
		}
		b, err := ioutil.ReadAll(rc)
		if err != nil {
			log.Fatalln(err)
		}
		rc.Close()

		if bytes.Compare(buf, b) != 0 {
			log.Printf("delta\t%s differs from pak%d%s\n",
				name, i, ext)
			return true
		} else {
			return false
		}
	}

	if occurrences == 0 {
		log.Printf("new\t%s\n", name)
	}
	return occurrences == 0
}

func writepak(fname string) (int, int64, int64, error) {
	f, err := os.Create(fname)
	if err != nil {
		return 0, 0, 0, err
	}
	defer f.Close()
	w := zip.NewWriter(f)
	var nwrite int64
	var nfiles int
	for i := 0; i < len(paths); i++ {
		b, err := ioutil.ReadFile(paths[i])
		if err != nil {
			return 0, 0, 0, err
		}

		if !delta(b, pakpaths[i]) {
			continue
		}

		nfiles++

		f, err := w.Create(pakpaths[i])
		if err != nil {
			return 0, 0, 0, err
		}
		n, err := f.Write(b)
		if err != nil {
			return 0, 0, 0, err
		}
		nwrite += int64(n)
	}
	err = w.Close()
	if err != nil {
		return 0, 0, 0, err
	}

	stat, err := f.Stat()
	if err != nil {
		return 0, 0, 0, err
	}
	return nfiles, nwrite, stat.Size(), nil
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

func openPrecedingPaks() error {
	for i := 0; i < paknum; i++ {
		r, err := zip.OpenReader("pak" + strconv.Itoa(i) + ext)
		if err != nil {
			return err
		}
		paks = append(paks, r)
	}
	return nil
}

func closePrecedingPaks() {
	for _, r := range paks {
		if r != nil {
			r.Close()
		}
	}
}

func init() {
	flag.Parse()
	root = flag.Arg(0)
	paknum, _ = strconv.Atoi(flag.Arg(1))
}

func main() {
	log.SetFlags(0)
	log.SetOutput(os.Stdout)

	if flag.NArg() != 2 {
		log.Fatalln("usage: mkpak [dir] [paknum]")
	}

	openPrecedingPaks()
	defer closePrecedingPaks()

	// collect paths
	err := filepath.Walk(root, visit)
	if err != nil {
		log.Fatalln(err)
	}

	// transform to pakpaths
	for i := 0; i < len(paths); i++ {
		pp := strings.TrimPrefix(paths[i], root)
		pp = strings.Replace(pp, "\\", "/", -1)
		pakpaths = append(pakpaths, pp)
	}

	// write the pak
	fname := "pak" + strconv.Itoa(paknum) + ext
	nfiles, nin, nout, err := writepak(fname)
	if err != nil {
		log.Fatalln(err)
	}
	log.Printf("\n%s: %d files, %s in, %s out\n",
		fname, nfiles,
		readable(nin), readable(nout))
}
