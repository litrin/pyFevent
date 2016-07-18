from distutils.core import setup, Extension

module = Extension('pyf_event',
                    sources = ['pyf_event.c'])

setup (name = 'PerfEvent',
       version = '0.1',
       author = 'Litrin J.',
       author_email = "litrin@gmail.com",
       ext_modules = [module]
      )
