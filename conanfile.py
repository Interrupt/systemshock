from conans import ConanFile, CMake

class SystemShockConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    # Comma-separated list of requirements
    build_requires = [
        "glew/2.1.0@bincrafters/stable",
        "sdl2_mixer/2.0.4@bincrafters/stable",
    ]
    generators = "cmake_find_package"

    def configure(self):
        # Disable unused dependencies
        if self.settings.os == "Linux":
            self.options["sdl2"].jack = False
            self.options["sdl2"].nas = False
            self.options["sdl2_mixer"].tinymidi = False
        if self.settings.os == "Windows":
            self.options["sdl2_mixer"].flac = False
            self.options["sdl2_mixer"].mad = False
            self.options["sdl2_mixer"].modplug = False
            self.options["sdl2_mixer"].mpg123 = False
            self.options["sdl2_mixer"].ogg = False
            self.options["sdl2_mixer"].opus = False
            self.options["glew"].shared = True              # Should be shared for MinGW

    def imports(self):
        self.copy("*.dll", "", "bin")

