
from setuptools import setup, Extension


setup(
    name='greet',
    version='1.0',
    description='Python Package with Hello World C Extension',
    ext_modules=[
        Extension(
            'pyhash_ext',
            sources=['pyhash_ext.cc'],
            # extra_compile_args=["-O0", "-g"],
            py_limited_api=True)
    ],
)
