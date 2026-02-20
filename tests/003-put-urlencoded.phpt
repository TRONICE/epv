--TEST--
EPV: PUT request with application/x-www-form-urlencoded body
--SKIPIF--
<?php if (!extension_loaded('epv')) die('skip epv not loaded'); ?>
--CGI--
--ENV--
CONTENT_TYPE=application/x-www-form-urlencoded
--PUT--
name=John
--FILE--
<?php
var_dump($_PUT);
var_dump($_DELETE);
var_dump($_PATCH);
?>
--EXPECT--
array(1) {
  ["name"]=>
  string(4) "John"
}
array(0) {
}
array(0) {
}
