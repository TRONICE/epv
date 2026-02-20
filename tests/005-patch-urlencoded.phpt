--TEST--
EPV: PATCH request with application/x-www-form-urlencoded body
--SKIPIF--
<?php if (!extension_loaded('epv')) die('skip epv not loaded'); ?>
--CGI--
--ENV--
REQUEST_METHOD=PATCH
CONTENT_TYPE=application/x-www-form-urlencoded
--POST_RAW--
status=ok
--FILE--
<?php
var_dump($_PUT);
var_dump($_DELETE);
var_dump($_PATCH);
?>
--EXPECT--
array(0) {
}
array(0) {
}
array(1) {
  ["status"]=>
  string(2) "ok"
}
