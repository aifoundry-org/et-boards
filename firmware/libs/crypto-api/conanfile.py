from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain
from conan.tools.layout import cmake_layout
from conans import tools
import os


class CryptoApiConan(ConanFile):
    name = "cryptoApi"
    url = "https://gitlab.esperanto.ai/software/crypto-api"
    description = ""
    license = "Esperanto Technologies"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "warnings_as_errors": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "warnings_as_errors": False,
    }

    scm = {
        "type": "git",
        "url": "git@gitlab.esperanto.ai:software/crypto-api.git",
        "revision": "auto",
    }
    generators = "CMakeDeps"


    python_requires = "conan-common/[>=0.5.0 <1.0.0]"

    def set_version(self):
        self.version = self.python_requires["conan-common"].module.get_version_from_cmake_project(self, self.name)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC
    
    def configure(self):
        if self.options.shared:
            del self.options.fPIC
        # cryptoApi is a pure-C library
        del self.settings.compiler.cppstd
        del self.settings.compiler.libcxx
    
    def requirements(self):
        self.requires("signedImageFormat/1.4.0")
        self.requires("json-c/0.14")
        self.requires("openssl/1.1.1h")
        self.requires("pkcs11/20180731")
    
    def build_requirements(self):
        self.build_requires("cmake-modules/[>=0.4.1 <1.0.0]")
    
    def validate(self):
        check_req_min_cppstd = self.python_requires["conan-common"].module.check_req_min_cppstd
        check_req_min_cppstd(self, "17")
    
    def layout(self):
        cmake_layout(self)
        
    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_MODULE_PATH"] = os.path.join(self.dependencies.build["cmake-modules"].package_folder, "cmake")
        tc.variables["CRYPTO_API_DEPRECATED"] = False
        tc.variables["CRYPTO_API_WARNINGS_AS_ERRORS"] = self.options.warnings_as_errors
        tc.generate()
    
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
    
    def package(self):
        cmake = CMake(self)
        cmake.install()
        tools.rmdir(os.path.join(self.package_folder, "lib", "cmake"))
    
    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
        self.cpp_info.set_property("cmake_target_name", "cryptoApi::cryptoApi")
        self.cpp_info.requires = [
            "signedImageFormat::signedImageFormat",
            "json-c::json-c",
            "pkcs11::pkcs11"
        ]
