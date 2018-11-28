from conans import ConanFile, CMake

class QuariumController(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = "spdlog/0.17.0@bincrafters/stable", "catch2/2.3.0@bincrafters/stable", "jsonformoderncpp/3.1.2@vthiery/stable", "clara@1.1.4@bincrafters/stable", "boost_beast@1.66.0@bincrafters/stable", "wpa_ctrlpp/1.0@user/stable", "libgpiod/1.1.1-2@user/stable", "lvgl_cmake/5.1.1-3@user/stable"
    options = {"build_tests" : [True, False], "use_sdl" : [True, False], "use_gpiod_stub" : [True, False]}
    default_options = {"build_tests" : True, "use_sdl" : True, "use_gpiod_stub" : True}
    exports_sources = "tests*", "include*", "src*", "CMakeLists.txt"
    generators = "cmake", "ycm"

    def build(self):
        cmake = CMake(self)
        cmake.definitions["BUILD_TESTS"] = "On" if self.options.build_tests else "Off"
        cmake.definitions["GPIOD_STUB"] = "On" if self.options.use_gpiod_stub else "Off"
        cmake.definitions["USE_SDL"] = "On" if self.options.use_sdl else "Off"
        cmake.definitions["CMAKE_EXPORT_COMPILE_COMMANDS"] = "On"
        cmake.configure()
        cmake.build()

    def package(self):
        self.copy(pattern="*.h", dst="include", src="include")
        self.copy(pattern="*a", dst="lib", keep_path=False)
        self.copy(pattern="*so", dst="lib", keep_path=False)
        self.copy(pattern="*so.*", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
