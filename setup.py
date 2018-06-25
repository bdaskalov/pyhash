
from setuptools import setup, Extension


setup(
    name='greet',
    version='1.0',
    description='Python Package with Hello World C Extension',
    ext_modules=[
        Extension(
            'pyhash',
            sources=['pyhash.cc'],
            #libraries=['boost_python-py35'],
            extra_compile_flags=["-g"],
            py_limited_api=True)
    ],
)
