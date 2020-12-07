from conans import AutoToolsBuildEnvironment, CMake, ConanFile, tools
from conans.errors import ConanInvalidConfiguration
import os
import textwrap


class Mpg123Conan(ConanFile):
    name = "mpg123"
    description = "Fast console MPEG Audio Player and decoder library"
    topics = ("conan", "mpg123", "mpeg", "audio", "player", "decoder")
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "http://mpg123.org/"
    license = "LGPL-2.1-or-later", "GPL-2.0-or-later"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "flexible_resampling": [True, False],
        "network": [True, False],
        "icy": [True, False],
        "id3v2": [True, False],
        "ieeefloat": [True, False],
        "layer1": [True, False],
        "layer2": [True, False],
        "layer3": [True, False],
        "moreinfo": [True, False],
        "seektable": "ANY",
        "module": ["dummy", "libalsa", "tinyalsa", "win32"],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "flexible_resampling": True,
        "network": True,
        "icy": True,
        "id3v2": True,
        "ieeefloat": True,
        "layer1": True,
        "layer2": True,
        "layer3": True,
        "moreinfo": True,
        "seektable": "1000",
        "module": "dummy",
    }
    generators = "cmake", "cmake_find_package"

    @property
    def _source_subfolder(self):
        return "source_subfolder"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        del self.settings.compiler.libcxx
        del self.settings.compiler.cppstd
        if self.options.shared:
            del self.options.fPIC
        try:
            int(self.options.seektable)
        except ValueError:
            raise ConanInvalidConfiguration("seektable must be an integer")
        if self.settings.os != "Windows":
            if self.options == "win32":
                raise ConanInvalidConfiguration("win32 is an invalid module for non-Windows os'es")

    def requirements(self):
        if self.options.module == "libalsa":
            self.requires("libalsa/1.2.4")
        if self.options.module == "tinyalsa":
            self.requires("tinyalsa/1.1.1")

    def build_requirements(self):
        if self.settings.compiler == "Visual Studio" and self.settings.arch in ("x86", "x86_64"):
            self.build_requires("yasm/1.3.0")

    @property
    def _audio_module(self):
        return {
            "libalsa": "alsa",
        }.get(str(self.options.module), str(self.options.module))

    def build(self):
        tools.save("CMakeLists.txt",
                   textwrap.dedent(
                       """
                       cmake_minimum_required(VERSION 3.15)
                       project(cmake_wrapper)

                       include("{}/conanbuildinfo.cmake")
                       conan_basic_setup(TARGETS)

                       add_subdirectory("{}/ports/cmake" mpg123)
                       """).format(self.install_folder.replace("\\", "/"),
                                   self.source_folder.replace("\\", "/")))
        cmake = CMake(self)
        cmake.definitions["NO_MOREINFO"] = not self.options.moreinfo
        cmake.definitions["NETWORK"] = self.options.network
        cmake.definitions["NO_NTOM"] = not self.options.flexible_resampling
        cmake.definitions["NO_ICY"] = not self.options.icy
        cmake.definitions["NO_ID3V2"] = not self.options.id3v2
        cmake.definitions["IEEE_FLOAT"] = self.options.ieeefloat
        cmake.definitions["NO_LAYER1"] = not self.options.layer1
        cmake.definitions["NO_LAYER2"] = not self.options.layer2
        cmake.definitions["NO_LAYER3"] = not self.options.layer3
        cmake.definitions["USE_MODULES"] = False
        cmake.definitions["CHECK_MODULES"] = self._audio_module
        cmake.definitions["WITH_SEEKTABLE"] = self.options.seektable
        cmake.verbose = True
        cmake.parallel = False
        cmake.configure(source_folder=self.build_folder)
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.filenames["cmake_find_package"] = "mpg123"
        self.cpp_info.filenames["cmake_find_package_multi"] = "mpg123"
        self.cpp_info.names["cmake_find_package"] = "MPG123"
        self.cpp_info.names["cmake_find_package_multi"] = "MPG123"

        self.cpp_info.components["libmpg123"].libs = ["mpg123"]
        self.cpp_info.components["libmpg123"].names["pkg_config"] = "libmpg123"
        if self.settings.os == "Windows" and self.options.shared:
            self.cpp_info.components["libmpg123"].defines.append("LINK_MPG123_DLL")

        self.cpp_info.components["libout123"].libs = ["out123"]
        self.cpp_info.components["libout123"].names["pkg_config"] = "libout123"
        self.cpp_info.components["libout123"].requires = ["libmpg123"]

        self.cpp_info.components["libsyn123"].libs = ["syn123"]
        self.cpp_info.components["libsyn123"].names["pkg_config"] = "libsyn123"
        self.cpp_info.components["libsyn123"].requires = ["libmpg123"]

        if self.settings.os == "Linux":
            self.cpp_info.components["libmpg123"].system_libs = ["m"]
        elif self.settings.os == "Windows":
            self.cpp_info.components["libmpg123"].system_libs = ["shlwapi"]

        if self.options.module == "libalsa":
            self.cpp_info.components["libout123"].requires.append("libalsa::libalsa")
        if self.options.module == "tinyalsa":
            self.cpp_info.components["libout123"].requires.append("tinyalsa::tinyalsa")
        if self.options.module == "win32":
            self.cpp_info.components["libout123"].system_libs.append("winmm")
