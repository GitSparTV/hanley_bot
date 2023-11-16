```sh
conan install .. --build=missing -o boost:extra_b2_flags="define=BOOST_USE_WINAPI_VERSION=0x0A00" -s build_type=Debug -s compiler.runtime=MDd
```