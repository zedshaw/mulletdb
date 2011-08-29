
try:
    from setuptools import setup
except ImportError:
    from distutils.core import setup

config = {
    'description': 'NoNoSQL is Not A SQL Database Or A NoSQL Database, Not At All',
    'author': 'Zed A. Shaw',
    'url': 'http://pypi.python.org/pypi/mullet',
    'download_url': 'http://pypi.python.org/pypi/mullet',
    'author_email': 'zedshaw@zedshaw.com',
    'version': '0.1',
    'scripts': ['bin/mulletc'],
    'install_requires': ['nose', 'simplejson'],
    'packages': ['mullet'],
    'name': 'mullet'
}

setup(**config)

