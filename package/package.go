package cgfxcommon

import (
	cbase "github.com/jurgen-kluft/cbase/package"
	"github.com/jurgen-kluft/ccode/denv"
	cunittest "github.com/jurgen-kluft/cunittest/package"
)

// GetPackage returns the package object of 'cgfxcommon'
func GetPackage() *denv.Package {
	// Dependencies
	unittestpkg := cunittest.GetPackage()
	basepkg := cbase.GetPackage()

	// The main package
	mainpkg := denv.NewPackage("cgfxcommon")
	mainpkg.AddPackage(unittestpkg)
	mainpkg.AddPackage(basepkg)

	// 'cgfxcommon' library
	mainlib := denv.SetupDefaultCppLibProject("cgfxcommon", "github.com\\jurgen-kluft\\cgfxcommon")
	mainlib.Dependencies = append(mainlib.Dependencies, basepkg.GetMainLib())

	// 'cgfxcommon' unittest project
	maintest := denv.SetupDefaultCppTestProject("cgfxcommon"+"_test", "github.com\\jurgen-kluft\\cgfxcommon")
	maintest.Dependencies = append(maintest.Dependencies, unittestpkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, mainlib)

	mainpkg.AddMainLib(mainlib)
	mainpkg.AddUnittest(maintest)
	return mainpkg
}
