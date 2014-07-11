# EPV : Extra Predefined Variables for PHP

## Requirement
* FreeBSD or Linux
* PHP 5.3 to 5.5

## Installation

### Compile
```
$ /path/to/phpize
$ ./configure
$ make install
```

### Setup
* Add epv.so to php.ini
* Restart httpd or php-fpm

## Usage
You will get two Extra Predefined Variables: $_PUT and $_DELETE

$_PUT will be predefined when $_SERVER['REQUEST_METHOD'] is PUT    
$_DELETE will bepredefined when $_SERVER['REQUEST_METHOD'] is DELETE

$_PUT and $_DELETE bthod support two $_SERVER['CONTENT_TYPE']:    
"application/x-www-form-urlencoded" and "multipart/form-data"

$FILES will be manipulated when $_SERVER['CONTENT_TYPE'] is "multipart/form-data" and input type is "file"
