# EPV : Extra Predefined Variables for PHP

This extension adds support for `$_PUT`, `$_DELETE`, and `$_PATCH` superglobal variables, similar to how `$_POST` works, with full support for multipart/form-data and file uploads.

## Features

- **$_PUT**: Automatically populated when request method is PUT
- **$_DELETE**: Automatically populated when request method is DELETE
- **$_PATCH**: Automatically populated when request method is PATCH
- **File Uploads**: Full support for multipart/form-data and file uploads via `$_FILES`
- **Multiple Content-Types**: Supports both `application/x-www-form-urlencoded` and `multipart/form-data`

## Installation

### Using phpize (Recommended)

```bash
cd ext/epv
phpize
./configure
make
make install
```

Then add `extension=epv.so` to your php.ini and restart PHP-FPM:

```bash
echo "extension=epv.so" >> /etc/php.ini
systemctl restart php-fpm
```

### From PHP Source

1. This extension is in the `ext/epv` directory
2. Run the following commands:

```bash
cd /path/to/php-src
./buildconf --force
./configure --enable-epv [other options]
make
make install
```

## Usage

The extension works automatically once loaded. It creates three new superglobal variables:

### Example: PUT Request with Form Data

```php
<?php
// Client sends: PUT /api/user/123
// Content-Type: application/x-www-form-urlencoded
// Body: name=John&email=john@example.com

var_dump($_PUT);
// Output:
// array(2) {
//   ["name"]=>
//   string(4) "John"
//   ["email"]=>
//   string(16) "john@example.com"
// }
?>
```

### Example: PUT Request with File Upload

```php
<?php
// Client sends: PUT /api/avatar
// Content-Type: multipart/form-data
// Fields: name=John, avatar=(file)

var_dump($_PUT);
// array(1) {
//   ["name"]=> string(4) "John"
// }

var_dump($_FILES);
// array(1) {
//   ["avatar"]=> array(5) {
//     ["name"]=> string(10) "avatar.jpg"
//     ["type"]=> string(10) "image/jpeg"
//     ["tmp_name"]=> string(14) "/tmp/phpXXXXXX"
//     ["error"]=> int(0)
//     ["size"]=> int(12345)
//   }
// }

// Move uploaded file
if ($_FILES['avatar']['error'] === UPLOAD_ERR_OK) {
    move_uploaded_file(
        $_FILES['avatar']['tmp_name'],
        '/uploads/' . $_FILES['avatar']['name']
    );
}
?>
```

### Example: DELETE Request

```php
<?php
// Client sends: DELETE /api/user/123
// Content-Type: application/x-www-form-urlencoded
// Body: confirm=yes

var_dump($_DELETE);
// Output:
// array(1) {
//   ["confirm"]=>
//   string(3) "yes"
// }
?>
```

### Example: PATCH Request

```php
<?php
// Client sends: PATCH /api/user/123
// Content-Type: application/x-www-form-urlencoded
// Body: email=newemail@example.com

var_dump($_PATCH);
// Output:
// array(1) {
//   ["email"]=>
//   string(19) "newemail@example.com"
// }
?>
```

## Testing

You can test the extension using curl:

```bash
# Test PUT with form data
curl -X PUT -d "name=John&age=30" http://localhost/test.php

# Test PUT with file upload
curl -X PUT -F "name=John" -F "avatar=@photo.jpg" http://localhost/test.php

# Test DELETE
curl -X DELETE -d "id=123" http://localhost/test.php

# Test PATCH
curl -X PATCH -d "status=active" http://localhost/test.php

# Test multipart for all methods
curl -X PUT -F "field1=value1" -F "field2=value2" http://localhost/test.php
curl -X DELETE -F "confirm=yes" http://localhost/test.php
curl -X PATCH -F "email=new@email.com" http://localhost/test.php
```

## INI
```ini
; Limit to 10MB
epv.max_input_size = 10485760

; Unlimited (default)
epv.max_input_size = 0
