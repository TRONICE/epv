--TEST--
EPV: DELETE request with application/x-www-form-urlencoded body
--SKIPIF--
<?php if (!extension_loaded('epv')) die('skip epv not loaded'); ?>
--CGI--
--ENV--
REQUEST_METHOD=DELETE
CONTENT_TYPE=application/x-www-form-urlencoded
--POST_RAW--
id=123
--FILE--
<?php
var_dump($_PUT);
var_dump($_DELETE);
var_dump($_PATCH);
?>
--EXPECT--
array(0) {
}
array(1) {
  ["id"]=>
  string(3) "123"
}
array(0) {
}
