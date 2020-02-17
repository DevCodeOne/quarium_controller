from conans import ConanFile, CMake

class QuariumController(ConanFile):
    name = "quarium_controller"
    version = "none"
    settings = "os", "compiler", "build_type", "arch"
    requires =  "OpenSSL/1.1.1c@conan/stable", \
            "spdlog/1.5.0", \
            "catch2/2.3.0@bincrafters/stable", \
            "jsonformoderncpp/3.7.3@vthiery/stable", \
            "clara/1.1.4@bincrafters/stable", \
            "cpp-httplib/0.3.1@devcodeone/stable", \
            "libgpiod/1.2.1@ecashptyltd/stable", \
            "mqtt_cpp/8.x@devcodeone/stable", \
            "wt/4.0.4@bincrafters/stable"
    options = { "build_tests" : [True, False],
            "use_sdl" : [True, False],
            "with_gui" : [True, False],
            "use_gpiod_stub" : [True, False],
            }
    default_options = { "build_tests" : True, \
            "use_sdl" : False, \
            "use_gpiod_stub" : True, \
            "with_gui" : False, \
            "wt:shared" : False, \
            "wt:with_ssl" : True, \
            "wt:with_haru" : False, \
            "wt:with_pango" : False, \
            "wt:with_sqlite" : False, \
            "wt:with_postgres" : False, \
            "wt:with_firebird" : False, \
            "wt:with_mysql" : False, \
            "wt:with_mssql" : False, \
            "wt:with_qt4" : False, \
            "wt:with_test" : False, \
            "wt:with_dbo" : False, \
            "wt:with_opengl" : False, \
            "wt:with_unwind" : False, \
            "wt:multi_threaded" : True, \
            "wt:connector_http" : True, \
            "wt:connector_fcgi": False
            }
    exports_sources = "tests*", "include*", "src*", "CMakeLists.txt"
    generators = "cmake", "ycm"

    def configure(self):
        if self.options.with_gui:
            self.requires("lvgl_cmake/5.1.1-3@user/stable")
            if self.options.use_sdl:
                self.requires("sdl2/2.0.9@bincrafters/stable")
                self.options["sdl2"].jack = False
                self.options["sdl2"].alsa = False
                self.options["sdl2"].pulse = False
                self.options["sdl2"].nas = False
            else:
                self.requires("tslib/1.18-1@user/stable")

    def build(self):
        cmake = CMake(self)
        cmake.definitions["WITH_GUI"] = 1 if self.options.with_gui else 0
        cmake.definitions["BUILD_TESTS"] = 1 if self.options.build_tests else 0
        cmake.definitions["GPIOD_STUB"] = 1 if self.options.use_gpiod_stub else 0
        cmake.definitions["USE_SDL"] = 1 if self.options.use_sdl and self.options.with_gui else 0
        cmake.definitions["CMAKE_EXPORT_COMPILE_COMMANDS"] = 1
        cmake.configure()
        cmake.build()

    def package(self):
        self.copy(pattern="quarium_controller*", dst="bin", src="bin")
        self.copy(pattern="*.h", dst="include", src="include", keep_path=False)
        self.copy(pattern="*.a", dst="lib", keep_path=False)
        self.copy(pattern="*so", dst="lib", keep_path=False)
        self.copy(pattern="*so.*", dst="lib", keep_path=False)

    def deploy(self):
        self.copy("bin/quarium_controller")
        self.copy_deps("*.so")
        self.copy_deps("*.so.*")
