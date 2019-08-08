""" ConanFile for JoSIM """
import re
from conans import ConanFile, CMake, tools


def get_version():
    """ Get version from CMakeLists """

    try:
        content = tools.load("CMakeLists.txt")
        version = re.search(r"project\(JoSIM (.*)\)", content).group(1)
        return version.strip()
    except IOError:
        # If the package is installed, and there is no CMakeLists.txt, the
        # version info is in the package metadata
        return None


class JoSIM(ConanFile):
    """ Conan package for iron::io """

    name = "JoSIM"
    version = get_version()
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = ("include/*", "source/*", "tests/*", "CMakeLists.txt")
    generators = "cmake"

    def _cmake_configure(self):
        """ Shared cmake configure step """
        cmake = CMake(self)
        cmake.configure()

        return cmake

    def build(self):
        cmake = self._cmake_configure()
        cmake.build()
        cmake.test(["--", "CTEST_OUTPUT_ON_FAILURE=TRUE"])

    def package(self):
        cmake = self._cmake_configure()
        cmake.install()

    def build_requirements(self):
        pass

    def imports(self):
        self.copy("*.dll", "", src="bin")
        self.copy("*.dylib*", "", src="lib")
        self.copy("*.so*", "", src="bin")
