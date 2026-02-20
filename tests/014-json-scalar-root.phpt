--TEST--
EPV: PUT request with scalar JSON root (non object/array) results in empty array
--SKIPIF--
<?php
if (!extension_loaded('epv')) die('skip epv not loaded');
if (!extension_loaded('json')) die('skip json not loaded');
?>
--INI--
display_errors=0
log_errors=0
--CGI--
--ENV--
CONTENT_TYPE=application/json
--PUT--
123
--FILE--
<?php
var_dump($_PUT);
?>
--EXPECT--
array(0) {
}
